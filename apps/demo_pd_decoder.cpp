/**
 * @file demo_pd_decoder.cpp
 *
 * Demo of HDF5 file reader for PD fragments: this example shows how to extract fragments from a file and decode SSP frames.
 * 
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */


#include <iostream>
#include <string>
#include <fstream>

#include "logging/Logging.hpp"
#include "hdf5libs/DAQDecoder.hpp" 
#include "utils.hpp"

using namespace dunedaq::hdf5libs;


int main(int argc, char** argv){
  std::cout << "Starting PD decoder" << std::endl;
 
  // Default number of records to read
  int num_trs = 1;
  if(argc <2) {
    std::cerr << "Usage: tpc_decoder <fully qualified file name> [number of events to read]" << std::endl;
    return -1;
  }

  if(argc == 3) {
    num_trs = std::stoi(argv[2]);
    std::cout << "Number of events to read: " << num_trs << std::endl;
  }   


  DAQDecoder decoder = DAQDecoder(argv[1], num_trs);

  std::vector<std::string> datasets_path = decoder.get_fragments(num_trs);
  //std::vector<std::string> datasets_path = decoder.get_trh(num_trs);
 
  std::cout << "Number of fragments: " << datasets_path.size() << std::endl; 

  // Read all the fragments
  int dropped_fragments_per_event = 0;
  int fragment_counter = 0; 
  for (auto& element : datasets_path) {
    fragment_counter += 1;
    std::cout << "Reading fragment " << fragment_counter << "/" << datasets_path.size() << std::endl; 
    std::cout << "Number of dropped fragments: " << dropped_fragments_per_event << std::endl;
    ReadSSPFrag(decoder.get_frag_ptr(element), dropped_fragments_per_event);
  }
  
  

  std::cout << "Finished parsing all fragments" << std::endl;
  
 return 0;
}
