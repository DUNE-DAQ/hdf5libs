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
    std::cerr << "Usage: demo <fully qualified file name> [number of events to read]" << std::endl;
    return -1;
  }

  if(argc == 3) {
    num_trs = std::stoi(argv[2]);
    std::cout << "Number of events to read: " << num_trs << std::endl;
  }   


  std::cout << "Before DAQDecoder \n";

  DAQDecoder decoder = DAQDecoder(argv[1], num_trs);

  std::vector<std::string> datasets_path = decoder.get_fragments(num_trs);
  //std::vector<std::string> datasets_path = decoder.get_trh(num_trs);
 
  std::cout << "Start to read" << std::endl;
 
  // Read all the fragments
  std::ofstream output_file("./ssp_frames_total.txt");
  for (auto& element : datasets_path) {
    auto vector_adc = ReadSSPFrag(decoder.get_frag_ptr(element));
    std::cout <<" Read fragment " << std::endl; 
    for (const auto &e : vector_adc) output_file << e << "\n";  
  }
  std::cout << "Finished reading" << std::endl;
  
  // Read only one fragment
  /*
  auto frag = decoder.get_frag_ptr(datasets_path[0]);
  std::ofstream output_file("./ssp_frames.txt");
  if (frag->get_fragment_type() == dunedaq::dataformats::FragmentType::kPDSData) {
    auto daphne_frames = ReadSSPFrag(std::move(frag));
    for (const auto &e : daphne_frames) output_file << e << "\n";  
  }
  */
  return 0;
}
