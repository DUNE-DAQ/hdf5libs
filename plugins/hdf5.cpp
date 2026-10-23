/**
 * @file hdf5.cpp The HDF5 backends as a rawdatafilelibs plugin, for files named "*.hdf5"
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "hdf5libs/HDF5Backend.hpp"

DEFINE_DUNE_RAW_DATA_FILE_BACKEND(dunedaq::hdf5libs::HDF5ReaderBackend, dunedaq::hdf5libs::HDF5WriterBackend)
