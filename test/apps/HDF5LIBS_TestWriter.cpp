/**
 * @file HDF5LIBS_TestWriter.cpp
 *
 * Demo of HDF5 file writer.
 *
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "hdf5libs/HDF5RawDataFile.hpp"
#include "hdf5libs/hdf5filelayout/Nljs.hpp"
#include "hdf5libs/hdf5rawdatafile/Nljs.hpp"

#include "detdataformats/DetID.hpp"
#include "logging/Logging.hpp"

#include <nlohmann/json.hpp>

#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace dunedaq::hdf5libs;
using namespace dunedaq::daqdataformats;
using namespace dunedaq::detdataformats;

void
print_usage()
{
  TLOG() << "Usage: HDF5LIBS_TestWriter <configuration_file> <hardware_map_file> <output_file_name>";
}

int
main(int argc, char** argv)
{

  if (argc != 4) {
    print_usage();
    return 1;
  }

  const std::string app_name = std::string(argv[0]);
  const std::string ifile_name = std::string(argv[1]);
  const std::string hw_map_file_name = std::string(argv[2]);
  const std::string ofile_name = std::string(argv[3]);

  // read in configuration
  nlohmann::json j_in, fl_conf;
  std::ifstream ifile(ifile_name);
  ifile >> j_in;
  ifile.close();

  // get file_layout config
  try {
    fl_conf = j_in["file_layout"].get<hdf5filelayout::FileLayoutParams>();
    TLOG() << "Read 'file_layout' configuration:\n";
    TLOG() << fl_conf;
  } catch (...) {
    TLOG() << "ERROR: Improper 'file_layout' configuration in " << ifile_name;
    return 1;
  }

  // read test writer app configs
  const int run_number = j_in["run_number"].get<int>();
  const int file_index = j_in["file_index"].get<int>();

  const int trigger_count = j_in["trigger_count"].get<int>();
  const int fragment_size = j_in["data_size"].get<int>() + sizeof(FragmentHeader);
  const SourceID::Subsystem stype_to_use = SourceID::string_to_subsystem(j_in["subsystem_type"].get<std::string>());
  const DetID::Subdetector dtype_to_use = DetID::string_to_subdetector(j_in["subdetector_type"].get<std::string>());
  const FragmentType ftype_to_use = string_to_fragment_type(j_in["fragment_type"].get<std::string>());
  const int element_count = j_in["element_count"].get<int>();

  TLOG() << "\nOutput file: " << ofile_name << "\nRun number: " << run_number << "\nFile index: " << file_index
         << "\nNumber of trigger records: " << trigger_count << "\nNumber of fragments: " << element_count
         << "\nSubsystem: " << SourceID::subsystem_to_string(stype_to_use)
         << "\nFragment size (bytes, incl. header): " << fragment_size;

  // create the HardwareMapService
  // std::shared_ptr<dunedaq::detchannelmaps::HardwareMapService> hw_map_service(
  //   new dunedaq::detchannelmaps::HardwareMapService(hw_map_file_name));

  std::ifstream f(hw_map_file_name);
  nlohmann::json data = nlohmann::json::parse(f);

  // hdf5rawdatafile::SrcIDGeoIDMap srcid_geoid_map;
  // for (nlohmann::json::iterator it = data.begin(); it != data.end(); ++it) {
  //   srcid_geoid_map.push_back(it->get<hdf5rawdatafile::SrcIDGeoIDEntry>())
  // }
  auto srcid_geoid_map = data.get<hdf5rawdatafile::SrcIDGeoIDMap>();

  // open our file for writing
  HDF5RawDataFile h5_raw_data_file = HDF5RawDataFile(ofile_name,
                                                     run_number, // run_number
                                                     file_index, // file_index,
                                                     app_name,   // app_name
                                                     fl_conf,    // file_layout_confs
                                                     srcid_geoid_map,
                                                     ".writing", // optional: suffix to use for files being written
                                                     HighFive::File::Overwrite); // optional: overwrite existing file

  std::vector<char> dummy_data(fragment_size);

  // loop over desired number of triggers
  for (int trig_num = 1; trig_num <= trigger_count; ++trig_num) {

    // get a timestamp for this trigger
    uint64_t ts = std::chrono::duration_cast<std::chrono::milliseconds>( // NOLINT(build/unsigned)
                    system_clock::now().time_since_epoch())
                    .count();

    TLOG() << "\tWriting trigger " << trig_num << " with time_stamp " << ts;

    // create TriggerRecordHeader
    TriggerRecordHeaderData trh_data;
    trh_data.trigger_number = trig_num;
    trh_data.trigger_timestamp = ts;
    trh_data.num_requested_components = element_count;
    trh_data.run_number = run_number;
    trh_data.sequence_number = 0;
    trh_data.max_sequence_number = 1;
    trh_data.element_id = SourceID(SourceID::Subsystem::kTRBuilder, 0);

    TriggerRecordHeader trh(&trh_data);

    // create out TriggerRecord
    TriggerRecord tr(trh);

    // loop over elements
    for (int ele_num = 0; ele_num < element_count; ++ele_num) {

      // create our fragment
      FragmentHeader fh;
      fh.trigger_number = trig_num;
      fh.trigger_timestamp = ts;
      fh.window_begin = ts - 10;
      fh.window_end = ts;
      fh.run_number = run_number;
      fh.fragment_type = static_cast<fragment_type_t>(ftype_to_use);
      fh.sequence_number = 0;
      fh.detector_id = static_cast<uint16_t>(dtype_to_use);
      fh.element_id = SourceID(stype_to_use, ele_num);

      auto frag_ptr = std::make_unique<Fragment>(dummy_data.data(), dummy_data.size());
      frag_ptr->set_header_fields(fh);

      // add fragment to TriggerRecord
      tr.add_fragment(std::move(frag_ptr));

    } // end loop over elements

    // write trigger record to file
    h5_raw_data_file.write(tr);

  } // end loop over triggers

  TLOG() << "Finished writing to file " << h5_raw_data_file.get_file_name();
  TLOG() << "Recorded size: " << h5_raw_data_file.get_recorded_size();

  return 0;
}
