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

namespace py = pybind11;

namespace dunedaq {
namespace hdf5libs {
namespace python {

void
register_hdf5rawdatafile(py::module& m)
{

  py::class_<HDF5RawDataFile>(m, "HDF5RawDataFile")
    .def(py::init<std::string>())
    .def("get_datasets", &HDF5RawDataFile::get_datasets)
    .def("get_fragments", &HDF5RawDataFile::get_fragments)
    .def("get_trh", &HDF5RawDataFile::get_trh)
    .def("get_frag_ptr", &HDF5RawDataFile::get_frag_ptr)
    .def("get_trh_ptr", &HDF5RawDataFile::get_trh_ptr)

  ;

}

} // namespace python
} // namespace hdf5libs
} // namespace dunedaq
