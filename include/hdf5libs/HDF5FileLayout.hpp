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

#include "daqdataformats/GeoID.hpp"

#include "nlohmann/json.hpp"

#include <cstdint>
#include <limits>
#include <map>
#include <memory>
#include <string>

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

    // m_conf_params = conf.get<hdf5filelayout::FileLayoutParams>();
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
};

} // namespace hdf5libs
} // namespace dunedaq

#endif // HDF5LIBS_INCLUDE_HDF5LIBS_HDF5FILELAYOUT_HPP_
