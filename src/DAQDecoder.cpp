/**
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 *
 */

#include "DAQDecoder.hpp"

#include "dataformats/TriggerRecord.hpp"
#include "dataformats/wib/WIBFrame.hpp"

#include <sstream>
#include <iomanip>


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


// Read the HDF5 dataset and parse the single compoents
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
	  auto wf1ptr = reinterpret_cast<dunedaq::dataformats::WIBFrame*>((char*)(frag.get_data())+i*sizeof(dunedaq::dataformats::WIBFrame));
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




DAQDecoder::DAQDecoder(const std::string& file_name, const unsigned& num_events) {

  m_file_name = file_name; 
  m_number_events = num_events;

  m_run_number = extract_run_number_from_file_name();

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



//void DAQDecode::read_trigger_record_header() {}
void DAQDecoder::read_fragment(std::string dataset_path) {
  HighFive::Group parent_group = m_file_ptr->getGroup(m_top_level_group_name);
  HighFive::DataSet data_set = parent_group.getDataSet(dataset_path);
  HighFive::DataSpace data_space = data_set.getSpace();
  size_t data_size = data_set.getStorageSize();
  char* membuffer = new char[data_size];
  data_set.read(membuffer);
  readDataset(dataset_path, membuffer);
  delete[] membuffer;
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

int DAQDecoder::extract_run_number_from_file_name()
{
  std::vector<size_t> part_locs;
  size_t found = m_file_name.find("_");
  while(found!=std::string::npos){
    part_locs.emplace_back(found);
    found = m_file_name.find("/",found+1);
  }
  part_locs.emplace_back(m_file_name.size());

  for(size_t i=0; i<part_locs.size()-1; ++i){
    auto sub = m_file_name.substr(part_locs[i]+1,part_locs[i+1]-part_locs[i]-1);
    if(sub.find("run")==0)
      return std::stoi(sub.substr(3,sub.find(".")));
  }
  return -1;
}


/**
 * @brief Translate path to StorageKey
 */
dunedaq::hdf5libs::StorageKey DAQDecoder::make_key_from_path(std::string const& path)
{

  dunedaq::hdf5libs::StorageKey k;

  std::vector<size_t> path_locs;
  size_t found = path.find("/");
  while(found!=std::string::npos){
    path_locs.emplace_back(found);
    found = path.find("/",found+1);
  }
  path_locs.emplace_back(path.size());

  std::vector<std::string> substrings;
  for(size_t i=0; i<path_locs.size()-1; ++i){
    substrings.emplace_back(path.substr(path_locs[i]+1,path_locs[i+1]-path_locs[i]-1));
  }

  //run number is the first one? Should be ...
  //k.set_run_number(...)
  k.set_run_number(m_run_number);

  //TriggerRecordNumber next
  //Begins with 'TriggerRecord'
  k.set_trigger_number(std::stoi(substrings[1].substr(13)));

  //DataRecordGroupType next
  //will use string for lookup
  dunedaq::hdf5libs::DataRecordGroupType gt(substrings[2]);

  k.set_group_type(gt);

  if(gt.get_group_name()=="TriggerRecordHeader")
    return k;

  k.set_region_number(std::stoi(substrings[3].substr(gt.get_region_prefix().size())));
  k.set_element_number(std::stoi(substrings[4].substr(gt.get_element_prefix().size())));

  return k;

}

std::string DAQDecoder::make_path_from_key(const dunedaq::hdf5libs::StorageKey& k)
{
  std::stringstream ss_path;

  ss_path << "/"
          //<<no run number.... 
	  << "/"
	  << "TriggerRecord" << std::setfill('0') << std::setw(5) << k.get_trigger_number()
	  << "/"
	  << k.get_group_type().get_group_name();
  
  //TriggerRecordHeaders end here
  if(k.get_group_type().get_id()==dunedaq::hdf5libs::DataRecordGroupTypeID::kTriggerRecordHeader)
    return ss_path.str();
  
  ss_path << "/"
	  << k.get_group_type().get_region_prefix() << std::setfill('0') << std::setw(3) << k.get_region_number()
	  << "/"
	  << k.get_group_type().get_element_prefix() << std::setfill('0') << std::setw(2) << k.get_element_number();
  return ss_path.str();
}

void DAQDecoder::fill_storage_keys()
{
  m_storage_keys.clear();

  auto paths = get_datasets();

  for(auto const& path : paths)
    m_storage_keys.emplace_back(make_key_from_path(path));

}


/**
 * @brief Return a vector of datasets that correspond to a fragment
 */
std::vector<std::string> DAQDecoder::get_fragments(const unsigned& num_trs) {

 std::vector<std::string> fragment_path; 
 std::vector<std::string> dataset_path = this->get_datasets(); 

 unsigned trs_count = 0;
 for (auto& element : dataset_path) {
   if (element.find("Link") != std::string::npos && trs_count < num_trs) {
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

 unsigned trs_count = 0;
 for (auto& element : dataset_path) {
   if (element.find("TriggerRecordHeader") != std::string::npos && trs_count < num_trs) {
     trs_count += 1 ;
     trg_path.push_back(element);
   }
 }

 return trg_path;

}
