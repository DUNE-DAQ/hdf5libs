/**
 * @file HDF5FileLayout.hpp
 *
 * FileLayoutObject that is used to describe/provide instructions for
 * organizing DUNE DAQ HDF5 files.
 *
 * WK 02.02.2022 -- Future could make a FileLayout descriptor per SystemType
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef HDF5LIBS_INCLUDE_HDF5LIBS_HDF5FILELAYOUT_HPP_
#define HDF5LIBS_INCLUDE_HDF5LIBS_HDF5FILELAYOUT_HPP_

#include "hdf5libs/hdf5filelayout/Structs.hpp"

#include "daqdataformats/Fragment.hpp"
#include "daqdataformats/GeoID.hpp"
#include "daqdataformats/TriggerRecordHeader.hpp"

#include "nlohmann/json.hpp"

#include <cstdint>
#include <iomanip>
#include <limits>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace dunedaq {
namespace hdf5libs {

class HDF5FileLayout
{
public:
  /**
   * @brief Constructor from json conf, used in DataWriter. Version always most recent.
   */
  explicit HDF5FileLayout(hdf5filelayout::FileLayoutParams conf, uint32_t version = 1) // NOLINT(build/unsigned)
    : m_conf_params(conf)
    , m_version(version)
  {
    if (m_version == 0)
      m_conf_params = get_v0_file_layout_params();

    fill_path_params_map(m_conf_params);
  }

  uint32_t get_version() const { return m_version; } // NOLINT(build/unsigned)

  std::string get_trigger_record_name_prefix() const { return m_conf_params.trigger_record_name_prefix; }
  int get_digits_for_trigger_number() const { return m_conf_params.digits_for_trigger_number; }
  int get_digits_for_sequence_number() const { return m_conf_params.digits_for_sequence_number; }
  std::string get_trigger_header_dataset_name() const { return m_conf_params.trigger_record_header_dataset_name; }

  std::map<daqdataformats::GeoID::SystemType, hdf5filelayout::PathParams> get_path_params_map() const
  {
    return m_path_params_map;
  }
  hdf5filelayout::PathParams get_path_params(daqdataformats::GeoID::SystemType type) const
  {
    return m_path_params_map.at(type);
  }

  hdf5filelayout::FileLayoutParams get_file_layout_params() const { return m_conf_params; }

  /**
   * @brief get string for Trigger number
   */
  std::string get_trigger_number_string(daqdataformats::trigger_number_t trig_num,
                                        daqdataformats::sequence_number_t seq_num = 0) const
  {

    std::ostringstream trigger_number_string;
    trigger_number_string << m_conf_params.trigger_record_name_prefix
                          << std::setw(m_conf_params.digits_for_trigger_number) << std::setfill('0') << trig_num;

    if (m_conf_params.digits_for_sequence_number > 0) {
      trigger_number_string << "." << std::setw(m_conf_params.digits_for_sequence_number) << std::setfill('0')
                            << seq_num;
    }

    return trigger_number_string.str();
  }

  /**
   * @brief get the correct path for the TriggerRecordHeader
   */
  std::vector<std::string> get_path_elements(const daqdataformats::TriggerRecordHeader& trh) const
  {

    std::vector<std::string> path_elements;

    // first the Trigger string
    path_elements.push_back(get_trigger_number_string(trh.get_trigger_number(), trh.get_sequence_number()));

    // then the TriggerRecordHeader dataset name
    path_elements.push_back(m_conf_params.trigger_record_header_dataset_name);

    return path_elements;
  }

  /**
   * @brief get the correct path for the Fragment
   */
  std::vector<std::string> get_path_elements(const daqdataformats::FragmentHeader& fh) const
  {

    std::vector<std::string> path_elements;

    // first the Trigger string
    path_elements.push_back(get_trigger_number_string(fh.trigger_number, fh.sequence_number));

    // then get the path params from our file layout for this type
    auto const& path_params = get_path_params(fh.element_id.system_type);

    // next is the detector group name
    path_elements.push_back(path_params.detector_group_name);

    // then the region
    std::ostringstream region_string;
    region_string << path_params.region_name_prefix << std::setw(path_params.digits_for_region_number)
                  << std::setfill('0') << fh.element_id.region_id;
    path_elements.push_back(region_string.str());

    // finally the element
    std::ostringstream element_string;
    element_string << path_params.element_name_prefix << std::setw(path_params.digits_for_element_number)
                   << std::setfill('0') << fh.element_id.element_id;
    path_elements.push_back(element_string.str());

    return path_elements;
  }

  /**
   * @brief get the full path for a TriggerRecordHeader dataset based on trig/seq number
   */
  std::string get_trigger_record_header_path(daqdataformats::trigger_number_t trig_num,
                                             daqdataformats::sequence_number_t seq_num = 0) const
  {
    return std::string(get_trigger_number_string(trig_num, seq_num) + "/" +
                       m_conf_params.trigger_record_header_dataset_name);
  }

  /**
   * @brief get the full path for a Fragment dataset based on trig/seq number and element ID
   */
  std::string get_fragment_path(daqdataformats::trigger_number_t trig_num,
                                daqdataformats::GeoID element_id,
                                daqdataformats::sequence_number_t seq_num = 0) const
  {

    auto const& path_params = get_path_params(element_id.system_type);

    std::ostringstream path_string;
    path_string << get_trigger_number_string(trig_num, seq_num) << "/" << path_params.detector_group_name << "/"
                << path_params.region_name_prefix << std::setw(path_params.digits_for_region_number)
                << std::setfill('0') << element_id.region_id << "/" << path_params.element_name_prefix
                << std::setw(path_params.digits_for_element_number) << std::setfill('0') << element_id.element_id;
    return path_string.str();
  }

  /**
   * @brief get the full path for a Fragment dataset based on trig/seq number, give element_id pieces
   */
  std::string get_fragment_path(daqdataformats::trigger_number_t trig_num,
                                daqdataformats::GeoID::SystemType type,
                                uint16_t region_id, // NOLINT(build/unsigned)
                                uint32_t element_id, // NOLINT(build/unsigned)
                                daqdataformats::sequence_number_t seq_num = 0) const
  {
    daqdataformats::GeoID gid{ type, region_id, element_id };
    return get_fragment_path(trig_num, gid, seq_num);
  }

  /**
   * @brief get the full path for a Fragment dataset based on trig/seq number, give element_id pieces
   */
  std::string get_fragment_path(daqdataformats::trigger_number_t trig_num,
                                std::string typestring,
                                uint16_t region_id, // NOLINT(build/unsigned)
                                uint32_t element_id, // NOLINT(build/unsigned)
                                daqdataformats::sequence_number_t seq_num = 0) const
  {
    daqdataformats::GeoID gid{ daqdataformats::GeoID::string_to_system_type(typestring), region_id, element_id };
    return get_fragment_path(trig_num, gid, seq_num);
  }

  /**
   * @brief get the path for a Fragment type group based on trig/seq number and type
   */
  std::string get_fragment_type_path(daqdataformats::trigger_number_t trig_num,
                                     daqdataformats::GeoID::SystemType type,
                                     daqdataformats::sequence_number_t seq_num = 0) const
  {
    auto const& path_params = get_path_params(type);

    std::ostringstream path_string;
    path_string << get_trigger_number_string(trig_num, seq_num) << "/" << path_params.detector_group_name;
    return path_string.str();
  }

  /**
   * @brief get the path for a Fragment type group based on trig/seq number and type
   */
  std::string get_fragment_type_path(daqdataformats::trigger_number_t trig_num,
                                     std::string typestring,
                                     daqdataformats::sequence_number_t seq_num = 0) const
  {
    return get_fragment_type_path(trig_num, daqdataformats::GeoID::string_to_system_type(typestring), seq_num);
  }

  /**
   * @brief get the path for a Fragment region group based on trig/seq number,type, and region ID
   */
  std::string get_fragment_region_path(daqdataformats::trigger_number_t trig_num,
                                       daqdataformats::GeoID::SystemType type,
                                       uint16_t region_id, // NOLINT(build/unsigned)
                                       daqdataformats::sequence_number_t seq_num = 0) const
  {
    auto const& path_params = get_path_params(type);

    std::ostringstream path_string;
    path_string << get_trigger_number_string(trig_num, seq_num) << "/" << path_params.detector_group_name << "/"
                << path_params.region_name_prefix << std::setw(path_params.digits_for_region_number)
                << std::setfill('0') << region_id;
    return path_string.str();
  }

  /**
   * @brief get the path for a Fragment region group based on trig/seq number,type, and region ID
   */
  std::string get_fragment_region_path(daqdataformats::trigger_number_t trig_num,
                                       std::string typestring,
                                       uint16_t region_id, // NOLINT(build/unsigned)
                                       daqdataformats::sequence_number_t seq_num = 0) const
  {
    return get_fragment_region_path(
      trig_num, daqdataformats::GeoID::string_to_system_type(typestring), region_id, seq_num);
  }

private:
  /**
   * @brief FileLayout configuration parameters
   */
  hdf5filelayout::FileLayoutParams m_conf_params;

  /**
   * @brief version number
   */
  uint32_t m_version; // NOLINT(build/unsigned)

  /**
   * @brief map translation for GeoID::SystemType to dataset path parameters
   */
  std::map<daqdataformats::GeoID::SystemType, hdf5filelayout::PathParams> m_path_params_map;

  /**
   * @brief Fill path parameters map from FileLayoutParams
   */
  void fill_path_params_map(hdf5filelayout::FileLayoutParams const& flp)
  {
    for (auto const& path_param : flp.path_param_list) {
      auto sys_type = daqdataformats::GeoID::string_to_system_type(path_param.detector_group_type);
      if (sys_type == daqdataformats::GeoID::SystemType::kInvalid)
        continue; // update to make it show an error
      m_path_params_map[sys_type] = path_param;
    }
  }

  /**
   * @brief Version0 FileLayout parameters, for backward compatibility
   */
  hdf5filelayout::FileLayoutParams get_v0_file_layout_params()
  {
    hdf5filelayout::FileLayoutParams flp;
    flp.trigger_record_name_prefix = "TriggerRecord";
    flp.digits_for_trigger_number = 6;
    flp.digits_for_sequence_number = 0;
    flp.trigger_record_header_dataset_name = "TriggerRecordHeader";

    hdf5filelayout::PathParams pp;

    pp.detector_group_type = "TPC";
    pp.detector_group_name = "TPC";
    pp.region_name_prefix = "APA";
    pp.digits_for_region_number = 3;
    pp.element_name_prefix = "Link";
    pp.digits_for_element_number = 2;
    flp.path_param_list.push_back(pp);

    pp.detector_group_type = "PDS";
    pp.detector_group_name = "PDS";
    pp.region_name_prefix = "Region";
    pp.digits_for_region_number = 3;
    pp.element_name_prefix = "Element";
    pp.digits_for_element_number = 2;
    flp.path_param_list.push_back(pp);

    pp.detector_group_type = "NDLArTPC";
    pp.detector_group_name = "NDLArTPC";
    pp.region_name_prefix = "Region";
    pp.digits_for_region_number = 3;
    pp.element_name_prefix = "Element";
    pp.digits_for_element_number = 2;
    flp.path_param_list.push_back(pp);

    pp.detector_group_type = "Trigger";
    pp.detector_group_name = "Trigger";
    pp.region_name_prefix = "Region";
    pp.digits_for_region_number = 3;
    pp.element_name_prefix = "Element";
    pp.digits_for_element_number = 2;
    flp.path_param_list.push_back(pp);

    pp.detector_group_type = "TPC_TP";
    pp.detector_group_name = "TPC";
    pp.region_name_prefix = "APA";
    pp.digits_for_region_number = 3;
    pp.element_name_prefix = "Link";
    pp.digits_for_element_number = 2;
    flp.path_param_list.push_back(pp);

    return flp;
  }
};

} // namespace hdf5libs
} // namespace dunedaq

#endif // HDF5LIBS_INCLUDE_HDF5LIBS_HDF5FILELAYOUT_HPP_
