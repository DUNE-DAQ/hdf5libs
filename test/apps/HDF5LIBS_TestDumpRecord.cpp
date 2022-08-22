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

#include "hdf5libs/HDF5RawDataFileSid.hpp"

#include "detdataformats/DetID.hpp"
#include "logging/Logging.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

using namespace dunedaq::hdf5libs;
using namespace dunedaq::daqdataformats;
using namespace dunedaq::detdataformats;

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
  HDF5RawDataFileSid h5_raw_data_file(ifile_name);

  std::ostringstream ss;

  ss << "\nFile name: " << h5_raw_data_file.get_file_name();
  ss << "\n\tRecorded size from class: " << h5_raw_data_file.get_recorded_size();

  auto recorded_size = h5_raw_data_file.get_attribute<size_t>("recorded_size");
  ss << "\n\tRecorded size from attribute: " << recorded_size;

  auto record_type = h5_raw_data_file.get_record_type();
  ss << "\nRecord type = " << record_type;

  // nlohmann::json flp_json;
  // auto flp = h5_raw_data_file.get_file_layout().get_file_layout_params();
  // hdf5filelayout::to_json(flp_json, flp);
  // ss << "\nFile Layout Parameters:\n" << flp_json;

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
    auto trh_ptr = h5_raw_data_file.get_trh_ptr(record_id);
    ss << "\n\tTriggerRecordHeader: " << trh_ptr->get_header();
    std::set<SourceID> frag_sid_list = h5_raw_data_file.get_fragment_source_ids(record_id);
    for (auto const& source_id : frag_sid_list) {
      auto frag_ptr = h5_raw_data_file.get_frag_ptr(record_id, source_id);
      ss << "\n\t" << fragment_type_to_string(frag_ptr->get_fragment_type()) << " fragment from subdetector "
         << DetID::subdetector_to_string(static_cast<DetID::Subdetector>(frag_ptr->get_detector_id()))
         << " has size = " << frag_ptr->get_size();
    }
    TLOG() << ss.str();
    ss.str("");
  }

#if 0
  // 11-Aug-2022, KAB: what value do we get from seeing the header dataset paths?
  auto all_rh_paths = h5_raw_data_file.get_record_header_dataset_paths();
  ss << "\nAll record header datasets found:";
  for (auto const& path : all_rh_paths)
    ss << "\n\t" << path;
  TLOG() << ss.str();
  ss.str("");

  // do this to get all fragment dasets paths...
  //  auto all_frag_paths = h5_raw_data_file.get_all_fragment_dataset_paths();
  //  ss << "\nAll fragment datasets found:";
  //  for (auto const& path : all_frag_paths)
  //  	ss << "\n\t" << path;
  //  TLOG() << ss.str();
  //  ss.str("");

  if (h5_raw_data_file.is_trigger_record_type()) {
    auto trh_ptr = h5_raw_data_file.get_trh_ptr(first_rec);
    ss << "\nTrigger Record Headers:";
    ss << "\nFirst: " << trh_ptr->get_header();
    ss << "\nLast: " << h5_raw_data_file.get_trh_ptr(all_rh_paths.back())->get_header();
  } else if (h5_raw_data_file.is_timeslice_type()) {
    auto tsh_ptr = h5_raw_data_file.get_tsh_ptr(first_rec);
    ss << "\nTimeSlice Headers:";
    ss << "\nFirst: " << *tsh_ptr;
    ss << "\nLast: " << *(h5_raw_data_file.get_tsh_ptr(all_rh_paths.back()));
  }
  TLOG() << ss.str();
  ss.str("");

  // grabbing single fragments at a time...
  //  ss << "\nFragment Headers:";
  //  ss << "\nFirst: " << h5_raw_data_file.get_frag_ptr(first_rec, "TPC", 0, 0)->get_header();
  //  ss << "\nLast: " << h5_raw_data_file.get_frag_ptr(all_frag_paths.back())->get_header();
  //  TLOG() << ss.str();
  //  ss.str("");

  /**
   * Example of walking through a file record by record, 'efficiently'.
   * (1) Get all the records in the file.
   * (2) For a known rid, you can construct the record header dataset path
   *     using the internally-held file layout parameters. Use that to grab the
   *     record header pointer.
   * (3) For a known rid, you could get all of the fragment dataset path names.
   *     Alternatively, as is done here, you can get the set of GeoIDs found in this record.
   *     Then, loop over those and grab the appropriate fragment.
   *     Obviously this allows you to filter out gids that you don't want.
   */
  for (auto const& rid : h5_raw_data_file.get_all_record_ids()) {
    ss << "Processing record (" << rid.first << "," << rid.second << "):";

    auto record_header_dataset = h5_raw_data_file.get_record_header_dataset_path(rid);
    if (h5_raw_data_file.is_trigger_record_type()) {
      auto trh_ptr = h5_raw_data_file.get_trh_ptr(rid);
      ss << "\n\t" << trh_ptr->get_header();
    } else if (h5_raw_data_file.is_timeslice_type()) {
      auto tsh_ptr = h5_raw_data_file.get_tsh_ptr(rid);
      ss << "\n\t" << *tsh_ptr;
    }

    for (auto const& sid : h5_raw_data_file.get_source_ids(rid)) {
      // ss << "\n\t" << sid << ": ";
      auto frag_ptr = h5_raw_data_file.get_frag_ptr(rid, sid);
      ss << "\n\t" << frag_ptr->get_header();
    }

    // could also do loop like this...
    // for(auto const& frag_dataset : h5_raw_data_file.get_fragment_dataset_paths(rid))
    //   auto frag_ptr = h5_raw_data_file.get_frag_ptr(frag_dataset)

    TLOG() << ss.str();
    ss.str("");
  }

  /**
   * Example of walking through a file record by record, reconstructing the record.
   *
   * *** YOU SHOULD NOT DO THIS ***
   * Why not? It may seem convenient. But it requires that you hold all of the fragments
   * in memory. This won't scale well! Instead, use the fact that we can more intelligently
   * grab the data we want, using something like the example above.
   *
   * Still, it was requested, so here it is.
   */
  for (auto const& rid : h5_raw_data_file.get_all_record_ids()) {
    ss << "Processing record (" << rid.first << "," << rid.second << "):";

    if (h5_raw_data_file.is_trigger_record_type()) {
      auto record = h5_raw_data_file.get_trigger_record(rid);
      auto const& trh = record.get_header_ref();
      ss << "\n\t" << trh.get_header();
      for (auto const& frag_ptr : record.get_fragments_ref()) {
        // ss << "\n\t" << frag_ptr->get_element_id();
        ss << "\n\t" << frag_ptr->get_header();
      }
    } else if (h5_raw_data_file.is_timeslice_type()) {
      auto record = h5_raw_data_file.get_timeslice(rid);
      auto const& tsh = record.get_header();
      ss << "\n\t" << tsh;
      for (auto const& frag_ptr : record.get_fragments_ref()) {
        // ss << "\n\t" << frag_ptr->get_element_id();
        ss << "\n\t" << frag_ptr->get_header();
      }
    }
    TLOG() << ss.str();
    ss.str("");
  }
#endif

  return 0;
} // NOLINT
