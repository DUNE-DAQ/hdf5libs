/**
 * @file HDF5WriteReadTriggerRecord_test.cxx Application that tests and demonstrates
 * the write/read functions of the HDF5RawDataFile class.
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "hdf5libs/HDF5RawDataFile.hpp"
#include "hdf5libs/test/HDF5TestUtils.hpp"

#include "detdataformats/DetID.hpp"

#define BOOST_TEST_MODULE HDF5WriteReadTriggerRecord_test // NOLINT

#include "boost/test/unit_test.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <regex>
#include <string>
#include <utility>
#include <vector>

using namespace dunedaq::hdf5libs;

constexpr int run_number = 53;
constexpr int file_index = 0;
const std::string application_name = "HDF5WriteReadTriggerRecord_test";
constexpr size_t fragment_size = 100;
constexpr size_t element_count_tpc = 4;
constexpr size_t element_count_pds = 4;
constexpr size_t element_count_ta = 4;
constexpr size_t element_count_tc = 1;
size_t compressed_raw_data_size;
size_t uncompressed_raw_data_size;

const size_t components_per_record = element_count_tpc + element_count_pds + element_count_ta + element_count_tc;

HDF5FileLayoutParameters
create_file_layout_params()
{
  dunedaq::hdf5libs::HDF5PathParameters params_tpc;
  params_tpc.detector_group_type = "Detector_Readout";
  params_tpc.detector_group_name = "TPC";
  params_tpc.element_name_prefix = "Link";
  params_tpc.digits_for_element_number = 5;

  // dunedaq::hdf5libs::hdf5filelayout::PathParams params_pds;
  // params_pds.detector_group_type = "PDS";
  // params_pds.detector_group_name = "PDS";
  // params_pds.element_name_prefix = "Element";
  // params_pds.digits_for_element_number = 5;

  // note, for unit test json equality checks, 'PDS' needs to come before
  //'TPC', as on reading back the filelayout it looks like it's alphabetical.
  std::vector<dunedaq::hdf5libs::HDF5PathParameters> param_list;
  // param_list.push_back(params_pds);
  param_list.push_back(params_tpc);

  dunedaq::hdf5libs::HDF5FileLayoutParameters layout_params;
  layout_params.path_params_list = param_list;
  layout_params.record_name_prefix = "TriggerRecord";
  layout_params.digits_for_record_number = 6;
  layout_params.digits_for_sequence_number = 4;
  layout_params.record_header_dataset_name = "TriggerRecordHeader";

  return layout_params;
}

dunedaq::daqdataformats::TriggerRecord
create_trigger_record(uint64_t trig_num)
{
  // setup our dummy_data
  std::vector<char> dummy_vector(fragment_size);
  char* dummy_data = dummy_vector.data();

  // get a timestamp for this trigger
  int64_t ts = std::chrono::duration_cast<std::chrono::milliseconds>(system_clock::now().time_since_epoch()).count();

  // create TriggerRecordHeader
  dunedaq::daqdataformats::TriggerRecordHeaderData trh_data;
  trh_data.trigger_number = trig_num;
  trh_data.trigger_timestamp = ts;
  trh_data.num_requested_components = components_per_record;
  trh_data.run_number = run_number;
  trh_data.sequence_number = 0;
  trh_data.max_sequence_number = 1;
  trh_data.element_id = dunedaq::daqdataformats::SourceID(dunedaq::daqdataformats::SourceID::Subsystem::kTRBuilder, 0);

  dunedaq::daqdataformats::TriggerRecordHeader trh(&trh_data);

  // create our TriggerRecord
  dunedaq::daqdataformats::TriggerRecord tr(trh);

  // loop over elements tpc
  for (size_t ele_num = 0; ele_num < element_count_tpc; ++ele_num) {

    // create our fragment
    dunedaq::daqdataformats::FragmentHeader fh;
    fh.trigger_number = trig_num;
    fh.trigger_timestamp = ts;
    fh.window_begin = ts;
    fh.window_end = ts;
    fh.run_number = run_number;
    fh.fragment_type =
      static_cast<dunedaq::daqdataformats::fragment_type_t>(dunedaq::daqdataformats::FragmentType::kWIB);
    fh.sequence_number = 0;
    fh.detector_id = static_cast<uint16_t>(dunedaq::detdataformats::DetID::Subdetector::kHD_TPC);
    fh.element_id =
      dunedaq::daqdataformats::SourceID(dunedaq::daqdataformats::SourceID::Subsystem::kDetectorReadout, ele_num);

    std::unique_ptr<dunedaq::daqdataformats::Fragment> frag_ptr(
      new dunedaq::daqdataformats::Fragment(dummy_data, fragment_size));
    frag_ptr->set_header_fields(fh);

    // add fragment to TriggerRecord
    tr.add_fragment(std::move(frag_ptr));

  } // end loop over elements

  // loop over elements pds
  for (size_t ele_num = 0; ele_num < element_count_pds; ++ele_num) {

    // create our fragment
    dunedaq::daqdataformats::FragmentHeader fh;
    fh.trigger_number = trig_num;
    fh.trigger_timestamp = ts;
    fh.window_begin = ts;
    fh.window_end = ts;
    fh.run_number = run_number;
    fh.fragment_type =
      static_cast<dunedaq::daqdataformats::fragment_type_t>(dunedaq::daqdataformats::FragmentType::kDAPHNE);
    fh.sequence_number = 0;
    fh.detector_id = static_cast<uint16_t>(dunedaq::detdataformats::DetID::Subdetector::kHD_PDS);
    fh.element_id = dunedaq::daqdataformats::SourceID(dunedaq::daqdataformats::SourceID::Subsystem::kDetectorReadout,
                                                      ele_num + element_count_tpc);

    std::unique_ptr<dunedaq::daqdataformats::Fragment> frag_ptr(
      new dunedaq::daqdataformats::Fragment(dummy_data, fragment_size));
    frag_ptr->set_header_fields(fh);

    // add fragment to TriggerRecord
    tr.add_fragment(std::move(frag_ptr));

  } // end loop over elements

  // loop over TriggerActivity
  for (size_t ele_num = 0; ele_num < element_count_ta; ++ele_num) {

    // create our fragment
    dunedaq::daqdataformats::FragmentHeader fh;
    fh.trigger_number = trig_num;
    fh.trigger_timestamp = ts;
    fh.window_begin = ts;
    fh.window_end = ts;
    fh.run_number = run_number;
    fh.fragment_type =
      static_cast<dunedaq::daqdataformats::fragment_type_t>(dunedaq::daqdataformats::FragmentType::kTriggerActivity);
    fh.sequence_number = 0;
    fh.detector_id = static_cast<uint16_t>(dunedaq::detdataformats::DetID::Subdetector::kDAQ);
    fh.element_id = dunedaq::daqdataformats::SourceID(dunedaq::daqdataformats::SourceID::Subsystem::kTrigger, ele_num);

    std::unique_ptr<dunedaq::daqdataformats::Fragment> frag_ptr(
      new dunedaq::daqdataformats::Fragment(dummy_data, fragment_size));
    frag_ptr->set_header_fields(fh);

    // add fragment to TriggerRecord
    tr.add_fragment(std::move(frag_ptr));

  } // end loop over elements

  // loop over TriggerCandidate
  for (size_t ele_num = 0; ele_num < element_count_tc; ++ele_num) {

    // create our fragment
    dunedaq::daqdataformats::FragmentHeader fh;
    fh.trigger_number = trig_num;
    fh.trigger_timestamp = ts;
    fh.window_begin = ts;
    fh.window_end = ts;
    fh.run_number = run_number;
    fh.fragment_type =
      static_cast<dunedaq::daqdataformats::fragment_type_t>(dunedaq::daqdataformats::FragmentType::kTriggerCandidate);
    fh.sequence_number = 0;
    fh.detector_id = static_cast<uint16_t>(dunedaq::detdataformats::DetID::Subdetector::kDAQ);
    fh.element_id = dunedaq::daqdataformats::SourceID(dunedaq::daqdataformats::SourceID::Subsystem::kTrigger,
                                                      ele_num + element_count_ta);

    std::unique_ptr<dunedaq::daqdataformats::Fragment> frag_ptr(
      new dunedaq::daqdataformats::Fragment(dummy_data, fragment_size));
    frag_ptr->set_header_fields(fh);

    // add fragment to TriggerRecord
    tr.add_fragment(std::move(frag_ptr));

  } // end loop over elements

  return tr;
}

struct FileWriteFixture 
{
  FileWriteFixture(int num_triggers = 5, 
                   unsigned comp_lvl = 0, 
                   uint64_t trigger_record_number_offset = 0) 
    : trigger_count(num_triggers), 
      compression_level(comp_lvl),
      file_path(std::filesystem::temp_directory_path()),
      hdf5_filename(
        "demo" + std::to_string(getpid()) + "_" 
        + std::string(getenv("USER")) + "_comp" 
        + std::to_string(compression_level) + ".hdf5"),
      fl_pars(create_file_layout_params()),
      recorded_size_at_write(0)
  {
    delete_files_matching_pattern(file_path, hdf5_filename);

    // create src-geo id map
    auto srcid_geoid_map = create_srcid_geoid_map();
    // create the file
    std::unique_ptr<HDF5RawDataFile> h5file_ptr(new HDF5RawDataFile(file_path + "/" + hdf5_filename,
                                                                    run_number,
                                                                    file_index,
                                                                    application_name,
                                                                    fl_pars,
                                                                    srcid_geoid_map,
                                                                    compression_level));

    // write several events, each with several fragments
    for (uint64_t idx = 0; idx < trigger_count; ++idx) {
        // Allow for the possibility of large trigger record numbers, otherwise just write trigger_count fragments
        uint64_t trigger_number = (trigger_record_number_offset == 0)
                                      ? idx + 1 // Sequential numbering
                                      : 1 + (idx * 2000000000); // Large offset numbering

        BOOST_TEST_MESSAGE("Trigger count: " << trigger_count);
        BOOST_TEST_MESSAGE("Trigger number: " << trigger_number);

        h5file_ptr->write(create_trigger_record(trigger_number));
    }

    // get recorded size for checking
    recorded_size_at_write = h5file_ptr->get_recorded_size();

    h5file_ptr.reset(); // explicit destruction

  }
  ~FileWriteFixture() 
  {
    delete_files_matching_pattern(file_path, hdf5_filename);
  }

  void read_file_attributes()
  {
    // open file for reading now
    h5file_ptr.reset(new HDF5RawDataFile(file_path + "/" + hdf5_filename));

    // check attributes
    auto recorded_size_attr = h5file_ptr->get_attribute<size_t>("recorded_size");
    auto run_number_attr = h5file_ptr->get_attribute<size_t>("run_number");
    auto file_index_attr = h5file_ptr->get_attribute<size_t>("file_index");
    auto app_name_attr = h5file_ptr->get_attribute<std::string>("application_name");
    auto record_type_attr = h5file_ptr->get_attribute<std::string>("record_type");
    auto compression_level_attr = h5file_ptr->get_attribute<unsigned>("compression_level");
    BOOST_REQUIRE_EQUAL(recorded_size_at_write, recorded_size_attr);
    BOOST_REQUIRE_EQUAL(run_number, run_number_attr);
    BOOST_REQUIRE_EQUAL(file_index, file_index_attr);
    BOOST_REQUIRE_EQUAL(application_name, app_name_attr);
    BOOST_REQUIRE_EQUAL("TriggerRecord", record_type_attr);
    BOOST_REQUIRE_EQUAL(this->compression_level, compression_level_attr);

    // extract and check file layout parameters
    auto file_layout_parameters_read = h5file_ptr->get_file_layout().get_file_layout_params();
    BOOST_REQUIRE_EQUAL(fl_pars.to_json(), file_layout_parameters_read.to_json());

    if (this->compression_level == 0) {uncompressed_raw_data_size = recorded_size_at_write;}
    else {
      compressed_raw_data_size = recorded_size_at_write;
      BOOST_ASSERT(compressed_raw_data_size < uncompressed_raw_data_size);
    }
  }

  void read_file_datasets()
  {
    // open file for reading now
    h5file_ptr.reset(new HDF5RawDataFile(file_path + "/" + hdf5_filename));

    auto trigger_record_ids = h5file_ptr->get_all_trigger_record_ids();
    BOOST_REQUIRE_EQUAL(trigger_count, trigger_record_ids.size());

    auto first_trigger_record_id = *(trigger_record_ids.begin());
    auto last_trigger_record_id = *(std::next(trigger_record_ids.begin(), trigger_record_ids.size() - 1));
    BOOST_REQUIRE_EQUAL(1, first_trigger_record_id.first);
    BOOST_REQUIRE_EQUAL(trigger_count, last_trigger_record_id.first);

    auto all_datasets = h5file_ptr->get_dataset_paths();
    BOOST_REQUIRE_EQUAL(trigger_count * (1 + components_per_record), all_datasets.size());

    auto all_trh_paths = h5file_ptr->get_trigger_record_header_dataset_paths();
    BOOST_REQUIRE_EQUAL(trigger_count, all_trh_paths.size());

    auto all_frag_paths = h5file_ptr->get_all_fragment_dataset_paths();
    BOOST_REQUIRE_EQUAL(trigger_count * components_per_record, all_frag_paths.size());

    // test access by name
    std::unique_ptr<dunedaq::daqdataformats::TriggerRecordHeader> trh_ptr;
    trh_ptr = h5file_ptr->get_trh_ptr(all_trh_paths.at(2));
    BOOST_REQUIRE_EQUAL(trh_ptr->get_trigger_number(), 3);
    BOOST_REQUIRE_EQUAL(trh_ptr->get_run_number(), run_number);

    // test access by trigger number
    trh_ptr = h5file_ptr->get_trh_ptr(2, 0);
    BOOST_REQUIRE_EQUAL(trh_ptr->get_trigger_number(), 2);
    BOOST_REQUIRE_EQUAL(trh_ptr->get_run_number(), run_number);

    std::unique_ptr<dunedaq::daqdataformats::Fragment> frag_ptr;

    // test access by name
    frag_ptr = h5file_ptr->get_frag_ptr(all_frag_paths.back());
    BOOST_REQUIRE_EQUAL(frag_ptr->get_trigger_number(), last_trigger_record_id.first);
    BOOST_REQUIRE_EQUAL(frag_ptr->get_run_number(), run_number);

    // test access by trigger number, type,  element
    frag_ptr = h5file_ptr->get_frag_ptr(2, 0, "Detector_Readout", 0);
    BOOST_REQUIRE_EQUAL(frag_ptr->get_trigger_number(), 2);
    BOOST_REQUIRE_EQUAL(frag_ptr->get_run_number(), run_number);
    BOOST_REQUIRE_EQUAL(frag_ptr->get_element_id().subsystem,
                        dunedaq::daqdataformats::SourceID::Subsystem::kDetectorReadout);
    BOOST_REQUIRE_EQUAL(frag_ptr->get_element_id().id, 0);

    // test access by trigger number, type, element
    frag_ptr = h5file_ptr->get_frag_ptr(4, 0, "Detector_Readout", 4);
    BOOST_REQUIRE_EQUAL(frag_ptr->get_trigger_number(), 4);
    BOOST_REQUIRE_EQUAL(frag_ptr->get_run_number(), run_number);
    BOOST_REQUIRE_EQUAL(frag_ptr->get_element_id().subsystem,
                        dunedaq::daqdataformats::SourceID::Subsystem::kDetectorReadout);
    BOOST_REQUIRE_EQUAL(frag_ptr->get_element_id().id, 4);

    // test access by passing in SourceID
    dunedaq::daqdataformats::SourceID gid = { dunedaq::daqdataformats::SourceID::Subsystem::kDetectorReadout, 1 };
    frag_ptr = h5file_ptr->get_frag_ptr(5, 0, gid);
    BOOST_REQUIRE_EQUAL(frag_ptr->get_trigger_number(), 5);
    BOOST_REQUIRE_EQUAL(frag_ptr->get_run_number(), run_number);
    BOOST_REQUIRE_EQUAL(frag_ptr->get_element_id().subsystem,
                        dunedaq::daqdataformats::SourceID::Subsystem::kDetectorReadout);
    BOOST_REQUIRE_EQUAL(frag_ptr->get_element_id().id, 1);
  }

  void read_file_max_sequence()
  {
    // open file for reading now
    h5file_ptr.reset(new HDF5RawDataFile(file_path + "/" + hdf5_filename));

    auto trigger_record_ids = h5file_ptr->get_all_trigger_record_ids();
    BOOST_REQUIRE_EQUAL(trigger_count, trigger_record_ids.size());

    auto first_trigger_record_id = *(trigger_record_ids.begin());
    auto last_trigger_record_id = *(std::next(trigger_record_ids.begin(), trigger_record_ids.size() - 1));
    BOOST_REQUIRE_EQUAL(1, first_trigger_record_id.first);
    BOOST_REQUIRE_EQUAL(trigger_count, last_trigger_record_id.first);

    auto all_datasets = h5file_ptr->get_dataset_paths();
    BOOST_REQUIRE_EQUAL(trigger_count * (1 + components_per_record), all_datasets.size());

    auto all_trh_paths = h5file_ptr->get_trigger_record_header_dataset_paths();
    BOOST_REQUIRE_EQUAL(trigger_count, all_trh_paths.size());

    auto all_frag_paths = h5file_ptr->get_all_fragment_dataset_paths();
    BOOST_REQUIRE_EQUAL(trigger_count * components_per_record, all_frag_paths.size());

    // test access by name
    std::unique_ptr<dunedaq::daqdataformats::TriggerRecordHeader> trh_ptr;
    trh_ptr = h5file_ptr->get_trh_ptr(all_trh_paths.at(2));
    BOOST_REQUIRE_EQUAL(trh_ptr->get_trigger_number(), 3);
    BOOST_REQUIRE_EQUAL(trh_ptr->get_run_number(), run_number);

    // test access by trigger number
    trh_ptr = h5file_ptr->get_trh_ptr(2, 0);
    BOOST_REQUIRE_EQUAL(trh_ptr->get_trigger_number(), 2);
    BOOST_REQUIRE_EQUAL(trh_ptr->get_run_number(), run_number);

    std::unique_ptr<dunedaq::daqdataformats::Fragment> frag_ptr;

    // test access by name
    frag_ptr = h5file_ptr->get_frag_ptr(all_frag_paths.back());
    BOOST_REQUIRE_EQUAL(frag_ptr->get_trigger_number(), last_trigger_record_id.first);
    BOOST_REQUIRE_EQUAL(frag_ptr->get_run_number(), run_number);

    // test access by trigger number, type, element
    frag_ptr = h5file_ptr->get_frag_ptr(2, 0, "Detector_Readout", 0);
    BOOST_REQUIRE_EQUAL(frag_ptr->get_trigger_number(), 2);
    BOOST_REQUIRE_EQUAL(frag_ptr->get_run_number(), run_number);
    BOOST_REQUIRE_EQUAL(frag_ptr->get_element_id().subsystem,
                        dunedaq::daqdataformats::SourceID::Subsystem::kDetectorReadout);
    BOOST_REQUIRE_EQUAL(frag_ptr->get_element_id().id, 0);

    // test access by trigger number, type, element
    frag_ptr = h5file_ptr->get_frag_ptr(4, 0, "Detector_Readout", 4);
    BOOST_REQUIRE_EQUAL(frag_ptr->get_trigger_number(), 4);
    BOOST_REQUIRE_EQUAL(frag_ptr->get_run_number(), run_number);
    BOOST_REQUIRE_EQUAL(frag_ptr->get_element_id().subsystem,
                        dunedaq::daqdataformats::SourceID::Subsystem::kDetectorReadout);
    BOOST_REQUIRE_EQUAL(frag_ptr->get_element_id().id, 4);

    // test access by passing in SourceID
    dunedaq::daqdataformats::SourceID gid = { dunedaq::daqdataformats::SourceID::Subsystem::kDetectorReadout, 1 };
    frag_ptr = h5file_ptr->get_frag_ptr(5, 0, gid);
    BOOST_REQUIRE_EQUAL(frag_ptr->get_trigger_number(), 5);
    BOOST_REQUIRE_EQUAL(frag_ptr->get_run_number(), run_number);
    BOOST_REQUIRE_EQUAL(frag_ptr->get_element_id().subsystem,
                        dunedaq::daqdataformats::SourceID::Subsystem::kDetectorReadout);
    BOOST_REQUIRE_EQUAL(frag_ptr->get_element_id().id, 1);
  }

  void read_write_large_trigger_numbers() 
  {
    // open file for reading now
    h5file_ptr.reset(new HDF5RawDataFile(file_path + "/" + hdf5_filename));

    auto trigger_record_ids = h5file_ptr->get_all_trigger_record_ids();
    BOOST_REQUIRE_EQUAL(trigger_count, trigger_record_ids.size());

    auto first_trigger_record_id = *(trigger_record_ids.begin());
    auto last_trigger_record_id = *(std::next(trigger_record_ids.begin(), trigger_record_ids.size() - 1));
    BOOST_REQUIRE_EQUAL(1, first_trigger_record_id.first);
    BOOST_TEST_MESSAGE("Trigger count: " << trigger_count);
    BOOST_TEST_MESSAGE("Trigger number: " << trigger_number);
    BOOST_REQUIRE_EQUAL(trigger_number, last_trigger_record_id.first);
    BOOST_REQUIRE(trigger_number > 0xffffffff);
  }

  uint64_t trigger_count;
  uint64_t trigger_number;
  unsigned compression_level;
  std::string file_path;
  std::string hdf5_filename;
  HDF5FileLayoutParameters fl_pars;
  size_t recorded_size_at_write;
  std::unique_ptr<HDF5RawDataFile> h5file_ptr;
};

BOOST_AUTO_TEST_SUITE(HDF5WriteReadTriggerRecord_test)

BOOST_AUTO_TEST_CASE(ReadFileAttributes)
{
  FileWriteFixture fixture(5, 0, 0);
  fixture.read_file_attributes();
}

BOOST_AUTO_TEST_CASE(ReadCompressedFileAttributes)
{
  FileWriteFixture fixture(5, 1, 0);
  fixture.read_file_attributes();
}

BOOST_AUTO_TEST_CASE(ReadFileDatasets)
{
  FileWriteFixture fixture(5, 0, 0);
  fixture.read_file_datasets();
}

BOOST_AUTO_TEST_CASE(ReadCompressedFileDatasets)
{
  FileWriteFixture fixture(5, 1, 0);
  fixture.read_file_datasets();
}

BOOST_AUTO_TEST_CASE(ReadFileMaxSequence)
{
  FileWriteFixture fixture(5, 0, 0);
  fixture.read_file_max_sequence();
}

BOOST_AUTO_TEST_CASE(ReadCompressedFileMaxSequence)
{
  FileWriteFixture fixture(5, 1, 0);
  fixture.read_file_max_sequence();
}

BOOST_AUTO_TEST_CASE(LargeTriggerRecordNumbers)
{
  FileWriteFixture fixture(5, 0, 2000000000);
  fixture.read_write_large_trigger_numbers();
}

BOOST_AUTO_TEST_CASE(CompressedLargeTriggerRecordNumbers)
{
  FileWriteFixture fixture(5, 1, 2000000000);
  fixture.read_write_large_trigger_numbers();
}

BOOST_AUTO_TEST_SUITE_END()
