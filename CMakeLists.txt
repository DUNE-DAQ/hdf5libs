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

#add_library(DAQDecoder apps/daq_decoder.cpp)
#target_link_libraries(DAQDecoder ers::ers HighFive dataformats::dataformats logging::logging)
daq_add_library (DAQDecoder.cpp LINK_LIBRARIES ers::ers HighFive dataformats::dataformats logging::logging )

# Example of a simple data browser
#add_executable(data_file_browser src/reader.cpp )
#target_link_libraries(data_file_browser HighFive dataformats::dataformats)


# Demo
daq_add_application(demo demo.cpp LINK_LIBRARIES DAQDecoder)

#daq_install()
