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

#include "daqdataformats/Fragment.hpp"
#include "detdataformats/DetID.hpp"
#include "detdataformats/HSIFrame.hpp"
#include "logging/Logging.hpp"
#include "hdf5libs/hdf5rawdatafile/Structs.hpp"
#include "hdf5libs/hdf5rawdatafile/Nljs.hpp"
#include "trgdataformats/TriggerObjectOverlay.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

using namespace dunedaq::hdf5libs;
using namespace dunedaq::daqdataformats;
using namespace dunedaq::detdataformats;
using namespace dunedaq::trgdataformats;

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
      try {
        auto trh_ptr = h5_raw_data_file.get_trh_ptr(record_id);
        ComponentRequest cr = trh_ptr->get_component_for_source_id(frag_ptr->get_element_id());
        ss << "\n\t\t"
           << "Readout window before = " << (trh_ptr->get_trigger_timestamp()-cr.window_begin)
           << ", after = " << (cr.window_end-trh_ptr->get_trigger_timestamp());
      }
      catch (std::exception const& excpt) {
        ss << "\n\t\t"
           << "Unable to determine readout window, exception was \"" << excpt.what() << "\"";
      }
      if (frag_ptr->get_element_id().subsystem == SourceID::Subsystem::kDetectorReadout) {
        ss << "\n\t\t"
           << "It may contain data from the following detector components:";
        std::vector<uint64_t> geo_id_list = h5_raw_data_file.get_geo_ids_for_source_id(record_id, source_id);
        for (auto const& geo_id : geo_id_list) {
          // FIXME
          // GeoInfo = HardwareMapService::parse_geo_id(geo_id);
          uint16_t det_id   =  geo_id & 0xffff;
          uint16_t crate_id = (geo_id >> 16)& 0xffff;
          uint16_t slot_id  = (geo_id >> 32) & 0xffff;
          uint16_t link_id  = (geo_id >> 48) & 0xffff;
          ss << "\n\t\t\t"
             << "subdetector " << DetID::subdetector_to_string(static_cast<DetID::Subdetector>(det_id))
             << ", crate " << crate_id << ", slot " << slot_id << ", link " << link_id;
        }
      }
      if (frag_ptr->get_data_size() == 0) {
        ss << "\n\t\t" << "*** Empty fragment! Moving to next fragment. ***";
        continue;
      }
      if (frag_ptr->get_fragment_type() == FragmentType::kTriggerCandidate) {
        TriggerCandidate* tcptr = static_cast<TriggerCandidate*>(frag_ptr->get_data());
        ss << "\n\t\t" << "TC type = " << get_trigger_candidate_type_names()[tcptr->data.type]
           << " (" << static_cast<int>(tcptr->data.type) << "), TC algorithm = "
           << static_cast<int>(tcptr->data.algorithm) << ", number of TAs = " << tcptr->n_inputs;
        ss << "\n\t\t" << "Start time = " << tcptr->data.time_start << ", end time = " << tcptr->data.time_end
           << ", and candidate time = " << tcptr->data.time_candidate;
      }
      if (frag_ptr->get_fragment_type() == FragmentType::kHardwareSignal) {
        HSIFrame* hsi_ptr = static_cast<HSIFrame*>(frag_ptr->get_data());
        ss << "\n\t\t" << "Detector ID = " << hsi_ptr->detector_id
           << ", Crate = " << hsi_ptr->crate
           << ", Slot = " << hsi_ptr->slot
           << ", Link = " << hsi_ptr->link;
        ss << ",\n\t\t" << "Sequence = " << hsi_ptr->sequence
           << ", Trigger = " << hsi_ptr->trigger
           << ", Version = " << hsi_ptr->version;
        ss << ",\n\t\t" << "Timestamp = " << hsi_ptr->get_timestamp();

        // Finding the bit positions for input_low and input_high
        uint32_t bit_pos, bit_sniff;
        uint32_t input_low = hsi_ptr->input_low;
        ss << ",\n\t\t" << "Input Low Bitmap = " << input_low;
        if (input_low != 0) { // Skip printing the positions if the value is 0.
          ss << ", Input Low Bit Positions = ";
          bit_sniff = 1;
          for (bit_pos = 0; bit_pos < 32; bit_pos++) {
            if (input_low & bit_sniff) {
              bit_sniff = bit_sniff << 1;
              ss << bit_pos << " ";
            }
          }
        }

        uint32_t input_high = hsi_ptr->input_high;
        ss << ",\n\t\t" << "Input High Bitmap = " << input_high;
        if (input_high != 0) {
          ss << ", Input High Bit Positions = ";
          bit_sniff = 1;
          for (bit_pos = 0; bit_pos < 32; bit_pos++) {
            if (input_high & bit_sniff) {
              bit_sniff = bit_sniff << 1;
              ss << bit_pos << " ";
            }
          }
        }
        ss << ".";  // Finishes the HSI section.
      }
    }
    std::cout << ss.str() << std::endl;
    ss.str("");
  }

  return 0;
} // NOLINT
