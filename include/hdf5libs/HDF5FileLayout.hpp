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

#include <limits>
#include <cstdint>
#include <memory>
#include <map>

#include "nlohmann/json.hpp"

#include "daqdataformats/GeoID.hpp"

#include "hdf5libs/hdf5filelayout/Structs.hpp"

namespace dunedaq {
namespace hdf5libs {

class HDF5FileLayout
{
public:

  /**
   * @brief Constructor from json conf, used in DataWriter. Version always most recent.
   */
  HDF5FileLayout(const nlohmann::json& conf)
    : m_version(1)
  {

    m_conf_params = conf.get<hdf5filelayout::FileLayoutParams>();
    fill_path_params(m_conf_params);
  }

private:
  /**
   * @brief version number
   */
  uint32_t m_version;

  /**
   * @brief FileLayout configuration parameters
   */
  hdf5filelayout::FileLayoutParams m_conf_params;

  /**
   * @brief map translation for GeoID::SystemType to dataset path parameters
   */
  std::map< daqdataformats::GeoID::SystemType,hdf5filelayout::PathParams > m_path_params_map;

  /**
   * @brief Fill path parameters map from FileLayoutParams
   */
  void fill_path_params_map(hdf5filelayout::FileLayoutParams const& flp)
  {
    for(auto const& path_param : flp.path_param_list){
      auto sys_type = daqdataformats::GeoID::string_to_system_type(path_param.detector_group_type);
      if(sys_type==daqdataformats::GeoID::SystemType::kInvalid)
	continue; //update to make it show an error
      m_path_params_map[sys_type] = path_param;
    }

  }

};





/**
 * @brief comment
 */
struct KeyedDataBlock
{
public:
  HDF5FileLayout m_data_key;
  size_t m_data_size;
  const void* m_unowned_data_start;
  std::unique_ptr<char> m_owned_data_start;

  explicit KeyedDataBlock(const HDF5FileLayout& theKey) noexcept
    : m_data_key(theKey)
  {}

  const void* get_data_start() const
  {
    if (m_owned_data_start.get() != nullptr) {
      return static_cast<const void*>(m_owned_data_start.get());
    } else {
      return m_unowned_data_start;
    }
  }

  size_t get_data_size_bytes() const { return m_data_size; }
};

} // namespace hdf5libs
} // namespace dunedaq

#endif // HDF5LIBS_INCLUDE_HDF5LIBS_STORAGEKEY_HPP_
