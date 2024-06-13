/**
 * @file HDF5FileLayoutParameters.hpp
 *
 * Utilities for converting between different representations of the file layout parameters
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef HDF5LIBS_INCLUDE_HDF5LIBS_HDF5FILELAYOUTPARAMETERS_HPP_
#define HDF5LIBS_INCLUDE_HDF5LIBS_HDF5FILELAYOUTPARAMETERS_HPP_

#include "appmodel/HDF5FileLayoutParams.hpp"
#include "appmodel/HDF5PathParams.hpp"

#include "nlohmann/json.hpp"

#include <vector>

namespace dunedaq {

namespace hdf5libs {

struct HDF5PathParameters
{
  std::string detector_group_type = "unspecified";
  std::string detector_group_name = "unspecified";
  std::string element_name_prefix = "Element";
  int32_t digits_for_element_number = 5;

  HDF5PathParameters() = default;
  HDF5PathParameters(appmodel::HDF5PathParams const* from_conf)
  {
    detector_group_type = from_conf->get_detector_group_type();
    detector_group_name = from_conf->get_detector_group_name();
    element_name_prefix = from_conf->get_element_name_prefix();
    digits_for_element_number = from_conf->get_digits_for_element_number();
  }
  HDF5PathParameters(nlohmann::json from_json)
  {
    detector_group_type = from_json["detector_group_type"];
    detector_group_name = from_json["detector_group_name"];
    element_name_prefix = from_json["element_name_prefix"];
    digits_for_element_number = from_json["digits_for_element_number"];
  }

  nlohmann::json to_json()
  {
    nlohmann::json output;

    output["detector_group_type"] = detector_group_type;
    output["detector_group_name"] = detector_group_name;
    output["element_name_prefix"] = element_name_prefix;
    output["digits_for_element_number"] = digits_for_element_number;

    return output;
  }
};

struct HDF5FileLayoutParameters
{
  std::string record_name_prefix = "TriggerRecord";
  int32_t digits_for_record_number = 6;
  int32_t digits_for_sequence_number = 4;
  std::string record_header_dataset_name = "TriggerRecordHeader";
  std::string raw_data_group_name = "RawData";
  std::string view_group_name = "Views";
  std::vector<HDF5PathParameters> path_params_list;

  HDF5FileLayoutParameters() = default;
  HDF5FileLayoutParameters(appmodel::HDF5FileLayoutParams const* from_conf) {
    record_name_prefix = from_conf->get_record_name_prefix();
    digits_for_record_number = from_conf->get_digits_for_record_number();
    digits_for_sequence_number = from_conf->get_digits_for_sequence_number();
    record_header_dataset_name = from_conf->get_record_header_dataset_name();
    raw_data_group_name = from_conf->get_raw_data_group_name();
    view_group_name = from_conf->get_view_group_name();

    for (auto& pp : from_conf->get_path_params_list()) {
      path_params_list.emplace_back(pp);
    } 

  }
  HDF5FileLayoutParameters(nlohmann::json from_json)
  {
    record_name_prefix = from_json["record_name_prefix"];
    digits_for_record_number = from_json["digits_for_record_number"];
    digits_for_sequence_number = from_json["digits_for_sequence_number"];
    record_header_dataset_name = from_json["record_header_dataset_name"];
    raw_data_group_name = from_json["raw_data_group_name"];
    view_group_name = from_json["view_group_name"];

    for (auto& pp : from_json["path_param_list"]) {
      path_params_list.emplace_back(pp);
    } 
  
  }

  nlohmann::json to_json() {

    nlohmann::json output;

    output["record_name_prefix"] = record_name_prefix;
    output["digits_for_record_number"] = digits_for_record_number;
    output["digits_for_sequence_number"] = digits_for_sequence_number;
    output["record_header_dataset_name"] = record_header_dataset_name;
    output["raw_data_group_name"] = raw_data_group_name;
    output["view_group_name"] = view_group_name;
    output["path_param_list"] = nlohmann::json::array();

    for (auto& pp : path_params_list) {
      output["path_param_list"].push_back(pp.to_json());
    }

    return output;
  }
};

} // namespace hdf5libs

} // namespace dunedaq

#endif // HDF5LIBS_INCLUDE_HDF5LIBS_HDF5FILELAYOUTPARAMETERS_HPP_
