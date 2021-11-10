/**
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 *
 */

#include "hdf5libs/DAQDecoder.hpp"


namespace dunedaq {
namespace hdf5libs {


// HDF5 Utility function to recursively traverse a file
void exploreSubGroup(HighFive::Group parent_group, std::string relative_path, std::vector<std::string>& path_list){
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

DAQDecoder::DAQDecoder(const std::string& file_name, const unsigned& num_events) {

  m_file_name = file_name; 
  m_number_events = num_events;
  try {
    m_file_ptr.reset(new HighFive::File(m_file_name, HighFive::File::ReadOnly));
    TLOG_DEBUG(TLVL_BASIC) << "get_name()" << "Opened HDF5 file in read-only.";

    m_top_level_group_name = m_file_ptr->getPath();
   
  } catch (std::exception const& excpt) {
    // TODO: add ERS exceptions 
    throw "FileOperationProblem - ADD ERS";
  } catch (...) {
    throw "FileOperationProblem - ADD ERS";
  } 

}

/**
 * @brief Return a vector of datasets 
 */
std::vector<std::string> DAQDecoder::get_datasets() {
 
  // Vector containing the path list to the HDF5 datasets
  std::vector<std::string> path_list;

  std::string top_level_group_name = m_file_ptr->getPath();
  if (m_file_ptr->getObjectType(top_level_group_name) == HighFive::ObjectType::Group) {
    HighFive::Group parent_group = m_file_ptr->getGroup(top_level_group_name);
    exploreSubGroup(parent_group, top_level_group_name, path_list);
  }

  return path_list;

}

/**
 * @brief Return a vector of datasets that correspond to a fragment
 */
std::vector<std::string> DAQDecoder::get_fragments(const unsigned& num_trs) {

 std::vector<std::string> fragment_path; 
 std::vector<std::string> dataset_path = this->get_datasets(); 

 int trs_count = 0;
 for (auto& element : dataset_path) {
   if (element.find("Element") != std::string::npos && trs_count < num_trs) {
       fragment_path.push_back(element);
   }
   else if (element.find("Link") != std::string::npos && trs_count < num_trs) {
       fragment_path.push_back(element);
   }
   else if (element.find("TriggerRecordHeader") != std::string::npos) {
     trs_count += 1 ;
   }
 }

 return fragment_path;

}

/**
 * @brief Return a vector of datasets that correspond to a TRH
 */
std::vector<std::string> DAQDecoder::get_trh(const unsigned& num_trs) {

 std::vector<std::string> trg_path; 
 
 std::vector<std::string> dataset_path = this->get_datasets(); 

 int trs_count = 0;
 for (auto& element : dataset_path) {
   if (element.find("TriggerRecordHeader") != std::string::npos && trs_count < num_trs) {
     trs_count += 1 ;
     trg_path.push_back(element);
   }
 }

 return trg_path;

}

std::unique_ptr<dunedaq::daqdataformats::Fragment> DAQDecoder::get_frag_ptr(const std::string& dataset_name){
  HighFive::Group parent_group = m_file_ptr->getGroup(m_top_level_group_name);
  HighFive::DataSet data_set = parent_group.getDataSet(dataset_name);
  HighFive::DataSpace data_space = data_set.getSpace();
  size_t data_size = data_set.getStorageSize();
  
  char* membuffer = new char[data_size];
  data_set.read(membuffer);
  //readDataset(dataset_path, membuffer);
  std::unique_ptr<dunedaq::daqdataformats::Fragment> frag(new dunedaq::daqdataformats::Fragment(membuffer, dunedaq::daqdataformats::Fragment::BufferAdoptionMode::kTakeOverBuffer));
  //delete[] membuffer;
  return std::move(frag);
} 

std::unique_ptr<dunedaq::daqdataformats::TriggerRecordHeader> DAQDecoder::get_trh_ptr (const std::string& dataset_name) {
  HighFive::Group parent_group = m_file_ptr->getGroup(m_top_level_group_name);
  HighFive::DataSet data_set = parent_group.getDataSet(dataset_name);
  HighFive::DataSpace data_space = data_set.getSpace();
  size_t data_size = data_set.getStorageSize();

  char* membuffer = new char[data_size];
  data_set.read(membuffer);
  std::unique_ptr<dunedaq::daqdataformats::TriggerRecordHeader> trh(new dunedaq::daqdataformats::TriggerRecordHeader(membuffer,true));
  delete[] membuffer;
  return std::move(trh);
}

} // hdf5libs
} // dunedaq

