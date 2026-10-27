cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
project(hdf5libs VERSION 3.6.2)

find_package(daq-cmake REQUIRED)

daq_setup_environment()

find_package(logging REQUIRED)
find_package(ers REQUIRED)
find_package(HighFive REQUIRED)
find_package(appmodel REQUIRED)
find_package(daqdataformats REQUIRED)
find_package(rawdatafilelibs REQUIRED)
find_package(detdataformats REQUIRED)
find_package(trgdataformats REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(Boost COMPONENTS iostreams unit_test_framework REQUIRED)

##############################################################################
# Main library
daq_add_library (HDF5FileLayout.cpp HDF5SourceIDHandler.cpp HDF5Backend.cpp HDF5RawDataFile.cpp LINK_LIBRARIES stdc++fs ers::ers HighFive daqdataformats::daqdataformats detdataformats::detdataformats trgdataformats::trgdataformats rawdatafilelibs::rawdatafilelibs appmodel::appmodel logging::logging nlohmann_json::nlohmann_json)

##############################################################################
# Plugins: the HDF5 backend for rawdatafilelibs, for files named "*.hdf5"
daq_add_plugin(hdf5 duneRawDataFileBackend LINK_LIBRARIES hdf5libs)

##############################################################################
# Unit tests
daq_add_unit_test( HDF5WriteReadTriggerRecord_test LINK_LIBRARIES hdf5libs )
daq_add_unit_test( HDF5WriteReadTimeSlice_test LINK_LIBRARIES hdf5libs )
daq_add_unit_test( HDF5Backend_test LINK_LIBRARIES hdf5libs )
add_dependencies(HDF5Backend_test hdf5libs_hdf5_duneRawDataFileBackend)

##############################################################################
# Python bindings
daq_add_python_bindings(*.cpp LINK_LIBRARIES daqdataformats::daqdataformats detdataformats::detdataformats)

##############################################################################
# Applications
daq_add_application(HDF5LIBS_TestReader HDF5LIBS_TestReader.cpp TEST LINK_LIBRARIES ${PROJECT_NAME})
daq_add_application(HDF5LIBS_TestWriter HDF5LIBS_TestWriter.cpp TEST LINK_LIBRARIES ${PROJECT_NAME})
daq_add_application(HDF5LIBS_TestDumpRecord HDF5LIBS_TestDumpRecord.cpp TEST LINK_LIBRARIES ${PROJECT_NAME})
daq_add_application(HDF5LIBS_TestRecoverFile HDF5LIBS_TestRecoverFile.cpp TEST LINK_LIBRARIES ${PROJECT_NAME})

# The unit tests and test apps use the deprecated HDF5RawDataFile on purpose (they test it, or
# predate rawdatafilelibs), so they don't warn about it
foreach(target HDF5WriteReadTriggerRecord_test HDF5WriteReadTimeSlice_test HDF5Backend_test
               HDF5LIBS_TestReader HDF5LIBS_TestWriter HDF5LIBS_TestDumpRecord HDF5LIBS_TestRecoverFile)
  if(TARGET ${target})
    target_compile_options(${target} PRIVATE -Wno-deprecated-declarations)
  endif()
endforeach()

daq_install()
