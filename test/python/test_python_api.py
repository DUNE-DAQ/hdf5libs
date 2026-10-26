"""Tests of reading and writing HDF5 raw data files from Python, through rawdatafilelibs (both the
C++-mirror bindings and the Pythonic layer) and through the deprecated hdf5libs.HDF5RawDataFile.
Needs HDF5LIBS_TestWriter on PATH (it is, in a work area with hdf5libs built). Run with:
    pytest -v sourcecode/hdf5libs/test/python
"""

import json
import pathlib
import shutil
import subprocess
import warnings

import pytest

import daqdataformats
import detdataformats
import rawdatafilelibs
import hdf5libs

APPS_DIR = pathlib.Path(__file__).resolve().parent.parent / "apps"


@pytest.fixture(scope="module")
def test_file(tmp_path_factory):
    """A TriggerRecord file written by HDF5LIBS_TestWriter: 5 records of 12 fragments."""
    if shutil.which("HDF5LIBS_TestWriter") is None:
        pytest.skip("HDF5LIBS_TestWriter is not on PATH")
    directory = tmp_path_factory.mktemp("hdf5libs_python")
    path = directory / "testwriter.hdf5"
    subprocess.run(
        ["HDF5LIBS_TestWriter", str(APPS_DIR / "testwriter_conf.json"),
         str(APPS_DIR / "testwriter_hardwaremap.txt"), str(path)],
        check=True, capture_output=True)
    conf = json.loads((APPS_DIR / "testwriter_conf.json").read_text())
    return path, conf


def test_pythonic_reading(test_file):
    path, conf = test_file
    with rawdatafilelibs.open(path) as f:
        assert f.record_type == "TriggerRecord"
        assert f.metadata.run_number == conf["run_number"]
        assert f.metadata.layout_version == hdf5libs.s_current_file_layout_version
        assert len(f) == conf["trigger_count"]
        assert [rid.number for rid in f.record_ids] == list(range(1, conf["trigger_count"] + 1))

        records = list(f)
        assert [record.number for record in records] == list(range(1, conf["trigger_count"] + 1))

        record = f[3]
        assert record.header.get_trigger_number() == 3
        assert len(record) == conf["element_count"]
        assert len(record.source_ids(subsystem=conf["subsystem_type"])) == conf["element_count"]
        assert record.source_ids(subsystem="Trigger") == []
        assert len(record.source_ids(fragment_type=conf["fragment_type"])) == conf["element_count"]
        fragments = list(record.fragments(subdetector=conf["subdetector_type"]))
        assert len(fragments) == conf["element_count"]
        assert all(frag.get_trigger_number() == 3 for frag in fragments)
        assert len({frag.get_data_size() for frag in fragments}) == 1

        trigger_record = record.assemble()
        assert len(trigger_record.get_fragments_ref()) == conf["element_count"]


def test_backend_extras(test_file):
    path, _ = test_file
    with rawdatafilelibs.open(path) as f:
        backend = f.backend
        assert isinstance(backend, hdf5libs.HDF5ReaderBackend)
        assert not backend.is_legacy_layout()
        record = f[1]
        header_path = backend.get_dataset_path(record.id, record.header_source_id)
        assert header_path.startswith("/TriggerRecord000001.0000/RawData/")
        assert header_path in backend.get_dataset_paths()
        fragment_bytes = backend.read_dataset(backend.get_dataset_path(record.id, record.source_ids()[0]))
        assert len(fragment_bytes) == record.fragment(record.source_ids()[0]).get_size()


def test_compatibility_names_match_hdf5rawdatafile(test_file):
    path, _ = test_file
    with warnings.catch_warnings(record=True) as caught:
        warnings.simplefilter("always")
        old = hdf5libs.HDF5RawDataFile(str(path))
    assert any(issubclass(w.category, DeprecationWarning) for w in caught)

    with rawdatafilelibs.open(path) as new:
        assert new.get_all_record_ids() == [tuple(rid) for rid in old.get_all_record_ids()]
        with pytest.deprecated_call():
            assert new.get_all_trigger_record_ids() == [tuple(rid) for rid in old.get_all_trigger_record_ids()]
        assert new.get_record_type() == old.get_record_type()
        assert new.is_trigger_record_type() and not new.is_timeslice_type()
        assert new.get_recorded_size() == old.get_recorded_size()
        for rid in old.get_all_record_ids():
            with pytest.deprecated_call():
                assert new.get_source_ids(rid) == set(old.get_source_ids(rid))
            assert new.get_geo_ids(rid) == set(old.get_geo_ids(rid))
            assert new.get_trh(rid).get_trigger_timestamp() == old.get_trh(rid).get_trigger_timestamp()
            for source_id in new.get_fragment_source_ids(rid):
                assert new.get_frag(rid, source_id).get_data_bytes() == old.get_frag(rid, source_id).get_data_bytes()
        with pytest.raises(TypeError), pytest.deprecated_call():
            new.get_all_timeslice_ids()
    old.close()


def test_copy_records_to_new_files(test_file, tmp_path):
    """Records read from one file can be written to another, of the same or another format."""
    path, conf = test_file
    rawdatafilelibs.clear_in_memory_files()
    with rawdatafilelibs.open(path) as source:
        for destination in (tmp_path / "copy.hdf5", "copy.rdfmem"):
            with rawdatafilelibs.create(destination, "TriggerRecord", geo_id_map=source.geo_id_map,
                                        run_number=source.metadata.run_number) as writer:
                for record in source:
                    if record.number % 2 == 1:  # a filter
                        writer.write(record.assemble())
                with pytest.raises(RuntimeError, match="already exists"):
                    writer.write(source[1].assemble())

            with rawdatafilelibs.open(destination) as copy:
                assert [rid.number for rid in copy.record_ids] == [1, 3, 5]
                for record in copy:
                    original = source[record.id]
                    assert record.index == original.index
                    for source_id in record.source_ids():
                        assert (record.fragment(source_id).get_data_bytes()
                                == original.fragment(source_id).get_data_bytes())
    rawdatafilelibs.clear_in_memory_files()


def test_file_open_for_writing_is_hidden_from_readers_until_closed(test_file, tmp_path):
    path, _ = test_file
    destination = tmp_path / "inprogress.hdf5"
    with rawdatafilelibs.open(path) as source:
        writer = rawdatafilelibs.create(destination, "TriggerRecord")
        writer.write(source[1].assemble())
        assert not destination.exists()
        assert pathlib.Path(str(destination) + ".writing").exists()
        writer.close()
    assert destination.exists()
    with rawdatafilelibs.open(destination) as copy:
        assert copy.metadata.closing_timestamp_ms >= copy.metadata.creation_timestamp_ms > 0
        assert copy.metadata.stats.recorded_size > 0


def test_file_attributes_through_the_backend(test_file):
    path, conf = test_file
    with rawdatafilelibs.open(path) as f:
        backend = f.backend
        names = backend.get_attribute_names()
        assert {"run_number", "creation_timestamp", "record_type"} <= set(names)
        assert backend.get_attribute("run_number") == conf["run_number"] == f.metadata.run_number
        assert backend.get_attribute("creation_timestamp") == f.metadata.creation_timestamp_ms
        assert backend.get_attribute("record_type") == "TriggerRecord"
        assert not backend.has_attribute("file_recovery_timestamp")
        with pytest.raises(KeyError):
            backend.get_attribute("file_recovery_timestamp")


def test_recover_an_unfinished_file(test_file, tmp_path):
    path, _ = test_file
    unfinished = tmp_path / "recovered.hdf5.writing"
    shutil.copy(path, unfinished)  # a complete file still under its in-progress name

    report = rawdatafilelibs.recover_raw_data_file(str(unfinished))
    assert not report.applied and report.final_path == str(tmp_path / "recovered.hdf5")
    assert any(change.startswith("rename to") for change in report.changes)
    assert unfinished.exists()

    report = rawdatafilelibs.recover_raw_data_file(str(unfinished), apply=True)
    assert report.applied and report.renamed and "Done" in str(report)
    with rawdatafilelibs.open(tmp_path / "recovered.hdf5") as f:
        assert f.metadata.recovery_timestamp_ms > 0
        assert f.backend.get_attribute("file_recovery_timestamp") == f.metadata.recovery_timestamp_ms


def test_shared_geo_id_gives_the_first_source_id(test_file):
    """The compatibility get_frag(record, geo_id) answers like HDF5RawDataFile: the first SourceID."""
    path, _ = test_file
    rawdatafilelibs.clear_in_memory_files()
    with rawdatafilelibs.open(path) as source:
        record = source[1]
        first, second = list(record.source_ids())[:2]
        shared = 0x0001000200030003  # an encoded GeoID listed under both SourceIDs
        geo_id_map = daqdataformats.SourceIDGeoIDMap.from_encoded({first: [shared], second: [shared]})
        with rawdatafilelibs.create("shared.rdfmem", "TriggerRecord", geo_id_map=geo_id_map) as writer:
            writer.write(record.assemble())
    with rawdatafilelibs.open("shared.rdfmem") as copy:
        assert copy.geo_id_map.get_source_id(detdataformats.GeoID.decode(shared)) is None
        owners = copy.geo_id_map.get_source_ids(detdataformats.GeoID.decode(shared))
        assert len(owners) == 2 and set(owners) == {first, second}
        with pytest.deprecated_call():
            assert copy.get_frag(1, shared).get_element_id() == owners[0]
    rawdatafilelibs.clear_in_memory_files()
