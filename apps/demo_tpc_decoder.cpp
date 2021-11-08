/**
 * @file demo_tpc_decoder.cpp
 *
 * Demo of HDF5 file reader for TPC fragments: this example shows how to extract fragments from a file and decode WIB frames.
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

#include "detchannelmaps/TPCChannelMap.hpp"

using namespace dunedaq::hdf5libs;
using namespace dunedaq::detchannelmaps;

int main(int argc, char** argv){
  std::cout << "Starting TPC decoder" << std::endl;
 
  // Default number of records to read
  int num_trs = 1000000;
  if(argc <3) {
    std::cerr << "Usage: demo <fully qualified file name> <VDColdboxChannelMap | ProtoDUNESP1ChannelMap> [number of events to read]" << std::endl;
    return -1;
  }

  if(argc == 4) {
    num_trs = std::stoi(argv[3]);
    std::cout << "Number of events to read: " << num_trs << std::endl;
  }   


  DAQDecoder decoder = DAQDecoder(argv[1], num_trs);

  std::vector<std::string> datasets_path = decoder.get_fragments(num_trs);
  //std::vector<std::string> datasets_path = decoder.get_trh(num_trs);
 
  std::cout << "Number of fragments: " << datasets_path.size() << std::endl; 

  // Read all the fragments
  int dropped_fragments = 0;
  int fragment_counter = 0; 

  size_t raw_data_packets = (decoder.get_frag_ptr(datasets_path[0])->get_size() - sizeof(dunedaq::daqdataformats::FragmentHeader)) / sizeof(dunedaq::detdataformats::WIBFrame);

  std::map<size_t, std::pair<float,float> > offline_map;
  std::vector<uint32_t> adc_channels_sums(raw_data_packets,0);
  std::shared_ptr<TPCChannelMap> vdcb_map = make_map(argv[2]);

  for (auto& element : datasets_path) {
    fragment_counter += 1;
    std::cout << "Reading fragment " << fragment_counter << "/" << datasets_path.size() << std::endl; 
    std::cout << "Number of dropped fragments: " << dropped_fragments << std::endl;
    ReadWibFrag(decoder.get_frag_ptr(element), vdcb_map, &offline_map, &adc_channels_sums, dropped_fragments);
  }
  
  

  std::cout << "Finished parsing all fragments" << std::endl;
  
 return 0;
}
