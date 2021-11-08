cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
project(hdf5libs VERSION 1.0.0)

find_package(daq-cmake REQUIRED)
find_package(logging REQUIRED)
find_package(HighFive REQUIRED)
find_package(daqdataformats REQUIRED)
find_package(detdataformats REQUIRED)
find_package(detchannelmaps REQUIRED)

daq_setup_environment()

##############################################################################
# Main library
daq_add_library (DAQDecoder.cpp LINK_LIBRARIES ers::ers HighFive daqdataformats::daqdataformats logging::logging detdataformats::detdataformats detchannelmaps::detchannelmaps)

##############################################################################
# Python bindings
daq_add_python_bindings(*.cpp LINK_LIBRARIES ${PROJECT_NAME})

##############################################################################
# Applications
daq_add_application(hdf5_demo_tpc_decoder demo_tpc_decoder.cpp LINK_LIBRARIES hdf5libs)
#daq_add_application(hdf5_demo_pd_decoder demo_pd_decoder.cpp LINK_LIBRARIES ${PROJECT_NAME} detdataformats)

daq_install()
