/**
 * @file demo_tpc_decoder.cpp
 *
 * Demo of HDF5 file reader for TPC fragments: this example shows how to extract fragments from a file and decode WIB
 * frames.
 *
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */


#include "hdf5libs/HDF5RawDataFile.hpp"
#include "hdf5libs/hdf5filelayout/Nljs.hpp"

#include "logging/Logging.hpp"

#include <fstream>
#include <iostream>
#include <string>
#include <sstream>

using namespace dunedaq::hdf5libs;
using namespace dunedaq::daqdataformats;

void
print_usage()
{
  TLOG() << "Usage: HDF5LIBS_TestReader <input_file_name>";
}

int
main(int argc, char** argv)
{

  if (argc != 2) {
    print_usage();
    return 1;
  }

  const std::string ifile_name = std::string(argv[1]);

  // open our file reading
  HDF5RawDataFile h5_raw_data_file(ifile_name);

  std::ostringstream ss;
  
  ss << "\nFile name: " << h5_raw_data_file.get_file_name();
  ss << "\n\tRecorded size from class: " << h5_raw_data_file.get_recorded_size();

  auto recorded_size = h5_raw_data_file.get_attribute<size_t>("recorded_size");
  ss << "\n\tRecorded size from attribute: " << recorded_size;

  auto record_type = h5_raw_data_file.get_record_type();
  ss << "\nRecord type = " << record_type;
  
  nlohmann::json flp_json;
  auto flp = h5_raw_data_file.get_file_layout().get_file_layout_params();
  hdf5filelayout::to_json(flp_json, flp);
  ss << "\nFile Layout Parameters:\n" << flp_json;

  TLOG() << ss.str();
  ss.str("");
  
  // get some file attributes
  auto run_number = h5_raw_data_file.get_attribute<unsigned int>("run_number");
  auto file_index = h5_raw_data_file.get_attribute<unsigned int>("file_index");
  auto creation_timestamp = h5_raw_data_file.get_attribute<std::string>("creation_timestamp");
  auto app_name = h5_raw_data_file.get_attribute<std::string>("application_name");

  ss << "\n\tRun number: " << run_number;
  ss << "\n\tFile index: " << file_index;
  ss << "\n\tCreation timestamp: " << creation_timestamp;
  ss << "\n\tWriter app name: " << app_name;

  TLOG() << ss.str();
  ss.str("");

  auto records = h5_raw_data_file.get_all_record_numbers();
  ss << "\nNumber of records: " << records.size();
  if (records.empty()) {
    ss << "\n\nNO TRIGGER RECORDS FOUND";
    TLOG() << ss.str();
    return 0;
  }
  ss << "\n\tFirst record: " << *(records.begin());
  ss << "\n\tLast record: " << *(std::next(records.begin(), records.size() - 1));

  TLOG() << ss.str();
  ss.str("");
    
  auto all_datasets = h5_raw_data_file.get_dataset_paths();
  ss << "\nAll datasets found:";
  for (auto const& path : all_datasets)
    ss << "\n\t" << path;

  TLOG() << ss.str();
  ss.str("");

  auto all_rh_paths = h5_raw_data_file.get_record_header_dataset_paths();
  ss << "\nAll record header datasets found:";
  for (auto const& path : all_rh_paths)
    ss << "\n\t" << path;

  TLOG() << ss.str();
  ss.str("");

  auto all_frag_paths = h5_raw_data_file.get_all_fragment_dataset_paths();
  ss << "\nAll fragment datasets found:";
  for (auto const& path : all_frag_paths)
    ss << "\n\t" << path;

  TLOG() << ss.str();
  ss.str("");

  auto first_rec_num = *(records.begin());

  if(h5_raw_data_file.is_trigger_record_type()){
    auto trh_ptr = h5_raw_data_file.get_trh_ptr(first_rec_num,0);
    ss << "\nTrigger Record Headers:";
    ss << "\nFirst: " << trh_ptr->get_header();
    ss << "\nLast: " << h5_raw_data_file.get_trh_ptr(all_rh_paths.back())->get_header();
  }
  else if(h5_raw_data_file.is_timeslice_type()){
    auto tsh_ptr = h5_raw_data_file.get_tsh_ptr(first_rec_num);
    ss << "\nTimeSlice Headers:";
    ss << "\nFirst: " << *tsh_ptr;
    ss << "\nLast: " << *(h5_raw_data_file.get_tsh_ptr(all_rh_paths.back()));
  }
  TLOG() << ss.str();
  ss.str("");

  ss << "\nFragment Headers:";
  ss << "\nFirst: " << h5_raw_data_file.get_frag_ptr(first_rec_num, 0, "TPC", 0, 0)->get_header();
  ss << "\nLast: " << h5_raw_data_file.get_frag_ptr(all_frag_paths.back())->get_header();
  TLOG() << ss.str();
  ss.str("");

  return 0;
}
