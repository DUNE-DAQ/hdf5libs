/**
 * @file wib.cpp
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "hdf5libs/HDF5RawDataFile.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

#include <string>

namespace py = pybind11;

namespace dunedaq {
namespace hdf5libs {
namespace python {

void
register_hdf5rawdatafile(py::module& m)
{

  py::class_<HDF5RawDataFile>(m, "_HDF5RawDataFile")
    .def(py::init<std::string>())

    .def("get_attribute",
         py::overload_cast<const std::string&>
         (&HDF5RawDataFile::get_attribute<std::string>),
         "Get attribute")

    .def("get_file_name",
         &HDF5RawDataFile::get_file_name,"Get file name")
    .def("get_recorded_size",
         &HDF5RawDataFile::get_recorded_size,"Get recorded size")
    .def("get_record_type",
         &HDF5RawDataFile::get_record_type,"Get record type")
    .def("is_trigger_record_type",
         &HDF5RawDataFile::is_trigger_record_type,"Is record type TriggerRecord")
    .def("is_timeslice_type",
         &HDF5RawDataFile::is_timeslice_type,"Is record type TimeSlice")
    .def("get_version",
         &HDF5RawDataFile::get_version,"FileLayout version number")

    .def("get_dataset_paths",
         &HDF5RawDataFile::get_dataset_paths,
	       "Get all dataset paths under specified top group in file",
	       py::arg("top_level_group_name")="")

    .def("get_all_record_ids",
         &HDF5RawDataFile::get_all_record_ids,"Get all record/sequence number pairs.")
    .def("get_all_trigger_record_ids",
         &HDF5RawDataFile::get_all_trigger_record_ids,"Get all trigger record/sequence number pairs.")
    .def("get_all_timeslice_ids",
         &HDF5RawDataFile::get_all_timeslice_ids,"Get all timeslice/sequence number pairs.")

    .def("get_record_header_dataset_paths",
	       &HDF5RawDataFile::get_record_header_dataset_paths,"Get all paths to record header datasets")
    .def("get_trigger_record_header_dataset_paths",
	       &HDF5RawDataFile::get_trigger_record_header_dataset_paths,"Get all paths to TriggerRecordHeader datasets")
    .def("get_timeslice_header_dataset_paths",
         &HDF5RawDataFile::get_timeslice_header_dataset_paths,"Get all paths to TimeSliceHeader datasets")

    .def("get_record_header_dataset_path",
         py::overload_cast<const HDF5RawDataFile::record_id_t&>(&HDF5RawDataFile::get_record_header_dataset_path),
         "Get record header path for record id")
    .def("get_record_header_dataset_path",
         py::overload_cast<const uint64_t,const daqdataformats::sequence_number_t> //NOLINT(build/unsigned)
         (&HDF5RawDataFile::get_record_header_dataset_path),
         "Get record header path for record number and sequence number",
         py::arg("rec_num"),py::arg("seq_num")=0)

    .def("get_trigger_record_header_dataset_path",
         py::overload_cast<const HDF5RawDataFile::record_id_t&>(&HDF5RawDataFile::get_trigger_record_header_dataset_path),
         "Get record header path for trigger record id")
    .def("get_trigger_record_header_dataset_path",
         py::overload_cast<const daqdataformats::trigger_number_t,const daqdataformats::sequence_number_t>
         (&HDF5RawDataFile::get_trigger_record_header_dataset_path),
         "Get record header path for trigger record number and sequence number",
         py::arg("trig_num"),py::arg("seq_num")=0)

    .def("get_timeslice_header_dataset_path",
         py::overload_cast<const HDF5RawDataFile::record_id_t&>(&HDF5RawDataFile::get_timeslice_header_dataset_path),
         "Get record header path for timeslice record id")
    .def("get_timeslice_header_dataset_path",
         py::overload_cast<const daqdataformats::timeslice_number_t>
         (&HDF5RawDataFile::get_timeslice_header_dataset_path),
         "Get record header path for timeslice number")

    .def("get_all_fragment_dataset_paths",
         &HDF5RawDataFile::get_all_fragment_dataset_paths,
	       "Get all paths to Fragment datasets")
    .def("get_fragment_dataset_paths",
         py::overload_cast<const HDF5RawDataFile::record_id_t&>(&HDF5RawDataFile::get_fragment_dataset_paths),
         "Get fragment datasets for record ID")
    .def("get_fragment_dataset_paths",
         py::overload_cast<const uint64_t,const daqdataformats::sequence_number_t> //NOLINT(build/unsigned)
         (&HDF5RawDataFile::get_fragment_dataset_paths),
         "Get fragment datasets for record number and sequence number",
         py::arg("rec_num"),py::arg("seq_num")=0)
    .def("get_fragment_dataset_paths",
         py::overload_cast<const HDF5RawDataFile::record_id_t&,const daqdataformats::SourceID::Subsystem>
         (&HDF5RawDataFile::get_fragment_dataset_paths),
         "Get fragment datasets for record ID and SourceID Subsystem")
    .def("get_fragment_dataset_paths",
         py::overload_cast<const HDF5RawDataFile::record_id_t&,const std::string&>
         (&HDF5RawDataFile::get_fragment_dataset_paths),
         "Get fragment datasets for record ID and SourceID Subsystem string")
#if 0
    .def("get_fragment_dataset_paths",
         py::overload_cast<const daqdataformats::SourceID::Subsystem>(&HDF5RawDataFile::get_fragment_dataset_paths),
         "Get fragment datasets for SourceID Subsystem")
    .def("get_fragment_dataset_paths",
         py::overload_cast<const std::string&>(&HDF5RawDataFile::get_fragment_dataset_paths),
         "Get fragment datasets for SourceID Subsystem string")
    .def("get_fragment_dataset_paths",
         py::overload_cast<const daqdataformats::SourceID>(&HDF5RawDataFile::get_fragment_dataset_paths),
         "Get fragment datasets for SourceID")
    .def("get_fragment_dataset_paths",
         py::overload_cast<const daqdataformats::SourceID::Subsystem,const uint32_t> //NOLINT(build/unsigned)
         (&HDF5RawDataFile::get_fragment_dataset_paths),
         "Get fragment datasets for SourceID Subsystem")
    .def("get_fragment_dataset_paths",
         py::overload_cast<const std::string&,const uint32_t> //NOLINT(build/unsigned)
         (&HDF5RawDataFile::get_fragment_dataset_paths),
         "Get fragment datasets for SourceID Subsystem string")
#endif
    .def("get_geo_ids",
         py::overload_cast<const HDF5RawDataFile::record_id_t&>
         (&HDF5RawDataFile::get_geo_ids),
         "Get all GeoIDs in a record id")
    .def("get_geo_ids",
         py::overload_cast<const uint64_t,const daqdataformats::sequence_number_t> //NOLINT(build/unsigned)
         (&HDF5RawDataFile::get_geo_ids),
         "Get all GeoIDs in a record/sequence number")
    .def("get_geo_ids_for_subdetector",
         py::overload_cast<const HDF5RawDataFile::record_id_t&,const detdataformats::DetID::Subdetector>
         (&HDF5RawDataFile::get_geo_ids_for_subdetector),
         "Get all GeoIDs in a record id with the specified Subdetector type")
    .def("get_source_ids",
         py::overload_cast<const HDF5RawDataFile::record_id_t&>
         (&HDF5RawDataFile::get_source_ids),
         "Get all source IDs in a record id")
    .def("get_source_ids",
         py::overload_cast<const uint64_t,const daqdataformats::sequence_number_t> //NOLINT(build/unsigned)
         (&HDF5RawDataFile::get_source_ids),
         "Get all source IDs in a record/sequence number")
#if 0
    .def("get_source_ids",
         py::overload_cast<const HDF5RawDataFile::record_id_t&,const daqdataformats::SourceID::Subsystem>
         (&HDF5RawDataFile::get_source_ids),
         "Get all source IDs in a record id with a given SourceID Subsystem")
    .def("get_source_ids",
         py::overload_cast<const uint64_t,const daqdataformats::sequence_number_t, //NOLINT(build/unsigned)
                           const daqdataformats::SourceID::Subsystem>
         (&HDF5RawDataFile::get_source_ids),
         "Get all source IDs in a record/sequence number with a given SourceID Subsystem")
    .def("get_source_ids",
         py::overload_cast<const HDF5RawDataFile::record_id_t&,const std::string&>
         (&HDF5RawDataFile::get_source_ids),
         "Get all source IDs in a record id with a given SourceID Subsystem string")
    .def("get_source_ids",
         py::overload_cast<const uint64_t,const daqdataformats::sequence_number_t, //NOLINT(build/unsigned)
                           const std::string&>
         (&HDF5RawDataFile::get_source_ids),
         "Get all source IDs in a record/sequence number with a given SourceID Subsystem string")
    .def("get_source_ids",
         py::overload_cast<const daqdataformats::SourceID::Subsystem>
         (&HDF5RawDataFile::get_source_ids),
         "Get all source IDs with a given SourceID Subsystem")
    .def("get_source_ids",
         py::overload_cast<const std::string&>
         (&HDF5RawDataFile::get_source_ids),
         "Get all source IDs with a given SourceID Subsystem string")
#endif

//    .def("get_frag_ptr", py::overload_cast<const std::string & >
//         (&HDF5RawDataFile::get_frag_ptr),
//	       "Get Fragment from dataset")
    .def("get_frag",
         py::overload_cast<const std::string & >
         (&HDF5RawDataFile::get_frag_ptr),
	       "Get Fragment from dataset")
    .def("get_frag",
        py::overload_cast<const uint64_t, //NOLINT(build/unsigned)
                          const daqdataformats::sequence_number_t,
                          const daqdataformats::SourceID&>
        (&HDF5RawDataFile::get_frag_ptr),
        "Get Fragment from record/sequence number and SourceID")
    .def("get_frag",
         py::overload_cast<const HDF5RawDataFile::record_id_t&,
                           const daqdataformats::SourceID&>
         (&HDF5RawDataFile::get_frag_ptr),
         "Get Fragment from record id and SourceID")

    .def("get_frag",
         py::overload_cast<const uint64_t, //NOLINT(build/unsigned)
                           const daqdataformats::sequence_number_t,
                           const daqdataformats::SourceID::Subsystem,
                            const uint32_t> //NOLINT(build/unsigned)
         (&HDF5RawDataFile::get_frag_ptr),
         "Get Fragment from record/sequence number and SourceID elements")
    .def("get_frag",
         py::overload_cast<const HDF5RawDataFile::record_id_t&,
                           const daqdataformats::SourceID::Subsystem,
                            const uint32_t> //NOLINT(build/unsigned)
         (&HDF5RawDataFile::get_frag_ptr),
         "Get Fragment from record id and SourceID elements")

    .def("get_frag",
         py::overload_cast<const uint64_t, //NOLINT(build/unsigned)
                           const daqdataformats::sequence_number_t,
                           const std::string&,
                            const uint32_t> //NOLINT(build/unsigned)
         (&HDF5RawDataFile::get_frag_ptr),
         "Get Fragment from record/sequence number and SourceID elements")
    .def("get_frag",
         py::overload_cast<const HDF5RawDataFile::record_id_t&,
                           const std::string&,
                            const uint32_t> //NOLINT(build/unsigned)
         (&HDF5RawDataFile::get_frag_ptr),
         "Get Fragment from record id and SourceID elements")

    .def("get_frag",
         py::overload_cast<const HDF5RawDataFile::record_id_t&,
                           const uint64_t> //NOLINT(build/unsigned)
         (&HDF5RawDataFile::get_frag_ptr),
         "Get Fragment from record id and GeoID")
    .def("get_frag",
        py::overload_cast<const uint64_t, //NOLINT(build/unsigned)
                          const daqdataformats::sequence_number_t,
                          const uint64_t> //NOLINT(build/unsigned)
        (&HDF5RawDataFile::get_frag_ptr),
        "Get Fragment from record/sequence number and GeoID")

//    .def("get_trh_ptr",
//         py::overload_cast<const std::string & >(&HDF5RawDataFile::get_trh_ptr),
//	       "Get TriggerRecordHeader from datset")
    .def("get_trh",
         py::overload_cast<const std::string & >
         (&HDF5RawDataFile::get_trh_ptr),
	       "Get TriggerRecordHeader from dataset")
     .def("get_trh",
          py::overload_cast<const HDF5RawDataFile::record_id_t&>
          (&HDF5RawDataFile::get_trh_ptr),
     	    "Get TriggerRecordHeader from record id")
     .def("get_trh",
          py::overload_cast<const daqdataformats::trigger_number_t,daqdataformats::sequence_number_t>
          (&HDF5RawDataFile::get_trh_ptr),
          "Get TriggerRecordHeader from record/sequence number")
//    .def("get_tsh_ptr", py::overload_cast<const std::string & >(&HDF5RawDataFile::get_tsh_ptr),
//	 "Get TimeSliceHeader from datset")
    .def("get_tsh",
         py::overload_cast<const std::string & >
         (&HDF5RawDataFile::get_tsh_ptr),
	       "Get TimeSliceHeader from datset")
    .def("get_tsh",
         py::overload_cast<const HDF5RawDataFile::record_id_t&>
         (&HDF5RawDataFile::get_tsh_ptr),
         "Get TimeSliceHeader from record id")
    .def("get_tsh",
         py::overload_cast<const daqdataformats::timeslice_number_t> //NOLINT(build/unsigned)
         (&HDF5RawDataFile::get_tsh_ptr),
         "Get TimeSliceHeader from timeslince number")

    .def("get_trigger_record",
         py::overload_cast<const HDF5RawDataFile::record_id_t&>
         (&HDF5RawDataFile::get_trigger_record),
         "Get TriggerRecord object from record id")
    .def("get_trigger_record",
         py::overload_cast<const daqdataformats::trigger_number_t,
                           const daqdataformats::sequence_number_t>
         (&HDF5RawDataFile::get_trigger_record),
         "Get TriggerRecord object from record/sequence number")
    .def("get_timeslice",
         py::overload_cast<const HDF5RawDataFile::record_id_t&>
         (&HDF5RawDataFile::get_timeslice),
         "Get TimeSlice object from record id")
    .def("get_timeslice",
         py::overload_cast<const daqdataformats::timeslice_number_t>
         (&HDF5RawDataFile::get_timeslice),
         "Get TimeSlice object from timeslice number")
    ;

} //NOLINT

} // namespace python
} // namespace hdf5libs
} // namespace dunedaq
