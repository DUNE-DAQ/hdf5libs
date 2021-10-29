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
  int num_trs = 0;
  if(argc <2) {
    std::cerr << "Usage: demo <fully qualified file name> [number of events to read]" << std::endl;
    return -1;
  }

  if(argc == 3) {
    num_trs = std::stoi(argv[2]);
    std::cout << "Number of events to read: " << num_trs << std::endl;
  }   

  auto decoder = dunedaq::hdf5libs::DAQDecoder(argv[1], num_trs);

  //getting all storage keys
  auto all_keys = decoder.get_all_storage_keys();
  std::cout << "Found " << all_keys.size() << " total keys." << std::endl;

  //example to grab the set of trigger numbers from those keys
  auto trigger_numbers = dunedaq::hdf5libs::keyutils::get_trigger_numbers(all_keys);
  std::cout << "\tFound " << trigger_numbers.size() << " unique trigger records, "
	    << "ranging from " << *(trigger_numbers.begin()) << " to " << *(trigger_numbers.rbegin()) << "."
	    << std::endl;

  //Loop over TriggerRecordHeader keys from first num_trs events, and create TriggerRecordHeaders
  for (auto& trh_key : decoder.get_trh_keys(num_trs)) {
    std::cout << "=== " << decoder.make_path_from_key(trh_key) << " ===" << std::endl;
    std::unique_ptr<dunedaq::dataformats::TriggerRecordHeader> trh_ptr(decoder.get_trh_ptr(trh_key));
    std::cout << "Trigger record with run number: " << trh_ptr->get_run_number()
              << " Trigger number: " << trh_ptr->get_trigger_number()
              << " Sequence number: " << trh_ptr->get_sequence_number() << std::endl;
  }
  
  //Loop over TPC keys from first num_trs events, and create WIBFragments
  for (auto& tpc_key : decoder.get_fragment_keys(num_trs,"TPC")) {
    std::cout << "*** " << decoder.make_path_from_key(tpc_key) << " ***" << std::endl;
    ReadWibFrag(decoder.get_frag_ptr(tpc_key));
  }
  return 0;
}
