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
#include <chrono>
#include <ctime>

using namespace std::chrono;
using namespace dunedaq::hdf5libs;
using namespace dunedaq::daqdataformats;
using namespace dunedaq::detdataformats;

void print_usage()
{
  TLOG() << "Usage: filetransform_file_to_file_test <configuration_file> <detector_readout_map_file> <input_file_name> <output_file_name> <chunk_size>";
}

int main(int argc, char **argv)
{

  if (argc != 6)
  {
    print_usage();
    return 1;
  }

  const std::string app_name = std::string(argv[0]);
  const std::string conf_file_name = std::string(argv[1]);
  const std::string hw_map_file_name = std::string(argv[2]);
  const std::string ifile_name = std::string(argv[3]);
  const std::string ofile_name = std::string(argv[4]);

  // read in configuration
  nlohmann::json j_in, fl_conf;
  std::ifstream conf_file(conf_file_name);
  conf_file >> j_in;
  conf_file.close();

  // get file_layout config
  try
  {
    fl_conf = j_in["file_layout"].get<hdf5filelayout::FileLayoutParams>();
    // TLOG() << "Read 'file_layout' configuration:\n";
    // TLOG() << fl_conf;
  }
  catch (...)
  {
    TLOG() << "ERROR: Improper 'file_layout' configuration in " << fl_conf;
    return 1;
  }

  // read test writer app configs
  const int run_number = j_in["run_number"].get<int>();
  const int file_index = j_in["file_index"].get<int>();

  const int trigger_count = j_in["trigger_count"].get<int>();
  const long int data_size = j_in["data_size"].get<long int>();
  const long int fragment_size = data_size + sizeof(FragmentHeader);
  const SourceID::Subsystem stype_to_use = SourceID::string_to_subsystem(j_in["subsystem_type"].get<std::string>());
  const DetID::Subdetector dtype_to_use = DetID::string_to_subdetector(j_in["subdetector_type"].get<std::string>());
  const FragmentType ftype_to_use = string_to_fragment_type(j_in["fragment_type"].get<std::string>());
  const int element_count = j_in["element_count"].get<int>();

  // TLOG() << "\nOutput file: " << ofile_name << "\nRun number: " << run_number << "\nFile index: " << file_index
  //        << "\nNumber of trigger records: " << trigger_count << "\nNumber of fragments: " << element_count
  //        << "\nSubsystem: " << SourceID::subsystem_to_string(stype_to_use)
  //        << "\nFragment size (bytes, incl. header): " << fragment_size;

  // Read hardware map
  std::ifstream f(hw_map_file_name);
  nlohmann::json data = nlohmann::json::parse(R"(
    [
    {
      "source_id": 0,
      "geo_id": {
        "det_id": 3,
        "crate_id": 1,
        "slot_id": 0,
        "stream_id": 0
      }
    },
    {
      "source_id": 1,
      "geo_id": {
        "det_id": 3,
        "crate_id": 1,
        "slot_id": 0,
        "stream_id": 1
      }
    },
    {
      "source_id": 3,
      "geo_id": {
        "det_id": 3,
        "crate_id": 1,
        "slot_id": 1,
        "stream_id": 0
      }
    },
    {
      "source_id": 4,
      "geo_id": {
        "det_id": 3,
        "crate_id": 1,
        "slot_id": 1,
        "stream_id": 1
      }
    },
    {
      "source_id": 4,
      "geo_id": {
        "det_id": 2,
        "crate_id": 1,
        "slot_id": 0,
        "stream_id": 0
      }
    },
    {
      "source_id": 5,
      "geo_id": {
        "det_id": 2,
        "crate_id": 1,
        "slot_id": 0,
        "stream_id": 1
      }
    },
    {
      "source_id": 6,
      "geo_id": {
        "det_id": 2,
        "crate_id": 1,
        "slot_id": 1,
        "stream_id": 0
      }
    },
    {
      "source_id": 7,
      "geo_id": {
        "det_id": 2,
        "crate_id": 1,
        "slot_id": 1,
        "stream_id": 1
      }
    }
  ]
  )");

  auto srcid_geoid_map = data.get<hdf5rawdatafile::SrcIDGeoIDMap>();

  // open our file for writing
  HDF5RawDataFile h5_raw_data_file = HDF5RawDataFile(ofile_name,
                                                     run_number, // run_number
                                                     file_index, // file_index,
                                                     app_name,   // app_name
                                                     fl_conf,    // file_layout_confs
                                                     srcid_geoid_map,
                                                     ".writing",                 // optional: suffix to use for files being written
                                                     HighFive::File::Overwrite); // optional: overwrite existing file

  // read fragment data
  const int buffer_size = std::stoi(argv[5]);
  char *buffer = new char[buffer_size];
  for (unsigned int i = 0; i < buffer_size; i++)
  {
    buffer[i] = rand() % 255;
  }

  // Start time measurement
  auto start_time = high_resolution_clock::now();
  auto avg_time_tr = 0.0;
  auto last_time = start_time;

  // loop over desired number of triggers
  for (int trig_num = 1; trig_num <= trigger_count; ++trig_num)
  {

    // get a timestamp for this trigger
    uint64_t ts = std::chrono::duration_cast<std::chrono::milliseconds>( // NOLINT(build/unsigned)
                      system_clock::now().time_since_epoch())
                      .count();

    // TLOG() << "\tWriting trigger " << trig_num << " with time_stamp " << ts;

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

    // the source_id_path map that we will build up as we write the TR header
    // and fragments (and then write the map into the HDF5 TR_record Group)
    HDF5SourceIDHandler::source_id_path_map_t source_id_path_map;

    // the map of fragment types to SourceIDS
    HDF5SourceIDHandler::fragment_type_source_id_map_t fragment_type_source_id_map;

    // the map of subdetectors to SourceIDS
    HDF5SourceIDHandler::subdetector_source_id_map_t subdetector_source_id_map;

    // write the record header into the HDF5 file/group
    HighFive::Group record_level_group;

    // create out TriggerRecord
    TriggerRecord tr(trh);

    h5_raw_data_file.write_header_only(tr, source_id_path_map, record_level_group);

    auto start_time_tr = high_resolution_clock::now();
    // loop over elements
    for (int ele_num = 0; ele_num < element_count; ++ele_num)
    {

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

      int split_count = ceil((data_size + sizeof(FragmentHeader)) / (double)buffer_size);
      int buffer_size_first = buffer_size - (sizeof(FragmentHeader) - buffer_size * int(sizeof(FragmentHeader) / buffer_size));
      int buffer_size_last = (data_size + sizeof(FragmentHeader)) % buffer_size;

      if (buffer_size_last == 0)
        buffer_size_last = buffer_size;

      HighFive::DataSet dataset = h5_raw_data_file.write_fragment_begin(fh, data_size + sizeof(FragmentHeader), buffer_size, true);

      // TLOG() << "Writing fragment " << ele_num << " with size " << data_size + sizeof(FragmentHeader) << " bytes, split into " << split_count << " parts";
      // TLOG() << "First fragment part size: " << buffer_size_first << " bytes";
      // TLOG() << "Last fragment part size: " << buffer_size_last << " bytes";

      // write header
      h5_raw_data_file.write_fragment_part(dataset, reinterpret_cast<const char *>(&fh), 0, sizeof(FragmentHeader));

      for (int frag_num = 0; frag_num < split_count; frag_num++)
      {
        if (frag_num == 0)
        {
          h5_raw_data_file.write_fragment_part(dataset, buffer, sizeof(FragmentHeader), buffer_size_first);
        }
        else if (frag_num == split_count - 1)
        {
          h5_raw_data_file.write_fragment_part(dataset, buffer, frag_num * buffer_size, buffer_size_last);
        }
        else
          h5_raw_data_file.write_fragment_part(dataset, buffer, frag_num * buffer_size, buffer_size);
      }

      h5_raw_data_file.write_fragment_end(fragment_type_source_id_map, subdetector_source_id_map, source_id_path_map, dataset.getPath(), fh);
    } // end loop over elements

    // write attributes of the trigger record
    h5_raw_data_file.write_end_tr(source_id_path_map, record_level_group, fragment_type_source_id_map, subdetector_source_id_map);

    auto stop_time_tr = high_resolution_clock::now();
    avg_time_tr += duration_cast<microseconds>(stop_time_tr - start_time_tr).count();

  } // end loop over triggers

  avg_time_tr /= trigger_count;

  auto stop_time = high_resolution_clock::now();
  std::time_t end_time = std::chrono::system_clock::to_time_t(stop_time);

  // print current date for reference
  std::cout << std::endl
            << "Test finished at " << std::ctime(&end_time);
  // print parameters of test
  std::cout << "trigger_count=" << trigger_count << ",data_size=" << data_size << ",fragment_size=" << fragment_size << ",element_count=" << element_count << std::endl;
  std::cout << "Finished writing to file=" << h5_raw_data_file.get_file_name() << std::endl;
  std::cout << "Recorded size (Bytes)=" << h5_raw_data_file.get_recorded_size() << std::endl;
  std::cout << "Average time to write a trigger record (s)=" << avg_time_tr / 10e6 << std::endl;
  std::cout << "Total time (s)=" << duration_cast<microseconds>(stop_time - start_time).count() / 10e6 << std::endl;
  std::cout << "Average write rate (MiB/s)=" << (h5_raw_data_file.get_recorded_size() / duration_cast<microseconds>(stop_time - start_time).count()) << std::endl;
  std::cout << "Average trigger record write rate (MiB/s)=" << (data_size / avg_time_tr * element_count) << std::endl;
  std::cout << h5_raw_data_file.get_recorded_size() << ";" << avg_time_tr / 10e6 << ";" << duration_cast<microseconds>(stop_time - start_time).count() / 10e6 << ";" << (h5_raw_data_file.get_recorded_size() / duration_cast<microseconds>(stop_time - start_time).count()) << ";" << (data_size / avg_time_tr * element_count) << std::endl;
  delete[] buffer;
  return 0;
}