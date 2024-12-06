/**
 * @file HDF5TestUtils.hpp
 *
 * Utility functions for use only in HDF5 unit tests
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef HDF5LIBS_TESTUTILITIES_HPP_
#define HDF5LIBS_TESTUTILITIES_HPP_

#include "hdf5libs/HDF5RawDataFile.hpp"

#include <vector>
#include <string>
#include <filesystem>
#include <regex>

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

uint64_t
encode_geoid(int det_id, int crate_id, int slot_id, int stream_id)
{
  return (static_cast<uint64_t>(stream_id) << 48) | (static_cast<uint64_t>(slot_id) << 32) |
         (static_cast<uint64_t>(crate_id) << 16) | det_id;
}

HDF5SourceIDHandler::source_id_geo_id_map_t
create_srcid_geoid_map()
{
  HDF5SourceIDHandler::source_id_geo_id_map_t map;

  dunedaq::daqdataformats::SourceID sid;
  sid.subsystem = dunedaq::daqdataformats::SourceID::Subsystem::kDetectorReadout;

  sid.id = 0;
  map[sid].push_back(encode_geoid(3, 1, 0, 0));

  sid.id = 1;
  map[sid].push_back(encode_geoid(3, 1, 0, 1));

  sid.id = 3;
  map[sid].push_back(encode_geoid(3, 1, 1, 0));

  sid.id = 4;
  map[sid].push_back(encode_geoid(3, 1, 1, 1));

  sid.id = 4;
  map[sid].push_back(encode_geoid(2, 1, 0, 0));

  sid.id = 5;
  map[sid].push_back(encode_geoid(2, 1, 0, 1));

  sid.id = 6;
  map[sid].push_back(encode_geoid(2, 1, 1, 0));

  sid.id = 7;
  map[sid].push_back(encode_geoid(2, 1, 1, 1));

  return map;
}

// Template function to be used in unit tests
template<typename DAQDataType>
HDF5FileLayoutParameters
create_file_layout_params()
{
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
  layout_params.record_name_prefix = "DefaultSlice";
  layout_params.digits_for_record_number = 6;
  layout_params.digits_for_sequence_number = 0;
  layout_params.record_header_dataset_name = "DefaultSliceHeader";

  return layout_params;
}

// TriggerRecord specialization
template<>
HDF5FileLayoutParameters create_file_layout_params<dunedaq::daqdataformats::TriggerRecord>() {
  dunedaq::hdf5libs::HDF5FileLayoutParameters layout_params = create_file_layout_params<void>();
  layout_params.record_name_prefix = "TriggerRecord";
  layout_params.digits_for_sequence_number = 4;
  layout_params.record_header_dataset_name = "TriggerRecordHeader";
  return layout_params;
}

// TimeSlice specialization
template<>
HDF5FileLayoutParameters create_file_layout_params<dunedaq::daqdataformats::TimeSlice>() {
  dunedaq::hdf5libs::HDF5FileLayoutParameters layout_params = create_file_layout_params<void>();
  layout_params.record_name_prefix = "TimeSlice";
  layout_params.digits_for_sequence_number = 0;
  layout_params.record_header_dataset_name = "TimeSliceHeader";

  return layout_params;
}

} // namespace hdf5libs

} // namespace dunedaq

#endif // HDF5LIBS_TESTUTILITIES_HPP_
