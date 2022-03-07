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

  TLOG() << "File name: " << h5_raw_data_file.get_file_name();
  TLOG() << "\tRecorded size from class: " << h5_raw_data_file.get_recorded_size();

  auto recorded_size = h5_raw_data_file.get_attribute<size_t>("recorded_size");
  TLOG() << "\tRecorded size from attribute: " << recorded_size;

  nlohmann::json flp_json;
  auto flp = h5_raw_data_file.get_file_layout().get_file_layout_params();
  hdf5filelayout::to_json(flp_json, flp);
  TLOG() << "File Layout Parameters:\n" << flp_json;
  TLOG();

  // get some file attributes
  auto run_number = h5_raw_data_file.get_attribute<unsigned int>("run_number");
  auto file_index = h5_raw_data_file.get_attribute<unsigned int>("file_index");
  auto creation_timestamp = h5_raw_data_file.get_attribute<std::string>("creation_timestamp");
  auto app_name = h5_raw_data_file.get_attribute<std::string>("application_name");

  TLOG() << "\n\tRun number: " << run_number;
  TLOG() << "\n\tFile index: " << file_index;
  TLOG() << "\n\tCreation timestamp: " << creation_timestamp;
  TLOG() << "\n\tWriter app name: " << app_name;
  TLOG();

  auto trigger_records = h5_raw_data_file.get_all_trigger_record_numbers();
  TLOG() << "Number of trigger records: " << trigger_records.size();
  if (trigger_records.empty()) {
    TLOG() << "NO TRIGGER RECORDS FOUND";
    return 0;
  }
  TLOG() << "\tFirst record: " << *(trigger_records.begin());
  TLOG() << "\tLast record: " << *(std::next(trigger_records.begin(), trigger_records.size() - 1));

  auto all_datasets = h5_raw_data_file.get_dataset_paths();
  TLOG() << "All datasets found:";
  for (auto const& path : all_datasets)
    TLOG() << "\n\t" << path;
  TLOG();

  auto all_trh_paths = h5_raw_data_file.get_trigger_record_header_dataset_paths();
  TLOG() << "All trigger record header datasets found:";
  for (auto const& path : all_trh_paths)
    TLOG() << "\n\t" << path;
  TLOG();

  auto all_frag_paths = h5_raw_data_file.get_all_fragment_dataset_paths();
  TLOG() << "All fragment datasets found:";
  for (auto const& path : all_frag_paths)
    TLOG() << "\n\t" << path;
  TLOG();

  auto first_trig_num = *(trigger_records.begin());

  auto trh_ptr = h5_raw_data_file.get_trh_ptr(first_trig_num,0);
  TLOG() << "Trigger Record Headers:";
  TLOG() << "First: " << trh_ptr->get_header();
  TLOG() << "Last: " << h5_raw_data_file.get_trh_ptr(all_trh_paths.back())->get_header();

  TLOG() << "Fragment Headers:";
  TLOG() << "First: " << h5_raw_data_file.get_frag_ptr(first_trig_num, 0, "TPC", 0, 0)->get_header();
  TLOG() << "Last: " << h5_raw_data_file.get_frag_ptr(all_frag_paths.back())->get_header();

  return 0;
}
