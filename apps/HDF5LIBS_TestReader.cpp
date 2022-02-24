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
#include "hdf5libs/hdf5filelayout/Nljs.hpp"


using namespace dunedaq::hdf5libs;
using namespace dunedaq::daqdataformats;

void print_usage()
{
  std::cout << "Usage: HDF5LIBS_TestReader <input_file_name>" << std::endl;
}

int main(int argc, char** argv){

  if(argc!=2){
    print_usage();
    return 1;
  }

  const std::string ifile_name = std::string(argv[1]);


  //open our file reading
  HDF5RawDataFile h5_raw_data_file = HDF5RawDataFile(ifile_name);

  std::cout << "File name: " << h5_raw_data_file.get_file_name() << std::endl;
  std::cout << "\tRecorded size from class: " << h5_raw_data_file.get_recorded_size() << std::endl;

  auto recorded_size = h5_raw_data_file.get_attribute<size_t>("recorded_size");
  std::cout << "\tRecorded size from attribute: " << recorded_size << std::endl;

  nlohmann::json flp_json;
  auto flp = h5_raw_data_file.get_file_layout().get_file_layout_params();
  hdf5filelayout::to_json(flp_json,flp);
  std::cout << "File Layout Parameters:\n" << flp_json;
  std::cout << std::endl;


  //get some file attributes
  auto run_number = h5_raw_data_file.get_attribute<unsigned int>("run_number");
  auto file_index = h5_raw_data_file.get_attribute<unsigned int>("file_index");
  auto creation_timestamp = h5_raw_data_file.get_attribute<std::string>("creation_timestamp");
  auto app_name = h5_raw_data_file.get_attribute<std::string>("application_name");

  std::cout << "\n\tRun number: " << run_number;
  std::cout << "\n\tFile index: " << file_index;
  std::cout << "\n\tCreation timestamp: " << creation_timestamp;
  std::cout << "\n\tWriter app name: " << app_name;
  std::cout << std::endl;
  
  auto trigger_records = h5_raw_data_file.get_all_trigger_record_numbers();
  std::cout << "Number of trigger records: " << trigger_records.size() << std::endl;
  if(trigger_records.empty()){
    std::cout << "NO TRIGGER RECORDS FOUND" << std::endl;
    return 0;
  }
  std::cout << "\tFirst record: " << *(trigger_records.begin()) << std::endl;
  std::cout << "\tLast record: " << *(std::next(trigger_records.begin(),trigger_records.size()-1)) << std::endl;


  auto all_datasets = h5_raw_data_file.get_dataset_paths();
  std::cout << "All datasets found:";
  for(auto const& path : all_datasets)
    std::cout << "\n\t" << path;
  std::cout << std::endl;

  auto all_trh_paths = h5_raw_data_file.get_trigger_record_header_dataset_paths();
  std::cout << "All trigger record header datasets found:";
  for(auto const& path : all_trh_paths)
    std::cout << "\n\t" << path;
  std::cout << std::endl;

  auto all_frag_paths = h5_raw_data_file.get_all_fragment_dataset_paths();
  std::cout << "All fragment datasets found:";
  for(auto const& path : all_frag_paths)
    std::cout << "\n\t" << path;
  std::cout << std::endl;

  auto first_trig_num = *(trigger_records.begin());

  auto trh_ptr = h5_raw_data_file.get_trh_ptr(first_trig_num);
  std::cout << "Trigger Record Headers:" << std::endl;
  std::cout << "First: " << trh_ptr->get_header() << std::endl;
  std::cout << "Last: " << h5_raw_data_file.get_trh_ptr(all_trh_paths.back())->get_header() << std::endl;
  
  std::cout << "Fragment Headers:" << std::endl;
  std::cout << "First: " << h5_raw_data_file.get_frag_ptr(first_trig_num,"TPC",0,0)->get_header() << std::endl;
  std::cout << "Last: " << h5_raw_data_file.get_frag_ptr(all_frag_paths.back())->get_header() << std::endl;


  return 0;
}
