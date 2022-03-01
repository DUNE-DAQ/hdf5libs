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
  explicit HDF5FileLayout(hdf5filelayout::FileLayoutParams conf, uint32_t version = 2); // NOLINT(build/unsigned)

  uint32_t get_version() const noexcept { return m_version; } // NOLINT(build/unsigned)

  std::string get_trigger_record_name_prefix() const noexcept 
  { return m_conf_params.trigger_record_name_prefix; }
  
  int get_digits_for_trigger_number() const noexcept 
  { return m_conf_params.digits_for_trigger_number; }

  int get_digits_for_sequence_number() const noexcept 
  { return m_conf_params.digits_for_sequence_number; }
  
  std::string get_trigger_header_dataset_name() const noexcept 
  { return m_conf_params.trigger_record_header_dataset_name; }

  std::map<daqdataformats::GeoID::SystemType, hdf5filelayout::PathParams> 
  get_path_params_map() const 
  { return m_path_params_map; }

  hdf5filelayout::PathParams 
  get_path_params(daqdataformats::GeoID::SystemType type) const
  { return m_path_params_map.at(type); }

  hdf5filelayout::FileLayoutParams get_file_layout_params() const 
  { return m_conf_params; }

  /**
   * @brief get string for Trigger number
   */
  std::string get_trigger_number_string(daqdataformats::trigger_number_t trig_num,
                                        daqdataformats::sequence_number_t seq_num = 0) const;

  /**
   * @brief get the correct path for the TriggerRecordHeader
   */
  std::vector<std::string> 
  get_path_elements(const daqdataformats::TriggerRecordHeader& trh) const;

  /**
   * @brief get the correct path for the Fragment
   */
  std::vector<std::string> 
  get_path_elements(const daqdataformats::FragmentHeader& fh) const;
  /**
   * @brief get the full path for a TriggerRecordHeader dataset based on trig/seq number
   */
  std::string get_trigger_record_header_path(daqdataformats::trigger_number_t trig_num,
                                             daqdataformats::sequence_number_t seq_num = 0) const;

  /**
   * @brief get the full path for a Fragment dataset based on trig/seq number and element ID
   */
  std::string get_fragment_path(daqdataformats::trigger_number_t trig_num,
                                daqdataformats::GeoID element_id,
                                daqdataformats::sequence_number_t seq_num = 0) const;
  /**
   * @brief get the full path for a Fragment dataset based on trig/seq number, give element_id pieces
   */
  std::string get_fragment_path(daqdataformats::trigger_number_t trig_num,
                                daqdataformats::GeoID::SystemType type,
                                uint16_t region_id, // NOLINT(build/unsigned)
                                uint32_t element_id, // NOLINT(build/unsigned)
                                daqdataformats::sequence_number_t seq_num = 0) const;

  /**
   * @brief get the full path for a Fragment dataset based on trig/seq number, give element_id pieces
   */
  std::string get_fragment_path(daqdataformats::trigger_number_t trig_num,
                                const std::string& typestring,
                                uint16_t region_id, // NOLINT(build/unsigned)
                                uint32_t element_id, // NOLINT(build/unsigned)
                                daqdataformats::sequence_number_t seq_num = 0) const;

  /**
   * @brief get the path for a Fragment type group based on trig/seq number and type
   */
  std::string get_fragment_type_path(daqdataformats::trigger_number_t trig_num,
                                     daqdataformats::GeoID::SystemType type,
                                     daqdataformats::sequence_number_t seq_num = 0) const;

  /**
   * @brief get the path for a Fragment type group based on trig/seq number and type
   */
  std::string get_fragment_type_path(daqdataformats::trigger_number_t trig_num,
                                     std::string typestring,
                                     daqdataformats::sequence_number_t seq_num = 0) const;

  /**
   * @brief get the path for a Fragment region group based on trig/seq number,type, and region ID
   */
  std::string get_fragment_region_path(daqdataformats::trigger_number_t trig_num,
                                       daqdataformats::GeoID::SystemType type,
                                       uint16_t region_id, // NOLINT(build/unsigned)
                                       daqdataformats::sequence_number_t seq_num = 0) const;

  /**
   * @brief get the path for a Fragment region group based on trig/seq number,type, and region ID
   */
  std::string get_fragment_region_path(daqdataformats::trigger_number_t trig_num,
                                       std::string typestring,
                                       uint16_t region_id, // NOLINT(build/unsigned)
                                       daqdataformats::sequence_number_t seq_num = 0) const;

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
  void fill_path_params_map(hdf5filelayout::FileLayoutParams const& flp);

  /**
   * @brief Version0 FileLayout parameters, for backward compatibility
   */
  hdf5filelayout::FileLayoutParams get_v0_file_layout_params();
};

} // namespace hdf5libs
} // namespace dunedaq

#endif // HDF5LIBS_INCLUDE_HDF5LIBS_HDF5FILELAYOUT_HPP_
