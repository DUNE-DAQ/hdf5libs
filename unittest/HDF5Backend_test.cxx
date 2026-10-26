/**
 * @file HDF5Backend_test.cxx Unit tests of the HDF5 backend for rawdatafilelibs, and of the
 * on-disk layout it writes (which must stay the one HDF5RawDataFile has always written)
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "hdf5libs/HDF5Backend.hpp"
#include "hdf5libs/HDF5RawDataFile.hpp"
#include "hdf5libs/Issues.hpp"

#include "rawdatafilelibs/BackendFactory.hpp"
#include "rawdatafilelibs/Issues.hpp"
#include "rawdatafilelibs/RawDataFileReader.hpp"
#include "rawdatafilelibs/RawDataFileWriter.hpp"

#include "daqdataformats/Fragment.hpp"
#include "daqdataformats/TimeSlice.hpp"
#include "daqdataformats/TriggerRecord.hpp"
#include "detdataformats/DetID.hpp"
#include "detdataformats/GeoID.hpp"

#define BOOST_TEST_MODULE HDF5Backend_test // NOLINT

#include "boost/test/unit_test.hpp"

#include <highfive/H5File.hpp>

#include <cstring>
#include <filesystem>
#include <memory>
#include <set>
#include <string>
#include <unistd.h>
#include <vector>

using namespace dunedaq;
using namespace dunedaq::hdf5libs;
using daqdataformats::FragmentType;
using daqdataformats::RecordID;
using daqdataformats::SourceID;
using detdataformats::DetID;
using detdataformats::GeoID;
using rawdatafilelibs::FileMetadata;
using rawdatafilelibs::RawDataFileWriter;
using rawdatafilelibs::RecordType;

namespace {

constexpr size_t s_fragment_payload_size = 64;
const SourceID s_header_source_id(SourceID::Subsystem::kTRBuilder, 0);

// the fragments in every test record: SourceID, fragment type, subdetector
struct FragmentSpec
{
  SourceID source_id;
  FragmentType fragment_type;
  DetID::Subdetector subdetector;
};
const std::vector<FragmentSpec> s_fragments = {
  { SourceID(SourceID::Subsystem::kDetectorReadout, 0), FragmentType::kWIBEth, DetID::Subdetector::kHD_TPC },
  { SourceID(SourceID::Subsystem::kDetectorReadout, 1), FragmentType::kWIBEth, DetID::Subdetector::kHD_TPC },
  { SourceID(SourceID::Subsystem::kDetectorReadout, 2), FragmentType::kDAPHNE, DetID::Subdetector::kHD_PDS },
  { SourceID(SourceID::Subsystem::kTrigger, 0), FragmentType::kTriggerActivity, DetID::Subdetector::kDAQ },
};

std::string
make_test_dir(const std::string& test_name)
{
  auto dir =
    std::filesystem::temp_directory_path() / ("HDF5Backend_test_" + test_name + "_" + std::to_string(::getpid()));
  std::filesystem::remove_all(dir);
  std::filesystem::create_directories(dir);
  return dir.string();
}

std::unique_ptr<daqdataformats::Fragment>
make_fragment(const FragmentSpec& spec, uint64_t record_number, uint8_t fill) // NOLINT(build/unsigned)
{
  std::vector<char> payload(s_fragment_payload_size, static_cast<char>(fill));
  auto fragment = std::make_unique<daqdataformats::Fragment>(payload.data(), payload.size());
  fragment->set_trigger_number(record_number);
  fragment->set_sequence_number(0);
  fragment->set_run_number(42);
  fragment->set_element_id(spec.source_id);
  fragment->set_type(spec.fragment_type);
  fragment->set_detector_id(static_cast<uint16_t>(spec.subdetector)); // NOLINT(build/unsigned)
  return fragment;
}

daqdataformats::TriggerRecord
make_trigger_record(uint64_t trigger_number) // NOLINT(build/unsigned)
{
  std::vector<daqdataformats::ComponentRequest> requests;
  requests.reserve(s_fragments.size());
  for (auto const& spec : s_fragments) {
    requests.emplace_back(spec.source_id, 1000, 2000);
  }
  daqdataformats::TriggerRecordHeader header(requests);
  header.set_trigger_number(trigger_number);
  header.set_run_number(42);
  header.set_sequence_number(0);
  header.set_max_sequence_number(1);
  header.set_element_id(s_header_source_id);

  daqdataformats::TriggerRecord record(header);
  uint8_t fill = 1; // NOLINT(build/unsigned)
  for (auto const& spec : s_fragments) {
    record.add_fragment(make_fragment(spec, trigger_number, fill));
    ++fill;
  }
  return record;
}

daqdataformats::SourceIDGeoIDMap
make_geo_id_map()
{
  daqdataformats::SourceIDGeoIDMap geo_id_map;
  geo_id_map.add(s_fragments[0].source_id, GeoID(DetID::Subdetector::kHD_TPC, 1, 0, 0));
  geo_id_map.add(s_fragments[1].source_id, GeoID(DetID::Subdetector::kHD_TPC, 1, 0, 1));
  geo_id_map.add(s_fragments[2].source_id, GeoID(DetID::Subdetector::kHD_PDS, 2, 3, 4));
  // a SourceID that no record contains
  geo_id_map.add(SourceID(SourceID::Subsystem::kDetectorReadout, 99), GeoID(DetID::Subdetector::kHD_TPC, 9, 9, 9));
  return geo_id_map;
}

FileMetadata
make_metadata(RecordType record_type = RecordType::kTriggerRecord)
{
  FileMetadata metadata;
  metadata.run_number = 42;
  metadata.file_index = 3;
  metadata.application_name = "HDF5Backend_test";
  metadata.record_type = record_type;
  metadata.creation_timestamp_ms = 1700000000123;
  metadata.operational_environment = "unittest";
  metadata.offline_data_stream = "cosmics";
  metadata.run_is_for_test_purposes = true;
  return metadata;
}

std::set<std::string>
attribute_names(const HighFive::File& file)
{
  auto names = file.listAttributeNames();
  return { names.begin(), names.end() };
}

template<typename T>
T
read_attribute(const HighFive::File& file, const std::string& name)
{
  T value;
  file.getAttribute(name).read(value);
  return value;
}

} // namespace

BOOST_AUTO_TEST_SUITE(HDF5Backend_test)

BOOST_AUTO_TEST_CASE(RoundTripThroughPlugin)
{
  const auto dir = make_test_dir("RoundTripThroughPlugin");
  const std::string file_name = dir + "/run42.hdf5";
  const auto geo_id_map = make_geo_id_map();

  rawdatafilelibs::FileStats written_stats;
  {
    auto writer = rawdatafilelibs::create_raw_data_file(file_name, make_metadata(), geo_id_map);
    BOOST_REQUIRE(std::filesystem::exists(file_name + ".writing"));
    writer->write(make_trigger_record(1));
    writer->write(make_trigger_record(2));
    BOOST_REQUIRE_THROW(writer->write(make_trigger_record(2)), rawdatafilelibs::RecordAlreadyExists);
    writer->close();
    written_stats = writer->get_metadata().stats;
  }
  BOOST_REQUIRE(std::filesystem::exists(file_name));
  BOOST_REQUIRE(!std::filesystem::exists(file_name + ".writing"));
  BOOST_REQUIRE(written_stats.recorded_size > 0);

  auto reader = rawdatafilelibs::open_raw_data_file(file_name);
  BOOST_REQUIRE(dynamic_cast<HDF5ReaderBackend*>(&reader->get_backend()) != nullptr);

  auto const& metadata = reader->get_metadata();
  auto expected = make_metadata();
  BOOST_REQUIRE_EQUAL(metadata.run_number, expected.run_number);
  BOOST_REQUIRE_EQUAL(metadata.file_index, expected.file_index);
  BOOST_REQUIRE_EQUAL(metadata.application_name, expected.application_name);
  BOOST_REQUIRE(metadata.record_type == RecordType::kTriggerRecord);
  BOOST_REQUIRE_EQUAL(metadata.creation_timestamp_ms, expected.creation_timestamp_ms);
  BOOST_REQUIRE(metadata.closing_timestamp_ms >= metadata.creation_timestamp_ms);
  BOOST_REQUIRE_EQUAL(metadata.operational_environment, expected.operational_environment);
  BOOST_REQUIRE_EQUAL(metadata.offline_data_stream, expected.offline_data_stream);
  BOOST_REQUIRE(metadata.run_is_for_test_purposes);
  BOOST_REQUIRE_EQUAL(metadata.compression_level, 0);
  BOOST_REQUIRE_EQUAL(metadata.layout_version, s_current_file_layout_version);
  BOOST_REQUIRE(metadata.stats == written_stats);

  BOOST_REQUIRE(reader->get_geo_id_map() == geo_id_map);
  BOOST_REQUIRE((reader->get_record_ids() == std::set<RecordID>{ RecordID{ 1 }, RecordID{ 2 } }));

  auto const& index = reader->get_index(RecordID{ 2 });
  BOOST_REQUIRE_EQUAL(index.get_header_source_id(), s_header_source_id);
  BOOST_REQUIRE_EQUAL(index.get_fragments().size(), s_fragments.size());
  for (auto const& spec : s_fragments) {
    auto info = index.get_fragment_info(spec.source_id);
    BOOST_REQUIRE(info.has_value());
    BOOST_REQUIRE(info->fragment_type == spec.fragment_type);
    BOOST_REQUIRE(info->subdetector == spec.subdetector);
  }

  // the record comes back byte for byte
  auto original = make_trigger_record(2);
  auto record = reader->get_trigger_record(RecordID{ 2 });
  BOOST_REQUIRE_EQUAL(record.get_header_ref().get_total_size_bytes(), original.get_header_ref().get_total_size_bytes());
  BOOST_REQUIRE_EQUAL(std::memcmp(record.get_header_ref().get_storage_location(),
                                  original.get_header_ref().get_storage_location(),
                                  original.get_header_ref().get_total_size_bytes()),
                      0);
  BOOST_REQUIRE_EQUAL(record.get_fragments_ref().size(), s_fragments.size());
  for (auto const& spec : s_fragments) {
    auto fragment = reader->get_fragment(RecordID{ 2 }, spec.source_id);
    BOOST_REQUIRE_EQUAL(fragment->get_element_id(), spec.source_id);
    BOOST_REQUIRE_EQUAL(fragment->get_size(), sizeof(daqdataformats::FragmentHeader) + s_fragment_payload_size);
  }

  // per-record GeoIDs are only those of fragments in the record
  BOOST_REQUIRE_EQUAL(reader->get_geo_ids(RecordID{ 1 }).size(), 3);

  reader->close();
  std::filesystem::remove_all(dir);
}

BOOST_AUTO_TEST_CASE(TimeSliceRoundTrip)
{
  const auto dir = make_test_dir("TimeSliceRoundTrip");
  const std::string file_name = dir + "/tp.hdf5";
  {
    auto writer = rawdatafilelibs::create_raw_data_file(
      file_name, make_metadata(RecordType::kTimeSlice), daqdataformats::SourceIDGeoIDMap());
    daqdataformats::TimeSliceHeader header;
    header.timeslice_number = 7;
    header.run_number = 42;
    header.element_id = SourceID(SourceID::Subsystem::kTRBuilder, 1);
    daqdataformats::TimeSlice timeslice(header);
    timeslice.add_fragment(make_fragment(s_fragments[3], 7, 9));
    writer->write(timeslice);
    BOOST_REQUIRE_THROW(writer->write(make_trigger_record(1)), rawdatafilelibs::RecordTypeMismatch);
    writer->close();
  }

  HighFive::File file(file_name, HighFive::File::ReadOnly);
  BOOST_REQUIRE(file.exist("/TimeSlice000007/RawData/" + SourceID(SourceID::Subsystem::kTRBuilder, 1).to_string() +
                           "_TimeSliceHeader"));

  auto reader = rawdatafilelibs::open_raw_data_file(file_name);
  BOOST_REQUIRE(reader->get_record_type() == RecordType::kTimeSlice);
  auto timeslice = reader->get_timeslice(RecordID{ 7 });
  BOOST_REQUIRE_EQUAL(timeslice.get_header().timeslice_number, 7);
  BOOST_REQUIRE_EQUAL(timeslice.get_fragments_ref().size(), 1);
  BOOST_REQUIRE_THROW(reader->get_trigger_record(RecordID{ 7 }), rawdatafilelibs::RecordTypeMismatch);
  reader->close();
  std::filesystem::remove_all(dir);
}

// The layout written by the backend, spelled out: group and dataset names, attribute names and types,
// exactly as HDF5RawDataFile wrote them before it was built on the backend.
BOOST_AUTO_TEST_CASE(OnDiskLayout)
{
  const auto dir = make_test_dir("OnDiskLayout");
  const std::string file_name = dir + "/layout.hdf5";
  {
    auto writer = rawdatafilelibs::create_raw_data_file(file_name, make_metadata(), make_geo_id_map());
    writer->write(make_trigger_record(1));
    writer->close();
  }

  HighFive::File file(file_name, HighFive::File::ReadOnly);
  BOOST_REQUIRE((attribute_names(file) == std::set<std::string>{ "run_number",
                                                                 "file_index",
                                                                 "creation_timestamp",
                                                                 "application_name",
                                                                 "filelayout_params",
                                                                 "filelayout_version",
                                                                 "source_id_geo_id_map",
                                                                 "record_type",
                                                                 "compression_level",
                                                                 "operational_environment",
                                                                 "offline_data_stream",
                                                                 "run_was_for_test_purposes",
                                                                 "recorded_size",
                                                                 "uncompressed_raw_data_size",
                                                                 "total_file_size",
                                                                 "closing_timestamp" }));
  BOOST_REQUIRE(file.getAttribute("run_number").getDataType() == HighFive::create_datatype<uint32_t>()); // NOLINT
  BOOST_REQUIRE(file.getAttribute("creation_timestamp").getDataType() == HighFive::create_datatype<size_t>());
  BOOST_REQUIRE(file.getAttribute("closing_timestamp").getDataType() == HighFive::create_datatype<size_t>());
  BOOST_REQUIRE(file.getAttribute("compression_level").getDataType() == HighFive::create_datatype<unsigned>());
  BOOST_REQUIRE_EQUAL(read_attribute<uint32_t>(file, "filelayout_version"), 7); // NOLINT(build/unsigned)
  BOOST_REQUIRE_EQUAL(read_attribute<std::string>(file, "record_type"), "TriggerRecord");
  BOOST_REQUIRE_EQUAL(read_attribute<std::string>(file, "run_was_for_test_purposes"), "true");
  BOOST_REQUIRE_EQUAL(read_attribute<size_t>(file, "creation_timestamp"), 1700000000123);

  const std::string record_group = "/TriggerRecord000001.0000";
  BOOST_REQUIRE(file.exist(record_group + "/RawData/" + s_header_source_id.to_string() + "_TriggerRecordHeader"));
  for (auto const& spec : s_fragments) {
    BOOST_REQUIRE(file.exist(record_group + "/RawData/" + spec.source_id.to_string() + "_" +
                             daqdataformats::fragment_type_to_string(spec.fragment_type)));
  }
  auto group = file.getGroup(record_group);
  auto group_attribute_names = group.listAttributeNames();
  BOOST_REQUIRE(
    (std::set<std::string>(group_attribute_names.begin(), group_attribute_names.end()) ==
     std::set<std::string>{
       "record_header_source_id", "source_id_path_map", "fragment_type_source_id_map", "subdetector_source_id_map" }));
  std::filesystem::remove_all(dir);
}

// The facade's original writing constructor writes the same file as the backend, minus the
// run-context attributes (which its callers write themselves, if at all)
BOOST_AUTO_TEST_CASE(FacadeWritesTheSameLayout)
{
  const auto dir = make_test_dir("FacadeWritesTheSameLayout");
  const std::string facade_name = dir + "/facade.hdf5";
  const std::string backend_name = dir + "/backend.hdf5";
  const auto geo_id_map = make_geo_id_map();

  {
    HDF5RawDataFile facade(facade_name,
                           42,
                           3,
                           "HDF5Backend_test",
                           default_file_layout_parameters(RecordType::kTriggerRecord),
                           geo_id_map.to_encoded());
    facade.write(make_trigger_record(1));
    facade.close();
  }
  {
    HDF5WriterOptions options;
    options.write_run_context_attributes = false;
    RawDataFileWriter writer(std::make_unique<HDF5WriterBackend>(options), backend_name, make_metadata(), geo_id_map);
    writer.write(make_trigger_record(1));
    writer.close();
  }

  HDF5RawDataFile facade_file(facade_name);
  HDF5RawDataFile backend_file(backend_name);
  auto facade_attributes = facade_file.get_attribute_names();
  auto backend_attributes = backend_file.get_attribute_names();
  BOOST_REQUIRE((std::set<std::string>(facade_attributes.begin(), facade_attributes.end()) ==
                 std::set<std::string>(backend_attributes.begin(), backend_attributes.end())));
  BOOST_REQUIRE(facade_file.get_dataset_paths() == backend_file.get_dataset_paths());
  for (auto const& name : { "filelayout_params", "source_id_geo_id_map", "record_type", "application_name" }) {
    BOOST_REQUIRE_EQUAL(facade_file.get_attribute<std::string>(name), backend_file.get_attribute<std::string>(name));
  }
  BOOST_REQUIRE_EQUAL(facade_file.get_recorded_size(), backend_file.get_recorded_size());
  std::filesystem::remove_all(dir);
}

BOOST_AUTO_TEST_CASE(FacadeKeepsOldSemantics)
{
  const auto dir = make_test_dir("FacadeKeepsOldSemantics");
  const std::string file_name = dir + "/semantics.hdf5";
  {
    auto writer = rawdatafilelibs::create_raw_data_file(file_name, make_metadata(), make_geo_id_map());
    writer->write(make_trigger_record(1));
    writer->close();
  }

  HDF5RawDataFile file(file_name);
  const HDF5RawDataFile::record_id_t rid{ 1, 0 };

  // the record header's SourceID counts as one of the record's SourceIDs, and as one of its subsystem's
  BOOST_REQUIRE_EQUAL(file.get_source_ids(rid).size(), s_fragments.size() + 1);
  BOOST_REQUIRE_EQUAL(file.get_fragment_source_ids(rid).size(), s_fragments.size());
  BOOST_REQUIRE_EQUAL(file.get_source_ids_for_subsystem(rid, SourceID::Subsystem::kTRBuilder).size(), 1);
  BOOST_REQUIRE_EQUAL(file.get_source_ids_for_subsystem(rid, SourceID::Subsystem::kDetectorReadout).size(), 3);
  BOOST_REQUIRE_EQUAL(file.get_source_ids_for_fragment_type(rid, FragmentType::kWIBEth).size(), 2);
  BOOST_REQUIRE_EQUAL(file.get_source_ids_for_subdetector(rid, DetID::Subdetector::kHD_PDS).size(), 1);
  BOOST_REQUIRE_EQUAL(file.get_source_ids_for_fragtype_and_subdetector(rid, "WIBEth", "HD_TPC").size(), 2);

  // GeoIDs: the whole map at file level; per record, only fragments in the record
  BOOST_REQUIRE_EQUAL(file.get_all_geo_ids().size(), 4);
  BOOST_REQUIRE_EQUAL(file.get_geo_ids(rid).size(), 3);
  BOOST_REQUIRE_EQUAL(file.get_geo_ids_for_subdetector(rid, DetID::Subdetector::kHD_TPC).size(), 2);
  const auto geo_id = GeoID(DetID::Subdetector::kHD_PDS, 2, 3, 4).encode();
  BOOST_REQUIRE_EQUAL(file.get_source_id_for_geo_id(rid, geo_id), s_fragments[2].source_id);
  BOOST_REQUIRE_EQUAL(file.get_frag_ptr(rid, geo_id)->get_element_id(), s_fragments[2].source_id);

  // the old error types
  BOOST_REQUIRE_THROW(file.get_frag_ptr(rid, SourceID(SourceID::Subsystem::kTrigger, 77)), SourceIDNotFound);
  BOOST_REQUIRE_THROW(file.get_trh_ptr(5, 0), RecordIDNotFound);
  BOOST_REQUIRE_THROW(file.get_all_timeslice_ids(), WrongRecordTypeRequested);

  auto record = file.get_trigger_record(rid);
  BOOST_REQUIRE_EQUAL(record.get_fragments_ref().size(), s_fragments.size());
  file.close();
  std::filesystem::remove_all(dir);
}

// Only top-level groups named exactly <prefix><digits>[.<digits>] are records; other groups that
// merely contain the prefix, or look like records but aren't, are ignored
BOOST_AUTO_TEST_CASE(OnlyWellFormedRecordNamesAreRecords)
{
  const auto dir = make_test_dir("OnlyWellFormedRecordNamesAreRecords");
  const std::string file_name = dir + "/names.hdf5";
  {
    auto writer = rawdatafilelibs::create_raw_data_file(file_name, make_metadata(), make_geo_id_map());
    writer->write(make_trigger_record(1));
    writer->write(make_trigger_record(2));
    writer->close();
  }
  {
    HighFive::File file(file_name, HighFive::File::ReadWrite);
    for (auto const& name : { "backupTriggerRecord000003",
                              "TriggerRecord",
                              "TriggerRecord00000x",
                              "TriggerRecord000004.",
                              "TriggerRecord000005.0000extra",
                              "TriggerRecord+00006",
                              "TriggerRecord 000007" }) {
      file.createGroup(name);
    }
  }

  auto reader = rawdatafilelibs::open_raw_data_file(file_name);
  BOOST_REQUIRE((reader->get_record_ids() == std::set<RecordID>{ RecordID{ 1 }, RecordID{ 2 } }));
  BOOST_REQUIRE_EQUAL(reader->get_trigger_record(RecordID{ 2 }).get_fragments_ref().size(), s_fragments.size());
  reader->close();
  std::filesystem::remove_all(dir);
}

BOOST_AUTO_TEST_CASE(NeverReusesAnExistingFile)
{
  const auto dir = make_test_dir("NeverReusesAnExistingFile");
  const std::string file_name = dir + "/existing.hdf5";
  {
    HighFive::File stale(file_name + ".writing", HighFive::File::Create);
    stale.createAttribute("marker", std::string("stale"));
  }
  BOOST_REQUIRE_THROW(rawdatafilelibs::create_raw_data_file(file_name, make_metadata(), make_geo_id_map()),
                      rawdatafilelibs::FileOpenFailed);

  // nor an existing file with the final name, without an in-progress suffix
  BOOST_REQUIRE_THROW(
    (void)RawDataFileWriter(std::make_unique<HDF5WriterBackend>(), file_name + ".writing", make_metadata(), {}, ""),
    rawdatafilelibs::FileOpenFailed);

  // and the backend on its own refuses one too
  HDF5WriterBackend backend;
  BOOST_REQUIRE_THROW(backend.open(file_name + ".writing", make_metadata(), {}), std::exception);

  HighFive::File stale(file_name + ".writing", HighFive::File::ReadOnly);
  BOOST_REQUIRE_EQUAL(read_attribute<std::string>(stale, "marker"), "stale");
  std::filesystem::remove_all(dir);
}

BOOST_AUTO_TEST_CASE(LayoutMustMatchRecordType)
{
  const auto dir = make_test_dir("LayoutMustMatchRecordType");
  HDF5WriterOptions options;
  options.file_layout_parameters = default_file_layout_parameters(RecordType::kTimeSlice);
  BOOST_REQUIRE_THROW(
    (void)RawDataFileWriter(std::make_unique<HDF5WriterBackend>(options), dir + "/mismatch.hdf5", make_metadata(), {}),
    rawdatafilelibs::FileOpenFailed);
  BOOST_REQUIRE(!std::filesystem::exists(dir + "/mismatch.hdf5.writing"));
  std::filesystem::remove_all(dir);
}

// Files whose timestamps are strings (layout versions <= 5) are read too
BOOST_AUTO_TEST_CASE(StringTimestamps)
{
  const auto dir = make_test_dir("StringTimestamps");
  const std::string file_name = dir + "/strings.hdf5";
  {
    auto writer = rawdatafilelibs::create_raw_data_file(file_name, make_metadata(), make_geo_id_map());
    writer->close();
  }
  {
    HighFive::File file(file_name, HighFive::File::ReadWrite);
    file.deleteAttribute("creation_timestamp");
    file.createAttribute("creation_timestamp", std::string("1600000000456"));
  }
  auto reader = rawdatafilelibs::open_raw_data_file(file_name);
  BOOST_REQUIRE_EQUAL(reader->get_metadata().creation_timestamp_ms, 1600000000456);
  reader->close();
  std::filesystem::remove_all(dir);
}

BOOST_AUTO_TEST_SUITE_END()
