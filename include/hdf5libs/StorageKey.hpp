/**
 * @file StorageKey.hpp
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef HDF5LIBS_INCLUDE_HDF5LIBS_STORAGEKEY_HPP_
#define HDF5LIBS_INCLUDE_HDF5LIBS_STORAGEKEY_HPP_

#include <limits>
#include <cstdint>
#include <memory>

namespace dunedaq {
namespace hdf5libs {

class StorageKey
{
public:
  static constexpr int s_invalid_run_number = std::numeric_limits<int>::max();
  static constexpr int s_invalid_trigger_number = std::numeric_limits<int>::max();
  static constexpr int s_invalid_region_number = std::numeric_limits<int>::max();
  static constexpr int s_invalid_element_number = std::numeric_limits<int>::max();

  /**
   * @brief The group that should be used within the data record.
   */
  enum DataRecordGroupType
  {
    kTriggerRecordHeader = 1,
    kTPC = 2,
    kPDS = 3,
    kTrigger = 4,
    kTPC_TP = 5,
    kNDLArTPC = 6,
    kInvalid = 0
  };

  StorageKey(int run_number,
             int trigger_number,
             DataRecordGroupType group_type,
             int region_number,
             int element_number) noexcept
    : m_run_number(run_number)
    , m_trigger_number(trigger_number)
    , m_group_type(group_type)
    , m_region_number(region_number)
    , m_element_number(element_number)
  {
    m_this_sequence_number = 0;
    m_max_sequence_number = 0;
  }

  int get_run_number() const;
  int get_trigger_number() const;
  DataRecordGroupType get_group_type() const;
  int get_region_number() const;
  int get_element_number() const;

  int m_this_sequence_number;
  int m_max_sequence_number;

private:
  int m_run_number;
  int m_trigger_number;
  DataRecordGroupType m_group_type;
  int m_region_number;
  int m_element_number;
};





/**
 * @brief comment
 */
struct KeyedDataBlock
{
public:
  // These data members will be made private, at some point in time.
  StorageKey m_data_key;
  size_t m_data_size;
  const void* m_unowned_data_start;
  std::unique_ptr<char> m_owned_data_start;

  explicit KeyedDataBlock(const StorageKey& theKey) noexcept
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
