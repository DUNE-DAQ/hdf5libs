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

#include <string>

namespace py = pybind11;

namespace dunedaq {
namespace hdf5libs {
namespace python {

void
register_hdf5rawdatafile(py::module& m)
{

  py::class_<HDF5RawDataFile>(m, "HDF5RawDataFile")
    .def(py::init<std::string>())
    .def("get_file_name", &HDF5RawDataFile::get_file_name,
	 "Get file name")
    .def("get_recorded_size", &HDF5RawDataFile::get_recorded_size,
	 "Get recorded size")
    .def("get_dataset_paths", &HDF5RawDataFile::get_dataset_paths,
	 "Get all dataset paths under specified top group in file",
	 py::arg("top_level_group_name")="")
    .def("get_all_record_numbers", &HDF5RawDataFile::get_all_record_numbers,
	 "Get all record numbers in file")
    .def("get_all_trigger_record_numbers", &HDF5RawDataFile::get_all_trigger_record_numbers,
	 "Get all trigger record numbers in file")
    .def("get_trigger_record_header_dataset_paths", 
	 &HDF5RawDataFile::get_trigger_record_header_dataset_paths,
	 "Get all paths to TriggerRecordHeader datasets")
    .def("get_all_fragment_dataset_paths",
	 &HDF5RawDataFile::get_all_fragment_dataset_paths,
	 "Get all paths to Fragment datasets")
    .def("get_frag_ptr", py::overload_cast<const std::string & >(&HDF5RawDataFile::get_frag_ptr),
	 "Get Fragment from dataset")
    .def("get_frag", py::overload_cast<const std::string & >(&HDF5RawDataFile::get_frag_ptr),
	 "Get Fragment from dataset")
    .def("get_trh_ptr", py::overload_cast<const std::string & >(&HDF5RawDataFile::get_trh_ptr),
	 "Get TriggerRecordHeader from datset")
    .def("get_trh", py::overload_cast<const std::string & >(&HDF5RawDataFile::get_trh_ptr),
	 "Get TriggerRecordHeader from datset")

    ;
}

} // namespace python
} // namespace hdf5libs
} // namespace dunedaq
