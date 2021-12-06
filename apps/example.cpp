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
#include "hdf5libs/HDF5RawDataFile.hpp" 


using namespace dunedaq::hdf5libs;

int main(int argc, char** argv){

  std::cout << "Starting EXAMPLE code" << std::endl;

  nlohmann::json conf ;

  HDF5RawDataFile h5_raw_data_file = HDF5RawDataFile(conf);

/*
  // Get and print attribute names and their values
  auto attributes_map = decoder.get_attributes();
  for (const auto& [k, v] : attributes_map){
    std::cout << k << " : ";
    std::visit([](const auto& x){ std::cout << x; }, v);
    std::cout << '\n';
  }


  std::vector<std::string> datasets_path = decoder.get_fragments(start_tr,num_trs);
  //std::vector<std::string> datasets_path = decoder.get_trh(start_tr,num_trs);
 
  std::cout << "Number of fragments: " << datasets_path.size() << std::endl; 

  // Read all the fragments
  int dropped_fragments = 0;
  int fragment_counter = 0; 

  size_t raw_data_packets = (decoder.get_frag_ptr(datasets_path[0])->get_size() - sizeof(dunedaq::daqdataformats::FragmentHeader)) / sizeof(dunedaq::detdataformats::wib::WIBFrame);

  std::map<size_t, std::pair<float,float> > offline_map;
  std::vector<uint32_t> adc_channels_sums(raw_data_packets,0);
  std::shared_ptr<TPCChannelMap> vdcb_map = make_map(argv[2]);

  for (auto& element : datasets_path) {
    fragment_counter += 1;
    std::cout << "Reading fragment " << fragment_counter << "/" << datasets_path.size() << std::endl; 
    std::cout << "Number of dropped fragments: " << dropped_fragments << std::endl;
    ReadWibFrag(decoder.get_frag_ptr(element), vdcb_map, &offline_map, &adc_channels_sums, dropped_fragments);
  }
*/  
  

  
 return 0;
}
