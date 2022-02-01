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

//int main(int argc, char** argv){
int main(int , char** ){ //since argc/argv not in use

  std::cout << "Starting EXAMPLE code" << std::endl;

  nlohmann::json conf ;
  conf["mode"] = "one-fragment-per-file";
  std::string file_path = "/afs/cern.ch/user/a/aabedabu/work_public/hdf5libs_dev/work/build/hdf5libs/apps";
  conf["directory_path"] = file_path ;
  conf["name"] = "tempWriter" ;
  conf["max_file_size_bytes"] = 100000000;
  conf["filename_parameters.overall_prefix"] = "file_prefix_";
  conf["disable_unique_filename_suffix"] = true;
  conf["free_space_safety_factor_for_write"] = 2;
  conf["run_number"] = 52;
  //conf["file_layout_parameters"] = create_file_layout_params();
 
  HDF5RawDataFile h5_raw_data_file = HDF5RawDataFile(conf);

  // =================
  // DUMMY WRITE
  // =================
  // write several events, each with several fragments
  constexpr int dummydata_size = 7;
  const int trigger_count = 5;
  const StorageKey::DataRecordGroupType group_type = StorageKey::DataRecordGroupType::kTPC;
  const int apa_count = 3;
  const int link_count = 1;

  std::array<char, dummydata_size> dummy_data;
  for (int trigger_number = 1; trigger_number <= trigger_count; ++trigger_number) {
    for (int apa_number = 1; apa_number <= apa_count; ++apa_number) {
      for (int link_number = 1; link_number <= link_count; ++link_number) {
        StorageKey key(trigger_number, group_type, apa_number, link_number);
        KeyedDataBlock data_block(key);
        data_block.m_unowned_data_start = static_cast<void*>(&dummy_data[0]);
        data_block.m_data_size = dummydata_size;
        h5_raw_data_file.write(data_block);
      }                   // link number
    }                     // apa number
  }                       // trigger number


  // Get and print attribute names and their values
  auto attributes_map = h5_raw_data_file.get_attributes();
  for (const auto& [k, v] : attributes_map){
    std::cout << k << " : ";
    std::visit([](const auto& x){ std::cout << x; }, v);
    std::cout << '\n';
  }


/*
  std::vector<std::string> datasets_path = decoder.get_fragments(start_tr,num_trs);
  //std::vector<std::string> datasets_path = decoder.get_trh(start_tr,num_trs);
 
  std::cout << "Number of fragments: " << datasets_path.size() << std::endl; 
*/  
  

  
 return 0;
}
