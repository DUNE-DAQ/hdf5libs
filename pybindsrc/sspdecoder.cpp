/**
 * @file sspdecoder.cpp
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "hdf5libs/SSPDecoder.hpp"
//#include "hdf5libs/utils.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

namespace dunedaq {
namespace hdf5libs {
namespace python {

void
register_sspdecoder(py::module& m)
{

    py::class_<SSPDecoder>(m, "SSPDecoder")
        .def(py::init<std::string, const unsigned&>())
        .def("get_frag_size", &SSPDecoder::get_frag_size)
        .def("get_frag_header_size", &SSPDecoder::get_frag_header_size)
        .def("get_module_channel_id", &SSPDecoder::get_module_channel_id)
        .def("get_frag_timestamp", &SSPDecoder::get_frag_timestamp)
        .def("get_nADC", &SSPDecoder::get_nADC)
        .def("get_ssp_frames", &SSPDecoder::get_ssp_frames)
    ;
    
}

} // namespace python
} // namespace hdf5libs
} // namespace dunedaq
