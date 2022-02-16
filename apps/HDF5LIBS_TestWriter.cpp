/**
 * @file demo_tpc_decoder.cpp
 *
 * Demo of HDF5 file reader for TPC fragments: this example shows how to extract fragments from a file and decode WIB frames.
 * 
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */


#include <iostream>
#include <string>
#include <fstream>

#include "logging/Logging.hpp"
#include "hdf5libs/HDF5RawDataFile.hpp" 
#include "hdf5libs/hdf5filelayout/Nljs.hpp"


using namespace dunedaq::hdf5libs;
using namespace dunedaq::daqdataformats;

void print_usage()
{
  std::cout << "Usage: HDF5LIBS_TestWriter <input_file_name> <output_file_name>" << std::endl;
}

int main(int argc, char** argv){

  if(argc!=3){
    print_usage();
    return 1;
  }

  const std::string app_name = std::string(argv[0]);
  const std::string ifile_name = std::string(argv[1]);
  const std::string ofile_name = std::string(argv[2]);

  nlohmann::json j_in,conf;

  std::ifstream ifile(ifile_name);
  ifile >> j_in;
  ifile.close();

  conf = j_in["file_layout"].get<hdf5filelayout::FileLayoutParams>();

  const int run_number = 100;
  const int file_index = 0;

  HDF5RawDataFile h5_raw_data_file = HDF5RawDataFile(ofile_name,
						     run_number,//run_number
						     file_index,//file_index,
						     app_name,//app_name
						     conf);

  
  constexpr int dummydata_size = 1000;
  const int trigger_count = 5;
  const GeoID::SystemType gtype_to_use = GeoID::SystemType::kTPC;
  const int region_count = 3;
  const int element_count = 10;

  std::array<char,dummydata_size> dummy_data;

  for( int trig_num=1; trig_num<=trigger_count; ++trig_num){

    uint64_t ts = std::chrono::duration_cast<std::chrono::milliseconds>(system_clock::now().time_since_epoch()).count();
    
    TriggerRecordHeaderData trh_data;
    trh_data.trigger_number = trig_num;
    trh_data.trigger_timestamp = ts;
    trh_data.num_requested_components = region_count*element_count;
    trh_data.run_number = run_number;
    trh_data.sequence_number = 0;
    trh_data.max_sequence_number = 1;

    TriggerRecordHeader trh(&trh_data);
    
    TriggerRecord tr(trh);

    for( int reg_num=0; reg_num < region_count; ++reg_num)
      {
	for( int ele_num=0; ele_num < element_count; ++ele_num){

	  FragmentHeader fh;
	  fh.trigger_number = trig_num;
	  fh.trigger_timestamp = ts;
	  fh.window_begin = ts - 10;
	  fh.window_end   = ts;
	  fh.run_number = run_number;
	  fh.fragment_type = 0;
	  fh.element_id = GeoID(gtype_to_use, reg_num, ele_num);

	  std::unique_ptr<Fragment> frag_ptr(new Fragment(dummy_data.data(),dummydata_size));
	  frag_ptr->set_header_fields(fh);
	  
	  tr.add_fragment(std::move(frag_ptr));
	}//end loop over elements
      }//end loop over regions

    h5_raw_data_file.write(tr);

  }//end loop over triggers


  /*
  // =================
  // DUMMY WRITE
  // =================
  // write several events, each with several fragments
  constexpr int dummydata_size = 7;
  const int trigger_count = 5;
  const StorageKey::DataRecordGroupType group_type = StorageKey::DataRecordGroupType::kTPC;
  const int apa_count = 3;
  const int link_count = 1;

  std::array<char, dummydata_size> dummy_data;
  for (int trigger_number = 1; trigger_number <= trigger_count; ++trigger_number) {
    for (int apa_number = 1; apa_number <= apa_count; ++apa_number) {
      for (int link_number = 1; link_number <= link_count; ++link_number) {
        StorageKey key(trigger_number, group_type, apa_number, link_number);
        KeyedDataBlock data_block(key);
        data_block.m_unowned_data_start = static_cast<void*>(&dummy_data[0]);
        data_block.m_data_size = dummydata_size;
        h5_raw_data_file.write(data_block);
      }                   // link number
    }                     // apa number
  }                       // trigger number

  */
  // Get and print attribute names and their values
  auto attributes_map = h5_raw_data_file.get_attributes();
  for (const auto& [k, v] : attributes_map){
    std::cout << k << " : ";
    std::visit([](const auto& x){ std::cout << x; }, v);
    std::cout << '\n';
  }


/*
  std::vector<std::string> datasets_path = decoder.get_fragments(start_tr,num_trs);
  //std::vector<std::string> datasets_path = decoder.get_trh(start_tr,num_trs);
 
  std::cout << "Number of fragments: " << datasets_path.size() << std::endl; 
*/  
  

  
 return 0;
}
