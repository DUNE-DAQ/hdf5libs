/**
 * @file hdf5backend.cpp Python bindings for the HDF5 backend's HDF5-only (expert) functions.
 * rawdatafilelibs' RawDataFileReader.get_backend() returns an HDF5ReaderBackend for HDF5 files.
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "hdf5libs/HDF5Backend.hpp"

#include <hdf5.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <cstdint>
#include <stdexcept>
#include <string>

namespace py = pybind11;

namespace dunedaq {
namespace hdf5libs {
namespace python {

namespace {
// A scalar file attribute as a Python value of the stored type (str, int or float)
py::object
attribute_value(const HighFive::Attribute& attribute)
{
  auto type = attribute.getDataType();
  switch (type.getClass()) {
    case HighFive::DataTypeClass::String: {
      std::string value;
      attribute.read(value);
      return py::str(value);
    }
    case HighFive::DataTypeClass::Integer:
      if (H5Tget_sign(type.getId()) == H5T_SGN_NONE) {
        uint64_t value = 0; // NOLINT(build/unsigned)
        attribute.read(value);
        return py::int_(value);
      } else {
        int64_t value = 0;
        attribute.read(value);
        return py::int_(value);
      }
    case HighFive::DataTypeClass::Float: {
      double value = 0;
      attribute.read(value);
      return py::float_(value);
    }
    default:
      throw std::runtime_error("attribute " + attribute.getName() + " has a type that isn't supported here");
  }
}
} // namespace

void
register_hdf5backend(py::module& m)
{
  // the base classes are registered by rawdatafilelibs
  py::module_::import("rawdatafilelibs._daq_rawdatafilelibs_py");

  m.attr("s_current_file_layout_version") = s_current_file_layout_version;
  m.attr("s_last_legacy_file_layout_version") = s_last_legacy_file_layout_version;

  py::class_<HDF5ReaderBackend, rawdatafilelibs::ReaderBackend>(m, "HDF5ReaderBackend")
    .def("get_file_layout_version", &HDF5ReaderBackend::get_file_layout_version)
    .def(
      "get_attribute_names",
      [](HDF5ReaderBackend& self) { return self.get_file().listAttributeNames(); },
      "The names of the file-level HDF5 attributes (HDF5-only; prefer FileMetadata where it has the value)")
    .def(
      "has_attribute",
      [](HDF5ReaderBackend& self, const std::string& name) { return self.get_file().hasAttribute(name); },
      py::arg("name"))
    .def(
      "get_attribute",
      [](HDF5ReaderBackend& self, const std::string& name) {
        if (!self.get_file().hasAttribute(name)) {
          throw py::key_error("no file attribute named " + name);
        }
        return attribute_value(self.get_file().getAttribute(name));
      },
      py::arg("name"),
      "A file-level HDF5 attribute's value (str, int or float), e.g. \"file_recovery_timestamp\"; KeyError if absent")
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
