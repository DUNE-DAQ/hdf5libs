/**
 * @file HDF5FileUtils.hpp
 *
 * Utility functions for HDF5 files
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef HDF5LIBS_INCLUDE_HDF5LIBS_HDF5FILEUTILS_HPP_
#define HDF5LIBS_INCLUDE_HDF5LIBS_HDF5FILEUTILS_HPP_

#include "hdf5libs/HDF5RawDataFile.hpp"
#include "hdf5libs/HDF5FileUtils.hpp"

#include <string>
#include <vector>
#include <regex>
#include <filesystem>

namespace dunedaq {

namespace hdf5libs {

std::vector<std::string>
get_files_matching_pattern(const std::string& path, const std::string& pattern)
{
  std::regex regex_search_pattern(pattern);
  std::vector<std::string> file_list;
  for (const auto& entry : std::filesystem::directory_iterator(path)) {
    if (std::regex_match(entry.path().filename().string(), regex_search_pattern)) {
      file_list.push_back(entry.path());
    }
  }
  return file_list;
}

std::vector<std::string>
delete_files_matching_pattern(const std::string& path, const std::string& pattern)
{
  std::regex regex_search_pattern(pattern);
  std::vector<std::string> file_list;
  for (const auto& entry : std::filesystem::directory_iterator(path)) {
    if (std::regex_match(entry.path().filename().string(), regex_search_pattern)) {
      if (std::filesystem::remove(entry.path())) {
        file_list.push_back(entry.path());
      }
    }
  }
  return file_list;
}

// Template function to be specialized for TriggerRecord and TimeSlice
template<typename DAQDataType>
HDF5FileLayoutParameters
create_file_layout_params()
{
  static_assert(sizeof(DAQDataType) == 0, "create_file_layout_params requires either a TriggerRecord or TimeSlice type");

  dunedaq::hdf5libs::HDF5FileLayoutParameters layout_params;
  return layout_params;
}

// TriggerRecord specialization
template<>
HDF5FileLayoutParameters create_file_layout_params<dunedaq::daqdataformats::TriggerRecord>() {
  dunedaq::hdf5libs::HDF5PathParameters params_tpc;
  params_tpc.detector_group_type = "Detector_Readout";
  params_tpc.detector_group_name = "TPC";
  params_tpc.element_name_prefix = "Link";
  params_tpc.digits_for_element_number = 5;

  // dunedaq::hdf5libs::hdf5filelayout::PathParams params_pds;
  // params_pds.detector_group_type = "PDS";
  // params_pds.detector_group_name = "PDS";
  // params_pds.element_name_prefix = "Element";
  // params_pds.digits_for_element_number = 5;

  // note, for unit test json equality checks, 'PDS' needs to come before
  //'TPC', as on reading back the filelayout it looks like it's alphabetical.
  std::vector<dunedaq::hdf5libs::HDF5PathParameters> param_list;
  // param_list.push_back(params_pds);
  param_list.push_back(params_tpc);

  dunedaq::hdf5libs::HDF5FileLayoutParameters layout_params;
  layout_params.path_params_list = param_list;
  layout_params.record_name_prefix = "TriggerRecord";
  layout_params.digits_for_record_number = 6;
  layout_params.digits_for_sequence_number = 4;
  layout_params.record_header_dataset_name = "TriggerRecordHeader";

  return layout_params;
}

// TimeSlice specialization
template<>
HDF5FileLayoutParameters create_file_layout_params<dunedaq::daqdataformats::TimeSlice>() {
  dunedaq::hdf5libs::HDF5PathParameters params_tpc;
  params_tpc.detector_group_type = "Detector_Readout";
  params_tpc.detector_group_name = "TPC";
  params_tpc.element_name_prefix = "Link";
  params_tpc.digits_for_element_number = 5;

  // dunedaq::hdf5libs::hdf5filelayout::PathParams params_pds;
  // params_pds.detector_group_type = "PDS";
  // params_pds.detector_group_name = "PDS";
  // params_pds.element_name_prefix = "Element";
  // params_pds.digits_for_element_number = 5;

  // note, for unit test json equality checks, 'PDS' needs to come before
  //'TPC', as on reading back the filelayout it looks like it's alphabetical.
  std::vector<dunedaq::hdf5libs::HDF5PathParameters> param_list;
  // param_list.push_back(params_pds);
  param_list.push_back(params_tpc);

  dunedaq::hdf5libs::HDF5FileLayoutParameters layout_params;
  layout_params.path_params_list = param_list;
  layout_params.record_name_prefix = "TimeSlice";
  layout_params.digits_for_record_number = 6;
  layout_params.digits_for_sequence_number = 0;
  layout_params.record_header_dataset_name = "TimeSliceHeader";

  return layout_params;
}

} // namespace hdf5libs

} // namespace dunedaq

#endif // HDF5LIBS_INCLUDE_HDF5LIBS_HDF5FILEUTILS_HPP_