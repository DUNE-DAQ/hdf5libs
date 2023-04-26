/**
 * @file HDF5WriteReadTimeSlice_test.cxx Application that tests and demonstrates
 * the write/read functions of the HDF5RawDataFile class.
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "hdf5libs/HDF5RawDataFile.hpp"
#include "hdf5libs/hdf5filelayout/Nljs.hpp"
#include "hdf5libs/hdf5filelayout/Structs.hpp"
#include "hdf5libs/hdf5rawdatafile/Structs.hpp"
#include "hdf5libs/hdf5rawdatafile/Nljs.hpp"

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

const size_t components_per_record = element_count_tpc + element_count_pds;

std::vector<std::string>
get_files_matching_pattern(const std::string& path, const std::string& pattern)
{
  std::regex regex_search_pattern(pattern);
  std::vector<std::string> file_list;
  for (const auto& entry : std::filesystem::directory_iterator(path)) {
    if (std::regex_match(entry.path().filename().string(), regex_search_pattern)) {
      file_list.push_back(entry.path());
    }
  }
  return file_list;
}

std::vector<std::string>
delete_files_matching_pattern(const std::string& path, const std::string& pattern)
{
  std::regex regex_search_pattern(pattern);
  std::vector<std::string> file_list;
  for (const auto& entry : std::filesystem::directory_iterator(path)) {
    if (std::regex_match(entry.path().filename().string(), regex_search_pattern)) {
      if (std::filesystem::remove(entry.path())) {
        file_list.push_back(entry.path());
      }
    }
  }
  return file_list;
}

hdf5filelayout::FileLayoutParams
create_file_layout_params()
{
  dunedaq::hdf5libs::hdf5filelayout::PathParams params_tpc;
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
  dunedaq::hdf5libs::hdf5filelayout::PathParamList param_list;
  // param_list.push_back(params_pds);
  param_list.push_back(params_tpc);

  dunedaq::hdf5libs::hdf5filelayout::FileLayoutParams layout_params;
  layout_params.path_param_list = param_list;
  layout_params.record_name_prefix = "TimeSlice";
  layout_params.digits_for_record_number = 6;
  layout_params.digits_for_sequence_number = 0;
  layout_params.record_header_dataset_name = "TimeSliceHeader";

  return layout_params;
}

void
create_hardware_map_file(const std::string& filename)
{
  std::ofstream tmpfile;
  tmpfile.open(filename);
  tmpfile << "# DRO_SourceID DetLink DetSlot DetCrate DetID DRO_Host DRO_Card DRO_SLR DRO_Link\n";
  tmpfile << "0 0 0 1 3 np04-srv-011 0 0 0\n";
  tmpfile << "1 1 0 1 3 np04-srv-011 0 0 1\n";
  tmpfile << "2 0 1 1 3 np04-srv-011 0 0 2\n";
  tmpfile << "3 1 1 1 3 np04-srv-011 0 0 3\n";
  tmpfile << "4 0 0 1 2 np04-srv-017 0 0 0\n";
  tmpfile << "5 1 0 1 2 np04-srv-017 0 0 1\n";
  tmpfile << "6 0 1 1 2 np04-srv-017 0 0 2\n";
  tmpfile << "7 1 1 1 2 np04-srv-017 0 0 3\n";
  tmpfile.close();
}


hdf5rawdatafile::SrcGeoIDMap
create_srcid_geoid_map(){
  using nlohmann::json;

  hdf5rawdatafile::SrcGeoIDMap map;
  json srcid_geoid_map = json::parse(R"(
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

  return srcid_geoid_map.get<hdf5rawdatafile::SrcGeoIDMap>();
  
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

BOOST_AUTO_TEST_SUITE(HDF5WriteReadTimeSlice_test)

BOOST_AUTO_TEST_CASE(WriteFileAndAttributes)
{
  std::string file_path(std::filesystem::temp_directory_path());
  std::string hdf5_filename = "demo" + std::to_string(getpid()) + "_" + std::string(getenv("USER")) + ".hdf5";
  std::string hw_map_file = "hw_map_" + std::to_string(getpid()) + "_" + std::string(getenv("USER")) + ".txt";

  const int timeslice_count = 5;

  // delete any pre-existing files so that we start with a clean slate
  delete_files_matching_pattern(file_path, hdf5_filename);
  delete_files_matching_pattern(file_path, hw_map_file);

  // convert file_params to json, allows for easy comp later
  hdf5filelayout::data_t flp_json_in;
  hdf5filelayout::to_json(flp_json_in, create_file_layout_params());

  // // create the hardware map service
  // create_hardware_map_file(file_path + "/" + hw_map_file);
  // std::shared_ptr<dunedaq::detchannelmaps::HardwareMapService> hw_map_service(
  //   new dunedaq::detchannelmaps::HardwareMapService(file_path + "/" + hw_map_file));

  // // create the file
  // std::unique_ptr<HDF5RawDataFile> h5file_ptr(new HDF5RawDataFile(file_path + "/" + hdf5_filename,
  //                                                                 run_number,
  //                                                                 file_index,
  //                                                                 application_name,
  //                                                                 // create_file_layout_params()));
  //                                                                 flp_json_in,
  //                                                                 hw_map_service));


  // create src-geo id map
  auto srcid_geoid_map = create_srcid_geoid_map();
  // create the file
  std::unique_ptr<HDF5RawDataFile> h5file_ptr(new HDF5RawDataFile(file_path + "/" + hdf5_filename,
                                                                  run_number,
                                                                  file_index,
                                                                  application_name,
                                                                  flp_json_in,
                                                                  srcid_geoid_map));


  // write several events, each with several fragments
  for (int timeslice_number = 1; timeslice_number <= timeslice_count; ++timeslice_number)
    h5file_ptr->write(create_timeslice(timeslice_number));

  // get recorded size for checking
  size_t recorded_size_at_write = h5file_ptr->get_recorded_size();

  h5file_ptr.reset(); // explicit destruction

  // open file for reading now
  h5file_ptr.reset(new HDF5RawDataFile(file_path + "/" + hdf5_filename));

  // check attributes
  auto recorded_size_attr = h5file_ptr->get_attribute<size_t>("recorded_size");
  auto run_number_attr = h5file_ptr->get_attribute<size_t>("run_number");
  auto file_index_attr = h5file_ptr->get_attribute<size_t>("file_index");
  auto app_name_attr = h5file_ptr->get_attribute<std::string>("application_name");
  auto record_type_attr = h5file_ptr->get_attribute<std::string>("record_type");
  BOOST_REQUIRE_EQUAL(recorded_size_at_write, recorded_size_attr);
  BOOST_REQUIRE_EQUAL(run_number, run_number_attr);
  BOOST_REQUIRE_EQUAL(file_index, file_index_attr);
  BOOST_REQUIRE_EQUAL(application_name, app_name_attr);
  BOOST_REQUIRE_EQUAL("TimeSlice", record_type_attr);

  // extract and check file layout parameters
  auto file_layout_parameters_read = h5file_ptr->get_file_layout().get_file_layout_params();
  hdf5filelayout::data_t flp_json_read;
  hdf5filelayout::to_json(flp_json_read, file_layout_parameters_read);
  BOOST_REQUIRE_EQUAL(flp_json_in, flp_json_read);

  // clean up the files that were created
  delete_files_matching_pattern(file_path, hdf5_filename);
  delete_files_matching_pattern(file_path, hw_map_file);
}

BOOST_AUTO_TEST_CASE(ReadFileDatasets)
{
  std::string file_path(std::filesystem::temp_directory_path());
  std::string hdf5_filename = "demo" + std::to_string(getpid()) + "_" + std::string(getenv("USER")) + ".hdf5";
  std::string hw_map_file = "hw_map_" + std::to_string(getpid()) + "_" + std::string(getenv("USER")) + ".txt";

  const int timeslice_count = 5;

  // delete any pre-existing files so that we start with a clean slate
  delete_files_matching_pattern(file_path, hdf5_filename);
  delete_files_matching_pattern(file_path, hw_map_file);

  // // create the hardware map service
  // create_hardware_map_file(file_path + "/" + hw_map_file);
  // std::shared_ptr<dunedaq::detchannelmaps::HardwareMapService> hw_map_service(
  //   new dunedaq::detchannelmaps::HardwareMapService(file_path + "/" + hw_map_file));

  // // create the file
  // std::unique_ptr<HDF5RawDataFile> h5file_ptr(new HDF5RawDataFile(file_path + "/" + hdf5_filename,
  //                                                                 run_number,
  //                                                                 file_index,
  //                                                                 application_name,
  //                                                                 create_file_layout_params(),
  //                                                                 hw_map_service));
  // create src-geo id map
  auto srcid_geoid_map = create_srcid_geoid_map();
  // create the file
  std::unique_ptr<HDF5RawDataFile> h5file_ptr(new HDF5RawDataFile(file_path + "/" + hdf5_filename,
                                                                  run_number,
                                                                  file_index,
                                                                  application_name,
                                                                  create_file_layout_params(),
                                                                  srcid_geoid_map));

  // write several events, each with several fragments
  for (int timeslice_number = 1; timeslice_number <= timeslice_count; ++timeslice_number)
    h5file_ptr->write(create_timeslice(timeslice_number));

  h5file_ptr.reset(); // explicit destruction

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

  // clean up the files that were created
  delete_files_matching_pattern(file_path, hdf5_filename);
  delete_files_matching_pattern(file_path, hw_map_file);
}

BOOST_AUTO_TEST_CASE(ReadFileMaxSequence)
{
  std::string file_path(std::filesystem::temp_directory_path());
  std::string hdf5_filename = "demo" + std::to_string(getpid()) + "_" + std::string(getenv("USER")) + ".hdf5";
  std::string hw_map_file = "hw_map_" + std::to_string(getpid()) + "_" + std::string(getenv("USER")) + ".txt";

  const int timeslice_count = 5;

  // delete any pre-existing files so that we start with a clean slate
  delete_files_matching_pattern(file_path, hdf5_filename);
  delete_files_matching_pattern(file_path, hw_map_file);

  auto fl_pars = create_file_layout_params();
  fl_pars.digits_for_sequence_number = 4;

  // create the hardware map service
  create_hardware_map_file(file_path + "/" + hw_map_file);
  std::shared_ptr<dunedaq::detchannelmaps::HardwareMapService> hw_map_service(
    new dunedaq::detchannelmaps::HardwareMapService(file_path + "/" + hw_map_file));

  // create the file
  std::unique_ptr<HDF5RawDataFile> h5file_ptr(new HDF5RawDataFile(
    file_path + "/" + hdf5_filename, run_number, file_index, application_name, fl_pars, hw_map_service));

  // write several events, each with several fragments
  for (int timeslice_number = 1; timeslice_number <= timeslice_count; ++timeslice_number)
    h5file_ptr->write(create_timeslice(timeslice_number));

  h5file_ptr.reset(); // explicit destruction

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

  // clean up the files that were created
  delete_files_matching_pattern(file_path, hdf5_filename);
  delete_files_matching_pattern(file_path, hw_map_file);
}

BOOST_AUTO_TEST_SUITE_END()
