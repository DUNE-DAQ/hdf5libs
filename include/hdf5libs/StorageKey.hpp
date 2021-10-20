/**
 * @file StorageKey.hpp Collection of parameters that identify a block of data
 *
 * 20.10.2021, WK: Copied and minimally modified from dfmodules
 * 
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef HDF5LIBS_INCLUDE_HDF5LIBS_STORAGEKEY_HPP_
#define HDF5LIBS_INCLUDE_HDF5LIBS_STORAGEKEY_HPP_

#include <limits>
#include <vector>

namespace dunedaq {
  namespace hdf5libs {

    class StorageKey;
    typedef std::vector<StorageKey> StorageKeyList;


    /**
     * @brief The StorageKey class defines the collection of parameters that
     * identify a given block of data.
     */
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
    //WK not sure what these do...
    //m_this_sequence_number = 0;
    //m_max_sequence_number = 0;
  }

  int get_run_number() const { return m_run_number; }
  int get_trigger_number() const { return m_trigger_number; }
  DataRecordGroupType get_group_type() const { return m_group_type; }
  int get_region_number() const { return m_region_number; }
  int get_element_number() const { return m_element_number; }

  //WK not sure what these do ...
  //int m_this_sequence_number;
  //int m_max_sequence_number;

private:
  int m_run_number;
  int m_trigger_number;
  DataRecordGroupType m_group_type;
  int m_region_number;
  int m_element_number;
};

  } // namespace hdf5libs
} // namespace dunedaq

#endif // HDF5LIBS_INCLUDE_HDF5LIBS_STORAGEKEY_HPP_
