cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

set(PACKAGE DAQDecoder)
set(PACKAGE_VERSION 0.1)

project(${PACKAGE} VERSION ${PACKAGE_VERSION}
	DESCRIPTION "Decoder for the the DUNE-DAQ HDF5 files"
        LANGUAGES C CXX)

# Print version
message(STATUS "Building ${PACKAGE} version ${PACKAGE_VERSION}")


find_package(daq-cmake REQUIRED)
daq_setup_environment()



find_package(logging REQUIRED)
find_package(HighFive REQUIRED)
find_package(dataformats REQUIRED)

daq_add_library (DAQDecoder.cpp LINK_LIBRARIES ers::ers HighFive dataformats::dataformats logging::logging )


# Demo
daq_add_application(hdf5_demo_reader demo.cpp LINK_LIBRARIES DAQDecoder)

#daq_install()
