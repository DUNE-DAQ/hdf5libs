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

#include "hdf5libs/DataRecordGroupType.hpp"

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
  static constexpr DataRecordGroupTypeID s_invalid_group_type = DataRecordGroupTypeID::kInvalid;
  static constexpr int s_invalid_region_number = std::numeric_limits<int>::max();
  static constexpr int s_invalid_element_number = std::numeric_limits<int>::max();

  //default constructor
  //useful for searching functionalities
  StorageKey()
    : m_run_number(s_invalid_run_number)
    , m_trigger_number(s_invalid_trigger_number)
    , m_group_type(s_invalid_group_type)
    , m_region_number(s_invalid_region_number)
    , m_element_number(s_invalid_element_number)
  {}

  StorageKey(int run_number,
             int trigger_number,
             DataRecordGroupTypeID group_type,
             int region_number,
             int element_number) noexcept
  : m_run_number(run_number)
    , m_trigger_number(trigger_number)
    , m_group_type(group_type)
    , m_region_number(region_number)
    , m_element_number(element_number)
  {
    convert_negative_to_invalid();
  }

  int get_run_number() const { return m_run_number; }
  int get_trigger_number() const { return m_trigger_number; }
  DataRecordGroupType get_group_type() const { return m_group_type; }
  int get_region_number() const { return m_region_number; }
  int get_element_number() const { return m_element_number; }

  void set_run_number(int r) { m_run_number=r; convert_negative_to_invalid(); }
  void set_trigger_number(int t) { m_trigger_number=t; convert_negative_to_invalid(); }
  void set_group_type(DataRecordGroupType gt) { m_group_type=gt; convert_negative_to_invalid(); }
  void set_group_type(DataRecordGroupTypeID gid) { m_group_type=DataRecordGroupType(gid); convert_negative_to_invalid(); }
  void set_group_type(std::string gname) { m_group_type=DataRecordGroupType(gname); convert_negative_to_invalid(); }
  void set_region_number(int r) { m_region_number=r; convert_negative_to_invalid(); }
  void set_element_number(int e) { m_element_number=e; convert_negative_to_invalid(); }

  bool is_valid_run_number() const 
  { return (m_run_number!=s_invalid_run_number); }
  bool is_valid_trigger_number() const 
  { return (m_trigger_number!=s_invalid_trigger_number); }
  bool is_valid_group_type() const
  { return (m_group_type.get_id()!=s_invalid_group_type); }
  bool is_valid_region_number() const
  { return (m_region_number!=s_invalid_region_number); }
  bool is_valid_element_number() const
  { return (m_element_number!=s_invalid_element_number); }


  bool is_fully_valid() const
  { 
    return ( is_valid_run_number() &&
	     is_valid_trigger_number() &&
	     is_valid_group_type() &&
	     is_valid_region_number() &&
	     is_valid_element_number() ); 
  }

  friend bool operator==(const StorageKey& k1, const StorageKey& k2)
  {
    return ( (k1.m_run_number==k2.m_run_number) &&
	     (k1.m_trigger_number==k2.m_trigger_number) &&
	     (k1.m_group_type.get_id()==k2.m_group_type.get_id()) &&
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
    if (k1.m_group_type.get_id()!=k2.m_group_type.get_id())
      return k1.m_group_type.get_id()<k2.m_group_type.get_id();
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
    return ( (!is_valid_run_number()     || !k.is_valid_run_number()     || k.get_run_number()==m_run_number) && 
	     (!is_valid_trigger_number() || !k.is_valid_trigger_number() || k.get_trigger_number()==m_trigger_number) &&
	     (!is_valid_group_type()     || !k.is_valid_group_type()     || k.get_group_type().get_id()==m_group_type.get_id()) &&
	     (!is_valid_region_number()  || !k.is_valid_region_number()  || k.get_region_number()==m_region_number) && 
	     (!is_valid_element_number() || !k.is_valid_element_number() || k.get_element_number()==m_element_number) );
  }

private:
  int m_run_number;
  int m_trigger_number;
  DataRecordGroupType m_group_type;
  int m_region_number;
  int m_element_number;

  //convert negative numbers to invalid ones...
  //very convenient for setting up matching keys
  void convert_negative_to_invalid()
  {
    if(m_run_number<0) m_run_number=s_invalid_run_number;
    if(m_trigger_number<0) m_trigger_number=s_invalid_trigger_number;
    //if(m_group_type.id<0) m_group_type.id=s_invalid_group_type;
    if(m_region_number<0) m_region_number=s_invalid_region_number;
    if(m_element_number<0) m_element_number=s_invalid_element_number;
  }

};

    //stream override
    std::ostream& operator<<(std::ostream& os, const StorageKey& k)
    {
      os << "Run: " << k.get_run_number()
	 << " Trig. num.: " << k.get_trigger_number()
	 << " Group: " << k.get_group_type();
      
      if(k.get_group_type().get_id()==DataRecordGroupTypeID::kTriggerRecordHeader)
	return os;
      
      os << " Reg.(" << k.get_group_type().get_region_prefix() << "): " << k.get_region_number()
	 << " Elem.(" << k.get_group_type().get_element_prefix() << "): " << k.get_element_number();
      return os;
    }
    
    
  } // namespace hdf5libs
} // namespace dunedaq

#endif // HDF5LIBS_INCLUDE_HDF5LIBS_STORAGEKEY_HPP_
