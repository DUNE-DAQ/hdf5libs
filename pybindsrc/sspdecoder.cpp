/**
 * @file sspdecoder.cpp
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "hdf5libs/SSPDecoder.hpp"

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
        .def_property_readonly("module_id", &SSPDecoder::get_module_id)
        .def_property_readonly("channel_id", &SSPDecoder::get_channel_id)
        .def_property_readonly("frag_timestamp", &SSPDecoder::get_frag_timestamp)
        .def_property_readonly("ssp_frames", &SSPDecoder::get_ssp_frames)
        .def_property_readonly("peaksum", &SSPDecoder::get_peaksum)
        .def_property_readonly("peaktime", &SSPDecoder::get_peaktime)
        .def_property_readonly("prerise", &SSPDecoder::get_prerise)
        .def_property_readonly("intsum", &SSPDecoder::get_intsum)
        .def_property_readonly("baseline", &SSPDecoder::get_baseline)
        .def_property_readonly("baselinesum", &SSPDecoder::get_baselinesum)
        //.def_property_readonly("cfd_interpol", &SSPDecoder::get_cfd_interpol)
        .def_property_readonly("internal_interpol", &SSPDecoder::get_internal_interpol)
        .def_property_readonly("internal_ts", &SSPDecoder::get_internal_ts)
    ;
    
}

} // namespace python
} // namespace hdf5libs
} // namespace dunedaq
