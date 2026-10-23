/**
 * @file hdf5backend.cpp Python bindings for the HDF5 backend's HDF5-only (expert) functions.
 * rawdatafilelibs' RawDataFileReader.get_backend() returns an HDF5ReaderBackend for HDF5 files.
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "hdf5libs/HDF5Backend.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <string>

namespace py = pybind11;

namespace dunedaq {
namespace hdf5libs {
namespace python {

void
register_hdf5backend(py::module& m)
{
  // the base classes are registered by rawdatafilelibs
  py::module_::import("rawdatafilelibs._daq_rawdatafilelibs_py");

  m.attr("s_current_file_layout_version") = s_current_file_layout_version;
  m.attr("s_last_legacy_file_layout_version") = s_last_legacy_file_layout_version;

  py::class_<HDF5ReaderBackend, rawdatafilelibs::ReaderBackend>(m, "HDF5ReaderBackend")
    .def("get_file_layout_version", &HDF5ReaderBackend::get_file_layout_version)
    .def("is_legacy_layout", &HDF5ReaderBackend::is_legacy_layout)
    .def("get_record_type_name", &HDF5ReaderBackend::get_record_type_name)
    .def("get_dataset_path",
         &HDF5ReaderBackend::get_dataset_path,
         py::arg("record_id"),
         py::arg("source_id"),
         "Path of the dataset holding a record's header (header SourceID) or a fragment")
    .def("get_dataset_paths",
         &HDF5ReaderBackend::get_dataset_paths,
         py::arg("group_path") = "",
         "All dataset paths below a group (default: the whole file)")
    .def(
      "read_dataset",
      [](HDF5ReaderBackend& self, const std::string& dataset_path) {
        auto buffer = self.read_dataset(dataset_path);
        return py::bytes(buffer.data.get(), buffer.size);
      },
      py::arg("dataset_path"),
      "The bytes of a dataset, by path");

  py::class_<HDF5WriterBackend, rawdatafilelibs::WriterBackend>(m, "HDF5WriterBackend")
    .def("get_file_layout_version", [](const HDF5WriterBackend& self) { return self.get_file_layout().get_version(); });
}

} // namespace python
} // namespace hdf5libs
} // namespace dunedaq
