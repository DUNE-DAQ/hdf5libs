/**
 * @file demo.cpp
 *
 * Example of HDF5 file reader 
 * 
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */


#include <iostream>
#include <string>

#include "DAQDecoder.hpp" 

int main(int argc, char** argv){

  int num_trs = 1000;
  if(argc <2) {
    std::cerr << "Usage: data_file_browser <fully qualified file name> [number of events to read]" << std::endl;
    return -1;
  }

  if(argc == 3) {
    num_trs = std::stoi(argv[2]);
  }   

  DAQDecoder decoder = DAQDecoder(argv[1], num_trs);

  //std::vector<std::string> datasets_path = decoder.get_fragments(num_trs);
  std::vector<std::string> datasets_path = decoder.get_trh(num_trs);
  for (auto& element : datasets_path) {
    decoder.read_fragment(element);
  }
  

  return 0;
}
