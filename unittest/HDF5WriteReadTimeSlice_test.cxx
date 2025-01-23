/**
 * @file HDF5WriteReadTimeSlice_test.cxx Application that tests and demonstrates
 * the write/read functions of the HDF5RawDataFile class.
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "hdf5libs/HDF5RawDataFile.hpp"
#include "hdf5libs/test/HDF5TestUtils.hpp"

#include "detdataformats/DetID.hpp"

#define BOOST_TEST_MODULE HDF5WriteReadTimeSlice_test // NOLINT

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
const std::string application_name = "HDF5WriteReadTimeSlice_test";
constexpr size_t fragment_size = 100;
constexpr size_t element_count_tpc = 4;
constexpr size_t element_count_pds = 4;
size_t compressed_raw_data_size = 0;
size_t uncompressed_raw_data_size = 0;

const size_t components_per_record = element_count_tpc + element_count_pds;

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
  layout_params.record_name_prefix = "TimeSlice";
  layout_params.digits_for_record_number = 6;
  layout_params.digits_for_sequence_number = 0;
  layout_params.record_header_dataset_name = "TimeSliceHeader";

  return layout_params;
}

dunedaq::daqdataformats::TimeSlice
create_timeslice(int ts_num)
{
  // setup our dummy_data
  std::vector<char> dummy_vector(fragment_size);
  char* dummy_data = dummy_vector.data();

  // get a timestamp for this trigger
  int64_t timestamp =
    std::chrono::duration_cast<std::chrono::milliseconds>(system_clock::now().time_since_epoch()).count();

  // create TimeSliceHeader
  dunedaq::daqdataformats::TimeSliceHeader tsh;
  tsh.timeslice_number = ts_num;
  tsh.run_number = run_number;
  tsh.element_id = dunedaq::daqdataformats::SourceID(dunedaq::daqdataformats::SourceID::Subsystem::kTRBuilder, 0);

  // create our TimeSlice
  dunedaq::daqdataformats::TimeSlice ts(tsh);

  // loop over elements tpc
  for (size_t ele_num = 0; ele_num < element_count_tpc; ++ele_num) {

    // create our fragment
    dunedaq::daqdataformats::FragmentHeader fh;
    fh.trigger_number = ts_num;
    fh.trigger_timestamp = timestamp;
    fh.window_begin = timestamp;
    fh.window_end = timestamp;
    fh.run_number = run_number;
    fh.fragment_type = 0;
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
    ts.add_fragment(std::move(frag_ptr));

  } // end loop over elements

  // loop over elements pds
  for (size_t ele_num = 0; ele_num < element_count_pds; ++ele_num) {

    // create our fragment
    dunedaq::daqdataformats::FragmentHeader fh;
    fh.trigger_number = ts_num;
    fh.trigger_timestamp = timestamp;
    fh.window_begin = timestamp;
    fh.window_end = timestamp;
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
    ts.add_fragment(std::move(frag_ptr));

  } // end loop over elements

  return ts;
}

struct FileWriteFixture 
{
  FileWriteFixture(int num_slices = 5, unsigned comp_lvl = 0) 
    : timeslice_count(num_slices), 
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

    // convert file_params to json, allows for easy comp later
    auto fl_pars = create_file_layout_params();

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
    for (int timeslice_number = 1; timeslice_number <= timeslice_count; ++timeslice_number)
      h5file_ptr->write(create_timeslice(timeslice_number));

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
    //std::unique_ptr<HDF5RawDataFile> h5file_ptr = std::make_unique<HDF5RawDataFile>(file_path + "/" + hdf5_filename);
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
    BOOST_REQUIRE_EQUAL("TimeSlice", record_type_attr);
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

    auto timeslices = h5file_ptr->get_all_timeslice_numbers();
    BOOST_REQUIRE_EQUAL(timeslice_count, timeslices.size());

    auto first_timeslice = *(timeslices.begin());
    auto last_timeslice = *(std::next(timeslices.begin(), timeslices.size() - 1));
    BOOST_REQUIRE_EQUAL(1, first_timeslice);
    BOOST_REQUIRE_EQUAL(timeslice_count, last_timeslice);

    auto all_datasets = h5file_ptr->get_dataset_paths();
    BOOST_REQUIRE_EQUAL(timeslice_count * (1 + components_per_record), all_datasets.size());

    auto all_tsh_paths = h5file_ptr->get_timeslice_header_dataset_paths();
    BOOST_REQUIRE_EQUAL(timeslice_count, all_tsh_paths.size());

    auto all_frag_paths = h5file_ptr->get_all_fragment_dataset_paths();
    BOOST_REQUIRE_EQUAL(timeslice_count * components_per_record, all_frag_paths.size());

    // test access by name
    std::unique_ptr<dunedaq::daqdataformats::TimeSliceHeader> trs_ptr;
    trs_ptr = h5file_ptr->get_tsh_ptr(all_tsh_paths.at(2));
    BOOST_REQUIRE_EQUAL(trs_ptr->timeslice_number, 3);
    BOOST_REQUIRE_EQUAL(trs_ptr->run_number, run_number);

    // test access by trigger number
    trs_ptr = h5file_ptr->get_tsh_ptr(2);
    BOOST_REQUIRE_EQUAL(trs_ptr->timeslice_number, 2);
    BOOST_REQUIRE_EQUAL(trs_ptr->run_number, run_number);

    std::unique_ptr<dunedaq::daqdataformats::Fragment> frag_ptr;

    // test access by name
    frag_ptr = h5file_ptr->get_frag_ptr(all_frag_paths.back());
    BOOST_REQUIRE_EQUAL(frag_ptr->get_trigger_number(), last_timeslice);
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

  void read_file_max_sequence()
  {
    // open file for reading now
    h5file_ptr.reset(new HDF5RawDataFile(file_path + "/" + hdf5_filename));

    auto timeslices = h5file_ptr->get_all_timeslice_numbers();
    BOOST_REQUIRE_EQUAL(timeslice_count, timeslices.size());

    auto first_timeslice = *(timeslices.begin());
    auto last_timeslice = *(std::next(timeslices.begin(), timeslices.size() - 1));
    BOOST_REQUIRE_EQUAL(1, first_timeslice);
    BOOST_REQUIRE_EQUAL(timeslice_count, last_timeslice);

    auto all_datasets = h5file_ptr->get_dataset_paths();
    BOOST_REQUIRE_EQUAL(timeslice_count * (1 + components_per_record), all_datasets.size());

    auto all_tsh_paths = h5file_ptr->get_timeslice_header_dataset_paths();
    BOOST_REQUIRE_EQUAL(timeslice_count, all_tsh_paths.size());

    auto all_frag_paths = h5file_ptr->get_all_fragment_dataset_paths();
    BOOST_REQUIRE_EQUAL(timeslice_count * components_per_record, all_frag_paths.size());

    // test access by name
    std::unique_ptr<dunedaq::daqdataformats::TimeSliceHeader> trs_ptr;
    trs_ptr = h5file_ptr->get_tsh_ptr(all_tsh_paths.at(2));
    BOOST_REQUIRE_EQUAL(trs_ptr->timeslice_number, 3);
    BOOST_REQUIRE_EQUAL(trs_ptr->run_number, run_number);

    // test access by trigger number
    trs_ptr = h5file_ptr->get_tsh_ptr(2);
    BOOST_REQUIRE_EQUAL(trs_ptr->timeslice_number, 2);
    BOOST_REQUIRE_EQUAL(trs_ptr->run_number, run_number);

    std::unique_ptr<dunedaq::daqdataformats::Fragment> frag_ptr;

    // test access by name
    frag_ptr = h5file_ptr->get_frag_ptr(all_frag_paths.back());
    BOOST_REQUIRE_EQUAL(frag_ptr->get_trigger_number(), last_timeslice);
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

  int timeslice_count;
  unsigned compression_level;
  std::string file_path;
  std::string hdf5_filename;
  HDF5FileLayoutParameters fl_pars;
  size_t recorded_size_at_write;
  std::unique_ptr<HDF5RawDataFile> h5file_ptr;
};

BOOST_AUTO_TEST_SUITE(HDF5WriteReadTimeSlice_test)

BOOST_AUTO_TEST_CASE(ReadFileAttributes) 
{
  FileWriteFixture fixture(5, 0);
  fixture.read_file_attributes();
}

BOOST_AUTO_TEST_CASE(ReadCompressedFileAttributes) 
{
  FileWriteFixture fixture(5, 1);
  fixture.read_file_attributes();
}

BOOST_AUTO_TEST_CASE(ReadFileDatasets) 
{
  FileWriteFixture fixture(5, 0);
  fixture.read_file_datasets();
}

BOOST_AUTO_TEST_CASE(ReadCompressedFileDatasets) 
{
  FileWriteFixture fixture(5, 1);
  fixture.read_file_datasets();
}

BOOST_AUTO_TEST_CASE(ReadFileMaxSequence) 
{
  FileWriteFixture fixture(5, 0);
  fixture.read_file_max_sequence();
}

BOOST_AUTO_TEST_CASE(ReadCompressedFileMaxSequence) 
{
  FileWriteFixture fixture(5, 1);
  fixture.read_file_max_sequence();
}

BOOST_AUTO_TEST_SUITE_END()
