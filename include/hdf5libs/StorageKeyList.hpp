/**
 * @file StorageKeyList.hpp List of StorageKeys, with helpful manipulations.
 *
 * 20.10.2021, WK: Copied and minimally modified from dfmodules
 * 
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef HDF5LIBS_INCLUDE_HDF5LIBS_STORAGEKEYLIST_HPP_
#define HDF5LIBS_INCLUDE_HDF5LIBS_STORAGEKEYLIST_HPP_

#include <vector>
#include <set>

#include "hdf5libs/StorageKey.hpp"

namespace dunedaq {
namespace hdf5libs {

class StorageKeyList : public std::vector<StorageKey> {

public:

  //general matching function
  StorageKeyList get_all_matching_keys(const StorageKey k)
  {
    StorageKeyList matching_keys;
    for(auto const& key : *this)
      if(key.is_match(k)) matching_keys.emplace_back(key);
    return matching_keys;
  }

  //get by matching storage key
  std::set<int> get_matching_run_numbers(const StorageKey k)
  {
    std::set<int> runs;
    for(auto const& key : this->get_all_matching_keys(k))
      runs.emplace(key.get_run_number());
    return runs;
  }

  std::set<int> get_matching_trigger_numbers(const StorageKey k)
  {
    std::set<int> trigs;
    for(auto const& key : this->get_all_matching_keys(k))
      trigs.emplace(key.get_trigger_number());
    return trigs;
  }

  std::set<StorageKey::DataRecordGroupType> get_matching_group_types(const StorageKey k)
  {
    std::set<StorageKey::DataRecordGroupType> groups;
    for(auto const& key : this->get_all_matching_keys(k))
      groups.emplace(key.get_group_type());
    return groups;
  }

  std::set<int> get_matching_region_numbers(const StorageKey k)
  {
    std::set<int> regs;
    for(auto const& key : this->get_all_matching_keys(k))
      regs.emplace(key.get_region_number());
    return regs;
  }

  std::set<int> get_matching_element_numbers(const StorageKey k)
  {
    std::set<int> elems;
    for(auto const& key : this->get_all_matching_keys(k))
      elems.emplace(key.get_element_number());
    return elems;
  }

  //work down the heirarchy
  std::set<int> get_all_run_numbers()
  { 
    StorageKey k;
    return get_matching_run_numbers(k);
  }

  std::set<int> get_all_trigger_numbers(int run_number)
  {
    StorageKey k;
    k.set_run_number(run_number);
    return get_matching_trigger_numbers(k);
  }

  std::set<StorageKey::DataRecordGroupType> get_all_group_types(int run_number,
								int trigger_number)
  {
    StorageKey k;
    k.set_run_number(run_number);
    k.set_trigger_number(trigger_number);
    return get_matching_group_types(k);
  }

  std::set<int> get_all_region_numbers(int run_number,
				       int trigger_number,
				       StorageKey::DataRecordGroupType group_type)
  {
    StorageKey k;
    k.set_run_number(run_number);
    k.set_trigger_number(trigger_number);
    k.set_group_type(group_type);
    return get_matching_region_numbers(k);
  }
  std::set<int> get_all_element_numbers(int run_number,
					int trigger_number,
					StorageKey::DataRecordGroupType group_type,
					int region_number)
  {
    StorageKey k;
    k.set_run_number(run_number);
    k.set_trigger_number(trigger_number);
    k.set_group_type(group_type);
    k.set_region_number(region_number);
    return get_matching_element_numbers(k);
  }


private:


}; //class StorageKeyList


} // namespace hdf5libs
} // namespace dunedaq

#endif // HDF5LIBS_INCLUDE_HDF5LIBS_STORAGEKEY_HPP_
