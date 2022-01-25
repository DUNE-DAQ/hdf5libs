cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
project(hdf5libs VERSION 1.0.1)

find_package(daq-cmake REQUIRED)
find_package(logging REQUIRED)
find_package(HighFive REQUIRED)
find_package(daqdataformats REQUIRED)
find_package(detdataformats REQUIRED)
find_package(detchannelmaps REQUIRED)
find_package(cetlib REQUIRED)   # Uses the daq-buildtools/cmake/Findcetlib.cmake

daq_setup_environment()

##############################################################################
# Main library
daq_add_library (HDF5RawDataFile.cpp StorageKey.cpp LINK_LIBRARIES stdc++fs ers::ers HighFive daqdataformats::daqdataformats logging::logging ${CETLIB} ${CETLIB_EXCEPT})

##############################################################################
# Python bindings
#daq_add_python_bindings(*.cpp LINK_LIBRARIES ${PROJECT_NAME})

##############################################################################
# Applications
daq_add_application(example example.cpp LINK_LIBRARIES ${PROJECT_NAME})

daq_install()
