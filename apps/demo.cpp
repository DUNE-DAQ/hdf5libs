/**
 * @file demo.cpp
 *
 * Demo of HDF5 file reader 
 * 
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */


#include <iostream>
#include <string>

#include "DAQDecoder.hpp" 
#include "hdf5libs/StorageKeyList.hpp"

int main(int argc, char** argv){

  int num_trs = 1;
  if(argc <2) {
    std::cerr << "Usage: demo <fully qualified file name> [number of events to read]" << std::endl;
    return -1;
  }

  if(argc == 3) {
    num_trs = std::stoi(argv[2]);
    std::cout << "Number of events to read: " << num_trs << std::endl;
  }   

  DAQDecoder decoder = DAQDecoder(argv[1], num_trs);

  for(auto const& path : decoder.get_datasets())
    std::cout << path << std::endl;

  auto all_keys = decoder.get_all_storage_keys();
  std::cout << "Found " << all_keys.size() << " total keys." << std::endl;


  dunedaq::hdf5libs::StorageKey k;
  k.set_group_type(dunedaq::hdf5libs::StorageKey::DataRecordGroupType::kTriggerRecordHeader); //trigger record headers
  auto trh_keys = all_keys.get_all_matching_keys(k);

  std::cout << "Found " << trh_keys.size() << " Trigger Record Headers." << std::endl;

  k.set_group_type(dunedaq::hdf5libs::StorageKey::DataRecordGroupType::kTPC);
  auto apas_found = all_keys.get_matching_region_numbers(k);
  for(auto apa : apas_found)
    std::cout << "\tHave TPCs from APA " << apa << std::endl;
  auto links_found = all_keys.get_matching_element_numbers(k);
  for(auto link : links_found)
    std::cout << "\tHave TPCs with Links " << link << std::endl;

  /*
  for(auto const& key : decoder.get_all_storage_keys())
    std::cout << " Run: " << key.get_run_number()
	      << " Trigger: " << key.get_trigger_number()
	      << " Group Type: " << key.get_group_type()
	      << " Region: " << key.get_region_number()
	      << " Element: " << key.get_element_number()
	      << std::endl;
  */


  std::vector<std::string> datasets_path = decoder.get_fragments(num_trs);
  //std::vector<std::string> datasets_path = decoder.get_trh(num_trs);
  for (auto& element : datasets_path) {
    decoder.read_fragment(element);
  }
  

  return 0;
}
