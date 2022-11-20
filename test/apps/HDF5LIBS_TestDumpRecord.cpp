/**
 * @file HDF5TestDumpRecord.cpp
 *
 * Demo of HDF5 file reader for TPC fragments: this example demonstrates
 * simple 'record-dump' functionality.
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "hdf5libs/HDF5RawDataFile.hpp"

#include "detdataformats/DetID.hpp"
#include "logging/Logging.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

using namespace dunedaq::hdf5libs;
using namespace dunedaq::daqdataformats;
using namespace dunedaq::detdataformats;
using namespace dunedaq::detchannelmaps;

void
print_usage()
{
  TLOG() << "Usage: HDF5LIBS_TestDumpRecord <input_file_name>";
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

  auto records = h5_raw_data_file.get_all_record_ids();
  ss << "\nNumber of records: " << records.size();
  if (records.empty()) {
    ss << "\n\nNO TRIGGER RECORDS FOUND";
    TLOG() << ss.str();
    return 0;
  }
  auto first_rec = *(records.begin());
  auto last_rec = *(std::next(records.begin(), records.size() - 1));

  ss << "\n\tFirst record: " << first_rec.first << "," << first_rec.second;
  ss << "\n\tLast record: " << last_rec.first << "," << last_rec.second;

  TLOG() << ss.str();
  ss.str("");

  for (auto const& record_id : records) {
    if (h5_raw_data_file.is_timeslice_type()) {
      auto tsh_ptr = h5_raw_data_file.get_tsh_ptr(record_id);
      ss << "\n\tTimeSliceHeader: " << *tsh_ptr;
    } else {
      auto trh_ptr = h5_raw_data_file.get_trh_ptr(record_id);
      ss << "\n\tTriggerRecordHeader: " << trh_ptr->get_header();
    }
    TLOG() << ss.str();
    ss.str("");
    bool first_frag = true;
    std::set<SourceID> frag_sid_list = h5_raw_data_file.get_fragment_source_ids(record_id);
    for (auto const& source_id : frag_sid_list) {
      if (first_frag) {first_frag = false;}
      else {ss << "\n";}
      auto frag_ptr = h5_raw_data_file.get_frag_ptr(record_id, source_id);
      ss << "\t" << fragment_type_to_string(frag_ptr->get_fragment_type()) << " fragment with SourceID "
         << frag_ptr->get_element_id().to_string() << " from subdetector "
         << DetID::subdetector_to_string(static_cast<DetID::Subdetector>(frag_ptr->get_detector_id()))
         << " has size = " << frag_ptr->get_size();
      if (frag_ptr->get_element_id().subsystem == SourceID::Subsystem::kDetectorReadout) {
        ss << "\n\t\t"
           << "It may contain data from the following detector components:";
        std::vector<uint64_t> geo_id_list = h5_raw_data_file.get_geo_ids_for_source_id(record_id, source_id);
        for (auto const& geo_id : geo_id_list) {
          HardwareMapService::GeoInfo geo_info = HardwareMapService::parse_geo_id(geo_id);
          ss << "\n\t\t\t"
             << "subdetector " << DetID::subdetector_to_string(static_cast<DetID::Subdetector>(geo_info.det_id))
             << ", crate " << geo_info.det_crate << ", slot " << geo_info.det_slot << ", link " << geo_info.det_link;
        }
      }
    }
    std::cout << ss.str() << std::endl;
    ss.str("");
  }

  return 0;
} // NOLINT
