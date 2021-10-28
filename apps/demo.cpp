/**
 * @file demo.cpp
 *
 * Demo of HDF5 file reader: this example shows how to extract fragments from a file and decode WIB frames
 * A user who wishes to decode different data or make different operations should just change this file.
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

#include "dataformats/wib/WIBFrame.hpp"

void ReadWibFrag(std::unique_ptr<dunedaq::dataformats::Fragment> frag) {
     if(frag->get_fragment_type() == dunedaq::dataformats::FragmentType::kTPCData) {
       std::cout << "Fragment with Run number: " << frag->get_run_number()
                 << " Trigger number: " << frag->get_trigger_number()
                 << " Sequence number: " << frag->get_sequence_number()
                 << " GeoID: " << frag->get_element_id() << std::endl;

       size_t raw_data_packets = (frag->get_size() - sizeof(dunedaq::dataformats::FragmentHeader)) / sizeof(dunedaq::dataformats::WIBFrame);
       std::cout << "Fragment contains " << raw_data_packets << " WIB frames" << std::endl;
        for (size_t i=0; i < raw_data_packets; ++i) {
           auto wfptr = reinterpret_cast<dunedaq::dataformats::WIBFrame*>(frag->get_data()+i*sizeof(dunedaq::dataformats::WIBFrame));
           if (i==0) {
               std::cout << "First WIB header:"<< *(wfptr->get_wib_header());
//               std::cout << "Printout sampled timestamps in WIB headers: " ;
           }
 //          if(i%1000 == 0) std::cout << "Timestamp " << i << ": " << wfptr->get_timestamp() << " ";
       }
       std::cout << std::endl;
     }
     else {
       std::cout << "Skipping unknown fragment type" << std::endl;
     }
     return;
}

int main(int argc, char** argv){
  int num_trs = 1000000;
  if(argc <2) {
    std::cerr << "Usage: demo <fully qualified file name> [number of events to read]" << std::endl;
    return -1;
  }

  if(argc == 3) {
    num_trs = std::stoi(argv[2]);
    std::cout << "Number of events to read: " << num_trs << std::endl;
  }   

  DAQDecoder decoder = DAQDecoder(argv[1], num_trs);

  for(auto const& path : decoder.get_datasets()){
    auto key = decoder.make_key_from_path(path);
    auto path_back = decoder.make_path_from_key(key);

    //std::cout << path << std::endl;
    //std::cout << "Key: " << key << std::endl;
    //std::cout << "Path back: " << decoder.make_path_from_key(key) << std::endl;

    if(path!=path_back)
      std::cout << "Problem: path (" << path 
		<< ") != path_back (" << path_back << ")"
		<< std::endl;
  }

  auto all_keys = decoder.get_all_storage_keys();
  std::cout << "Found " << all_keys.size() << " total keys." << std::endl;


  dunedaq::hdf5libs::StorageKey k;
  k.set_group_type(dunedaq::hdf5libs::DataRecordGroupTypeID::kTriggerRecordHeader); //trigger record headers
  auto trh_keys = all_keys.get_all_matching_keys(k);

  std::cout << "Found " << trh_keys.size() << " Trigger Record Headers." << std::endl;

  /*
  k.set_group_type(dunedaq::hdf5libs::StorageKey::DataRecordGroupType::kTPC);
  auto apas_found = all_keys.get_matching_region_numbers(k);
  for(auto apa : apas_found)
    std::cout << "\tHave TPCs from APA " << apa << std::endl;
  auto links_found = all_keys.get_matching_element_numbers(k);
  for(auto link : links_found)
    std::cout << "\tHave TPCs with Links " << link << std::endl;
  */
  
  //for(auto const& key : decoder.get_all_storage_keys())
  //std::cout << " Key: " << key << std::endl;



  std::vector<std::string> tr_paths = decoder.get_trh(num_trs);
  for (auto& tr : tr_paths) {
    std::cout << "=== " << tr << " ===" << std::endl;
    std::unique_ptr<dunedaq::dataformats::TriggerRecordHeader> trh_ptr(decoder.get_trh_ptr(tr));
    std::cout << "Trigger record with run number: " << trh_ptr->get_run_number()
              << " Trigger number: " << trh_ptr->get_trigger_number()
              << " Sequence number: " << trh_ptr->get_sequence_number() << std::endl;
  }

  std::vector<std::string> datasets_path = decoder.get_fragments(num_trs);
  //std::vector<std::string> datasets_path = decoder.get_trh(num_trs);
  for (auto& element : datasets_path) {
    std::cout << "*** " << element << " ***" << std::endl;
    ReadWibFrag(decoder.get_frag_ptr(element));
  }
  return 0;
}
