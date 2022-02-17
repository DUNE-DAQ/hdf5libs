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
  std::cout << "Usage: HDF5LIBS_TestWriter <configuration_file> <output_file_name>" << std::endl;
}

int main(int argc, char** argv){

  if(argc!=3){
    print_usage();
    return 1;
  }

  const std::string app_name = std::string(argv[0]);
  const std::string ifile_name = std::string(argv[1]);
  const std::string ofile_name = std::string(argv[2]);

  //read in configuration
  nlohmann::json j_in,fl_conf;
  std::ifstream ifile(ifile_name);
  ifile >> j_in;
  ifile.close();

  //get file_layout config
  try{
    fl_conf = j_in["file_layout"].get<hdf5filelayout::FileLayoutParams>();
    std::cout << "Read 'file_layout' configuration:\n" << std::endl;
    std::cout << fl_conf << std::endl;
  } catch(...){
    std::cout << "ERROR: Improper 'file_layout' configuration in " << ifile_name << std::endl;
    return 1;
  }

  //read test writer app configs
  const int run_number = j_in["run_number"].get<int>();
  const int file_index = j_in["file_index"].get<int>();
  
  const int trigger_count = j_in["trigger_count"].get<int>();
  const int fragment_size = j_in["data_size"].get<int>()+sizeof(FragmentHeader);
  const GeoID::SystemType gtype_to_use = GeoID::string_to_system_type(j_in["fragment_type"].get<std::string>());
  const int region_count = j_in["region_count"].get<int>();
  const int element_count = j_in["element_count"].get<int>();


  std::cout << "\nOutput file: " << ofile_name
	    << "\nRun number: " << run_number
	    << "\nFile index: " << file_index
	    << "\nNumber of trigger records: " << trigger_count
	    << "\nNumber of fragments: " << region_count*element_count
	    << "  (Regions=" << region_count << ",Elements=" << element_count << ")"
	    << "\nFragment type: " << GeoID::system_type_to_string(gtype_to_use)
	    << "\nFragment size (bytes, incl. header): " << fragment_size
	    << std::endl;


  //open our file for writing
  HDF5RawDataFile h5_raw_data_file = HDF5RawDataFile(ofile_name,
						     run_number,//run_number
						     file_index,//file_index,
						     app_name,//app_name
						     fl_conf,//file_layout_confs
						     HighFive::File::Overwrite); //optional: overwrite existing file


  //setup our dummy_data
  char* dummy_data = new char[fragment_size];

  //loop over desired number of triggers
  for( int trig_num=1; trig_num<=trigger_count; ++trig_num){

    //get a timestamp for this trigger
    uint64_t ts = std::chrono::duration_cast<std::chrono::milliseconds>(system_clock::now().time_since_epoch()).count();

    std::cout << "\tWriting trigger " << trig_num << " with time_stamp " << ts << std::endl;
    
    //create TriggerRecordHeader
    TriggerRecordHeaderData trh_data;
    trh_data.trigger_number = trig_num;
    trh_data.trigger_timestamp = ts;
    trh_data.num_requested_components = region_count*element_count;
    trh_data.run_number = run_number;
    trh_data.sequence_number = 0;
    trh_data.max_sequence_number = 1;

    TriggerRecordHeader trh(&trh_data);
    
    //create out TriggerRecord
    TriggerRecord tr(trh);

    //loop over regions and elements
    for( int reg_num=0; reg_num < region_count; ++reg_num)
      {
	for( int ele_num=0; ele_num < element_count; ++ele_num){

	  //create our fragment
	  FragmentHeader fh;
	  fh.trigger_number = trig_num;
	  fh.trigger_timestamp = ts;
	  fh.window_begin = ts - 10;
	  fh.window_end   = ts;
	  fh.run_number = run_number;
	  fh.fragment_type = 0;
	  fh.element_id = GeoID(gtype_to_use, reg_num, ele_num);

	  std::unique_ptr<Fragment> frag_ptr(new Fragment(dummy_data,fragment_size));
	  frag_ptr->set_header_fields(fh);

	  //add fragment to TriggerRecord
	  tr.add_fragment(std::move(frag_ptr));

	}//end loop over elements
      }//end loop over regions

    //write trigger record to file
    h5_raw_data_file.write(tr);

  }//end loop over triggers

  delete[] dummy_data;

  std::cout << "Finished writing to file " << h5_raw_data_file.get_file_name() << std::endl;
  std::cout << "Recorded size: " << h5_raw_data_file.get_recorded_size() << std::endl;

 return 0;
}
