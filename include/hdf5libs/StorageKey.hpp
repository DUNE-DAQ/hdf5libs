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

namespace dunedaq {
  namespace hdf5libs {

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

  //default constructor
  //useful for searching functionalities
  StorageKey()
    : m_run_number(s_invalid_run_number)
    , m_trigger_number(s_invalid_trigger_number)
    , m_group_type(DataRecordGroupType::kInvalid)
    , m_region_number(s_invalid_region_number)
    , m_element_number(s_invalid_element_number)
  {}

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

    //convert negative numbers to invalid ones...
    //very convenient for setting up matching keys
    if(m_run_number<0) m_run_number=s_invalid_run_number;
    if(m_trigger_number<0) m_trigger_number=s_invalid_trigger_number;
    if(m_group_type<0) m_group_type=DataRecordGroupType::kInvalid;
    if(m_region_number<0) m_region_number=s_invalid_region_number;
    if(m_element_number<0) m_element_number=s_invalid_element_number;

    //WK not sure what these do...
    //m_this_sequence_number = 0;
    //m_max_sequence_number = 0;
  }

  int get_run_number() const { return m_run_number; }
  int get_trigger_number() const { return m_trigger_number; }
  DataRecordGroupType get_group_type() const { return m_group_type; }
  int get_region_number() const { return m_region_number; }
  int get_element_number() const { return m_element_number; }


  void set_run_number(int r) { m_run_number=r; }
  void set_trigger_number(int t) { m_trigger_number=t; }
  void set_group_type(DataRecordGroupType g) { m_group_type=g; }
  void set_region_number(int r) { m_region_number=r; }
  void set_element_number(int e) { m_element_number=e; }

  //WK not sure what these do ...
  //int m_this_sequence_number;
  //int m_max_sequence_number;

  bool is_fully_valid() const
  { return ((m_run_number!=s_invalid_run_number) &&
	    (m_trigger_number!=s_invalid_trigger_number) &&
	    (m_group_type!=DataRecordGroupType::kInvalid) &&
	    (m_region_number!=s_invalid_region_number) &&
	    (m_element_number!=s_invalid_element_number)); }

  friend bool operator==(const StorageKey& k1, const StorageKey& k2)
  {
    return ( (k1.m_run_number==k2.m_run_number) &&
	     (k1.m_trigger_number==k2.m_trigger_number) &&
	     (k1.m_group_type==k2.m_group_type) &&
	     (k1.m_region_number==k2.m_region_number) &&
	     (k1.m_element_number==k2.m_element_number) );
  }
  friend bool operator!=(const StorageKey& k1, const StorageKey& k2)
  { return !(operator==(k1,k2)); }

  friend bool operator<(const StorageKey& k1, const StorageKey& k2)
  {
    if (k1.m_run_number!=k2.m_run_number)
      return k1.m_run_number<k2.m_run_number;
    if (k1.m_trigger_number!=k2.m_trigger_number)
      return k1.m_trigger_number<k2.m_trigger_number;
    if (k1.m_group_type!=k2.m_group_type)
      return k1.m_group_type<k2.m_group_type;
    if (k1.m_region_number!=k2.m_region_number)
      return k1.m_region_number<k2.m_region_number;
    if (k1.m_element_number!=k2.m_element_number)
      return k1.m_element_number<k2.m_element_number;

    return false;
  }

  friend bool operator>(const StorageKey& k1, const StorageKey& k2)
  { return operator<(k2,k1); }
  
  friend bool operator<=(const StorageKey& k1, const StorageKey& k2)
  { return !(operator>(k1,k2)); }

  friend bool operator>=(const StorageKey& k1, const StorageKey& k2)
  { return !(operator<(k1,k2)); }

  bool is_match(const StorageKey& k) const
  {
    return ( (k.get_run_number()==s_invalid_run_number || k.get_run_number()==m_run_number) && 
	     (k.get_trigger_number()==s_invalid_trigger_number || k.get_trigger_number()==m_trigger_number) &&
	     (k.get_group_type()==DataRecordGroupType::kInvalid || k.get_group_type()==m_group_type) &&
	     (k.get_region_number()==s_invalid_region_number || k.get_region_number()==m_region_number) && 
	     (k.get_element_number()==s_invalid_element_number || k.get_element_number()==m_element_number) );
  }

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
