/**
 * @file reader.cpp
 *
 * Example of HDF5 file reader 
 * 
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */


#include <iostream>
#include <string>
#include <highfive/H5File.hpp>
#include <highfive/H5Object.hpp>

#include "dataformats/TriggerRecord.hpp"
#include "dataformats/wib/WIBFrame.hpp"

// AAA: TODO: make this as a method 
// GLM: Extract main info to see if we can read raw data
// Function to read datasets 
//
void readDataset(std::string path_dataset, void* buff) {

  std::string tr_header = "TriggerRecordHeader";
  if (path_dataset.find(tr_header) != std::string::npos) {
     std::cout << "--- TR header dataset" << path_dataset << std::endl;
     dunedaq::dataformats::TriggerRecordHeader trh(buff);
     std::cout << "Run number: " << trh.get_run_number()
               << " Trigger number: " << trh.get_trigger_number()
               << " Requested fragments: " <<trh.get_num_requested_components() << std::endl;
     std::cout << "============================================================" << std::endl;
  }
  else {
     std::cout << "+++ Fragment dataset" << path_dataset << std::endl;
     dunedaq::dataformats::Fragment frag(buff, dunedaq::dataformats::Fragment::BufferAdoptionMode::kReadOnlyMode);
     // Here I can now look into the raw data
     // As an example, we print a couple of attributes of the Fragment header and then dump the first WIB frame.
     if(frag.get_fragment_type() == dunedaq::dataformats::FragmentType::kTPCData) {
       std::cout << "Fragment with Run number: " << frag.get_run_number() 
                 << " Trigger number: " << frag.get_trigger_number() 
                 << " GeoID: " << frag.get_element_id() << std::endl;

       // Get pointer to the first WIB frame
       auto wfptr = reinterpret_cast<dunedaq::dataformats::WIBFrame*>(frag.get_data());       
       size_t raw_data_packets = (frag.get_size() - sizeof(dunedaq::dataformats::FragmentHeader)) / sizeof(dunedaq::dataformats::WIBFrame);
       std::cout << "Fragment contains " << raw_data_packets << " WIB frames" << std::endl;
        for (size_t i=0; i < raw_data_packets; ++i) {
           auto wf1ptr = reinterpret_cast<dunedaq::dataformats::WIBFrame*>(frag.get_data()+i*sizeof(dunedaq::dataformats::WIBFrame));
           // print first WIB header
           if (i==0) {
               std::cout << "First WIB header:"<< *(wfptr->get_wib_header());
               std::cout << "Printout sampled timestamps in WIB headers: " ;
           }
           // printout timestamp every now and then, only as example of accessing data...
           if(i%1000 == 0) std::cout << "Timestamp " << i << ": " << wf1ptr->get_timestamp() << " ";
       }
       std::cout << std::endl;


     }
     else {
       std::cout << "Skipping unknown fragment type" << std::endl;
     }

  }
}

// Recursive function to traverse the HDF5 file
void exploreSubGroup(HighFive::Group parent_group, std::string relative_path, std::vector<std::string>& path_list) {
   std::vector<std::string> childNames = parent_group.listObjectNames();
   for (auto& child_name : childNames) {
     std::string full_path = relative_path + "/" + child_name;
     HighFive::ObjectType child_type = parent_group.getObjectType(child_name);
     if (child_type == HighFive::ObjectType::Dataset) {
       //std::cout << "Dataset: " << child_name << std::endl;       
       path_list.push_back(full_path);
     } else if (child_type == HighFive::ObjectType::Group) {
       //std::cout << "Group: " << child_name << std::endl;

       HighFive::Group child_group = parent_group.getGroup(child_name);
       // start the recusion
       std::string new_path = relative_path + "/" + child_name;
       exploreSubGroup(child_group, new_path, path_list);
     }
   }
}



std::vector<std::string> traverseFile(HighFive::File input_file, int num_trs) {

  // Vector containing the path list to the HDF5 datasets
  std::vector<std::string> path_list; 

  std::string top_level_group_name = input_file.getPath();
  if (input_file.getObjectType(top_level_group_name) == HighFive::ObjectType::Group) {
    HighFive::Group parent_group = input_file.getGroup(top_level_group_name); 
    exploreSubGroup(parent_group, top_level_group_name, path_list);
  }
  // =====================================
  // THIS PART IS FOR TESTING ONLY
  // FIND A WAY TO USE THE HDF5DAtaStore 
  int i = 0;
  std::string prev_ds_name;
  for (auto& dataset_path : path_list) {
    if (dataset_path.find("Fragment") == std::string::npos && prev_ds_name.find("TriggerRecordHeader") != std::string::npos && i >= num_trs) {
       break;
    }
    if (dataset_path.find("TriggerRecordHeader") != std::string::npos) ++i;

    //readDataset(dataset_path);
    HighFive::Group parent_group = input_file.getGroup(top_level_group_name);
    HighFive::DataSet data_set = parent_group.getDataSet(dataset_path);
    HighFive::DataSpace thedataSpace = data_set.getSpace();
    size_t data_size = data_set.getStorageSize();
    char* membuffer = new char[data_size];
    data_set.read(membuffer);
    readDataset(dataset_path, membuffer);
    delete[] membuffer;

    prev_ds_name = dataset_path;
  }
  // =====================================
  
  return path_list;
}

int main(int argc, char** argv){

  int num_trs = 1000;
  if(argc <2) {
    std::cerr << "Usage: data_file_browser <fully qualified file name> [number of events to read]" << std::endl;
    return -1;
  }

  if(argc == 3) {
    num_trs = std::stoi(argv[2]);
  }   
  // Open the existing hdf5 file
  HighFive::File file(argv[1], HighFive::File::ReadOnly);

  std::vector<std::string> data_path = traverseFile(file, num_trs);
  return 0;
}
