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
#include <fstream>

#include "DAQDecoder.hpp" 
#include "utils.hpp"



int main(int argc, char** argv){
  int num_trs = 1;
  if(argc <2) {
    std::cerr << "Usage: tpc_decoder <fully qualified file name> [number of events to read]" << std::endl;
    return -1;
  }

  if(argc == 3) {
    num_trs = std::stoi(argv[2]);
    std::cout << "Number of events to read: " << num_trs << std::endl;
  }   


  std::cout << "Starting PD decoder" << std::endl;
  DAQDecoder decoder = DAQDecoder(argv[1], num_trs);

  std::vector<std::string> datasets_path = decoder.get_fragments(num_trs);
  //std::vector<std::string> datasets_path = decoder.get_trh(num_trs);
 
 
  // Read all the fragments
  /*
  for (auto& element : datasets_path) {
    std::cout <<" Reading fragment " << std::endl; 
    ReadSSPFrag(decoder.get_frag_ptr(element));
  }
  */

  // Read only one fragment
  auto frag = decoder.get_frag_ptr(datasets_path[0]);
  
  // Output file to dump all the values
  std::ofstream output_file("./ssp_frames.txt");
  auto daphne_frames = ReadSSPFrag(std::move(frag));
  for (const auto &e : daphne_frames) output_file << e << "\n";  
  

  std::cout << "Finished parsing all fragments" << std::endl;
  
 return 0;
}
