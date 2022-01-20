/**
 * @file module.cpp
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>



namespace py = pybind11;

namespace dunedaq {
namespace hdf5libs {
namespace python {

extern void register_daqdecoder(py::module &);
extern void register_sspdecoder(py::module &);



PYBIND11_MODULE(_daq_hdf5libs_py, m) {

    m.doc() = "c++ implementation of the dunedaq hdf5libs modules"; // optional module docstring

    register_daqdecoder(m);
    register_sspdecoder(m);
}

} // namespace python
} // namespace hdf5libs
} // namespace dunedaq
