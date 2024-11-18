/**
 * @file HDF5RecoverFile.cpp
 *
 * Preliminary utility to recover a raw data file that did not get closed properly. 
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2024.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "hdf5libs/HDF5RawDataFile.hpp"

#include "daqdataformats/Fragment.hpp"
#include "daqdataformats/SourceID.hpp"
#include "detdataformats/DetID.hpp"
#include "detdataformats/HSIFrame.hpp"
#include "logging/Logging.hpp"
#include "trgdataformats/TriggerObjectOverlay.hpp"

#include <bitset>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

using namespace dunedaq::hdf5libs;
using namespace dunedaq::daqdataformats;
using namespace dunedaq::detdataformats;
using namespace dunedaq::trgdataformats;

void
print_usage(const char *appname)
{
  std::cout << "Usage: " << appname << " [-R] <file_name>" << std::endl;
  std::cout << "The default behavior is to simply print out the changes that need to be made." << std::endl;
  std::cout << "The -R option causes the file to be modifed (Recovered)." << std::endl;
}

int
main(int argc, char** argv)
{
  bool do_recovery = false;
  signed char opt;
  while ((opt = getopt(argc, argv, "hR")) != -1) {
    switch (opt) {
    case 'h':
      print_usage(argv[0]);
      return 1;
    case 'R':
      do_recovery = true;
      break;
    default: /* '?' */
      print_usage(argv[0]);
      return 1;
    }
  }

  argc -= (optind-1);
  argv += (optind-1);
  if (argc != 2) {
    print_usage(argv[0]);
    return 1;
  }

  const std::string ifile_name = std::string(argv[1]);

  // open the file for reading and writing
  HDF5RawDataFile h5_raw_data_file(ifile_name, true);

  size_t last_modified_time = 0;
  struct stat stat_results;
  auto retcode = stat(ifile_name.c_str(), &stat_results);
  if (retcode == 0) {last_modified_time = stat_results.st_mtime;}

  std::string closing_timestamp = h5_raw_data_file.get_attribute("closing_timestamp", std::string(""));

  size_t recorded_size = h5_raw_data_file.get_attribute("recorded_size", SIZE_MAX);

  size_t calculated_recorded_size = 0;
  auto records = h5_raw_data_file.get_all_record_ids();
  for (auto const& record_id : records) {
    if (h5_raw_data_file.is_timeslice_type()) {
      try {
	auto tsh_ptr = h5_raw_data_file.get_tsh_ptr(record_id);
	calculated_recorded_size += sizeof(*tsh_ptr);
      } catch (std::exception const& excpt) {
	// bad record header, we'll skip the whole TimeSlice
	continue;
      }
    } else {
      try {
	auto trh_ptr = h5_raw_data_file.get_trh_ptr(record_id);
	calculated_recorded_size += trh_ptr->get_total_size_bytes();
      } catch (std::exception const& excpt) {
	// bad record header, we'll skip the whole TriggerRecord
	continue;
      }
    }
    std::set<SourceID> frag_sid_list = h5_raw_data_file.get_fragment_source_ids(record_id);
    for (auto const& source_id : frag_sid_list) {
      try {
	auto frag_ptr = h5_raw_data_file.get_frag_ptr(record_id, source_id);
	calculated_recorded_size += frag_ptr->get_size();
      } catch (std::exception const& excpt) {
	// nothing to do, just leave this fragment out of the sum
      }
    }
  }


  std::cout << std::endl;
  std::cout << std::endl;
  std::cout << "========================================" << std::endl;

  if (closing_timestamp == "") {
    std::cout << "The \"closing_timestamp\" Attribute is *not* currently set in the file, and it will be " << std::endl;
    std::cout << "    set to " << last_modified_time << " if/when the file is recovered." << std::endl;
  } else {
    std::cout << "The \"closing_timestamp\" Attribute in the file is currently set to " << closing_timestamp << "." << std::endl;
  }

  if (recorded_size == SIZE_MAX) {
    std::cout << "The \"recorded_size\" Attribute is *not* currently set in the file, and it will be " << std::endl;
    std::cout << "    set to " << calculated_recorded_size << " if/when the file is recovered." << std::endl;
  } else {
    std::cout << "The \"recorded_size\" Attribute in the file is currently set to " << recorded_size << "." << std::endl;
  }

  if (ifile_name.rfind(HDF5RawDataFile::s_inprogress_suffix) != std::string::npos) {
    std::cout << "The file *does* have the \"" << HDF5RawDataFile::s_inprogress_suffix << "\" suffix, "
	      << "so it will be renamed if/when it is recovered." << std::endl;
  } else {
    std::cout << "The file name does not have the \"" << HDF5RawDataFile::s_inprogress_suffix << "\" suffix, "
	      << "so it will not be renamed." << std::endl;
  }

  return 0;
} // NOLINT
