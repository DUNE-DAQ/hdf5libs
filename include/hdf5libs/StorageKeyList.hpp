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

typedef std::vector<StorageKey> StorageKeyList;

namespace keyutils{

  //general matching function
  StorageKeyList get_all_matching_keys(const StorageKeyList& keys,
				       const StorageKey k)
  {
    StorageKeyList matching_keys;
    for(auto const& key : keys)
      if(key.is_match(k)) matching_keys.emplace_back(key);
    return matching_keys;
  }

  std::set<int> get_run_numbers(const StorageKeyList& keys,
				const StorageKey k=StorageKey())
  {
    std::set<int> runs;
    for(auto const& key : get_all_matching_keys(keys,k))
      runs.emplace(key.get_run_number());
    return runs;
  }
  
  std::set<int> get_trigger_numbers(const StorageKeyList& keys,
				    const StorageKey k=StorageKey())
  {
    std::set<int> trigs;
    for(auto const& key : get_all_matching_keys(keys,k))
      trigs.emplace(key.get_trigger_number());
    return trigs;
  }
  
  std::set<DataRecordGroupTypeID> get_group_ids(const StorageKeyList& keys,
						const StorageKey k=StorageKey())
  {
    std::set<DataRecordGroupTypeID> gids;
    for(auto const& key : get_all_matching_keys(keys,k))
      gids.emplace(key.get_group_type().get_id());
    return gids;
  }

  std::set<std::string> get_group_names(const StorageKeyList& keys,
					const StorageKey k=StorageKey())
  {
    std::set<std::string> gnames;
    for(auto const& key : get_all_matching_keys(keys,k))
      gnames.emplace(key.get_group_type().get_group_name());
    return gnames;
  }

  std::set<int> get_region_numbers(const StorageKeyList& keys,
				   const StorageKey k=StorageKey())
  {
    std::set<int> regs;
    for(auto const& key : get_all_matching_keys(keys,k))
      regs.emplace(key.get_region_number());
    return regs;
  }

  std::set<int> get_element_numbers(const StorageKeyList& keys,
				    const StorageKey k=StorageKey())
  {
    std::set<int> elems;
    for(auto const& key : get_all_matching_keys(keys,k))
      elems.emplace(key.get_element_number());
    return elems;
  }

  StorageKeyList get_trh_keys(const StorageKeyList& keys,
			      StorageKey k=StorageKey())
  {
    k.set_group_type("TriggerRecordHeader");
    return get_all_matching_keys(keys,k);
  }

  StorageKeyList get_tpc_keys(const StorageKeyList& keys,
			      StorageKey k=StorageKey())
  {
    k.set_group_type("TPC");
    return get_all_matching_keys(keys,k);
  }
  
  StorageKeyList get_pds_keys(const StorageKeyList& keys,
			      StorageKey k=StorageKey())
  {
    k.set_group_type("PDS");
    return get_all_matching_keys(keys,k);
  }
  
  StorageKeyList get_trigger_keys(const StorageKeyList& keys,
				  StorageKey k=StorageKey())
  {
    k.set_group_type("Trigger");
    return get_all_matching_keys(keys,k);
  }

  StorageKeyList get_tpc_tp_keys(const StorageKeyList& keys,
				 StorageKey k=StorageKey())
  {
    k.set_group_type("TPC_TP");
    return get_all_matching_keys(keys,k);
  }

  StorageKeyList get_ndlartpc_keys(const StorageKeyList& keys,
				   StorageKey k=StorageKey())
  {
    k.set_group_type("NDLArTPC");
    return get_all_matching_keys(keys,k);
  }

  StorageKeyList get_keys_by_run_number(const StorageKeyList& keys,
					int run)
  {
    StorageKey k; k.set_run_number(run);
    return get_all_matching_keys(keys,k);
  }

  StorageKeyList get_keys_by_trigger_number(const StorageKeyList& keys,
					    int tn)
  {
    StorageKey k; k.set_trigger_number(tn);
    return get_all_matching_keys(keys,k);
  }

  StorageKeyList get_keys_by_group_name(const StorageKeyList& keys,
					std::string gname)
  {
    StorageKey k; k.set_group_type(gname);
    return get_all_matching_keys(keys,k);
  }

  StorageKeyList get_keys_by_group_id(const StorageKeyList& keys,
				      DataRecordGroupTypeID gid)
  {
    StorageKey k; k.set_group_type(gid);
    return get_all_matching_keys(keys,k);
  }

  StorageKeyList get_keys_by_region_number(const StorageKeyList& keys,
					   int reg)
  {
    StorageKey k; k.set_region_number(reg);
    return get_all_matching_keys(keys,k);
  }

  StorageKeyList get_keys_by_element_number(const StorageKeyList& keys,
					    int elem)
  {
    StorageKey k; k.set_element_number(elem);
    return get_all_matching_keys(keys,k);
  }

} // namespace keyutils
} // namespace hdf5libs
} // namespace dunedaq

#endif // HDF5LIBS_INCLUDE_HDF5LIBS_STORAGEKEY_HPP_
