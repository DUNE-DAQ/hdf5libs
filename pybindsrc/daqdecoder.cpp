/**
 * @file wib.cpp
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "hdf5libs/DAQDecoder.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

namespace dunedaq {
namespace hdf5libs {
namespace python {

void
register_daqdecoder(py::module& m)
{

  py::class_<DAQDecoder>(m, "DAQDecoder")
    .def(py::init<std::string>())
    .def("get_datasets", &DAQDecoder::get_datasets)
    .def("get_fragments", &DAQDecoder::get_fragments)
    .def("get_trh", &DAQDecoder::get_trh)
    .def("get_frag_ptr", &DAQDecoder::get_frag_ptr)
    .def("get_trh_ptr", &DAQDecoder::get_trh_ptr)

  ;

}

} // namespace python
} // namespace hdf5libs
} // namespace dunedaq
