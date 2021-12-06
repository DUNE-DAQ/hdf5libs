/**
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 *
 */

#include "hdf5libs/HDF5RawDataFile.hpp"


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


size_t get_free_space(const std::string& the_path) {
    struct statvfs vfs_results;
    int retval = statvfs(the_path.c_str(), &vfs_results);
    if (retval != 0) {
      return 0;
    }
    return vfs_results.f_bsize * vfs_results.f_bavail;
}




HDF5RawDataFile::HDF5RawDataFile(const nlohmann::json& conf)
  : m_file_name("test")
  , m_open_flags_of_open_file(0)
  , m_run_number(0)
 {
  try {
    std::cout<< "Filename: " << m_file_name << std::endl;
/*
    m_config_params = conf.get<hdf5datastore::ConfParams>();
    //m_key_translator_ptr.reset(new HDF5KeyTranslator(m_config_params));
    m_operation_mode = m_config_params.mode;
    m_path = m_config_params.directory_path;
    m_max_file_size = m_config_params.max_file_size_bytes;
    m_disable_unique_suffix = m_config_params.disable_unique_filename_suffix;
    m_free_space_safety_factor_for_write = m_config_params.free_space_safety_factor_for_write;
    if (m_free_space_safety_factor_for_write < 1.1) {
      m_free_space_safety_factor_for_write = 1.1;
    }

    m_file_index = 0;
    m_recorded_size = 0;

    if (m_operation_mode != "one-event-per-file" && m_operation_mode != "one-fragment-per-file" &&
        m_operation_mode != "all-per-file") {

      throw "Invalid operation mode";
    }

    m_application_name = "Unknown";
    char* appname_ptr = getenv("DUNEDAQ_APPLICATION_NAME");
    if (appname_ptr != nullptr) {
      std::string tmpstr(appname_ptr);
      m_application_name = tmpstr;
    }
*/
 
  } catch (std::exception const& excpt) {
    throw "Issue with parameters";
  } catch (...) {
    throw "Issue with parameters";
  } 

}

void HDF5RawDataFile::open_file_if_needed(const std::string& file_name, unsigned open_flags = HighFive::File::ReadOnly) {

    
    if (m_file_handle.get() == nullptr || m_basic_name_of_open_file.compare(file_name) ||
        m_open_flags_of_open_file != open_flags) {

      std::string unique_filename = file_name;
      time_t now = time(0);
      int64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(system_clock::now().time_since_epoch()).count();
      std::string file_creation_timestamp = std::to_string(timestamp);

      // close an existing open file
      if (m_file_handle.get() != nullptr) {
        std::string open_filename = m_file_handle->get_file_ptr()->getName();
        try {
          m_file_handle.reset();
        } catch (std::exception const& excpt) {
          throw "Issue when closing an existing open file";
        } catch (...) { // NOLINT(runtime/exceptions)
          throw "Issue when closing an existing open file";
        }
      }

      // opening file for the first time OR something changed in the name or the way of opening the file
      TLOG_DEBUG(TLVL_BASIC) << " going to open file " << unique_filename << " with open_flags "
                             << std::to_string(open_flags);
      m_basic_name_of_open_file = file_name;
      m_open_flags_of_open_file = open_flags;
      try {
        m_file_handle.reset(new HDF5FileHandle(unique_filename, open_flags));
      } catch (std::exception const& excpt) {
        throw "Issue when opening file a file";
      } catch (...) { // NOLINT(runtime/exceptions)
        throw "Issue when opening file a file";
      }

      if (open_flags == HighFive::File::ReadOnly) {
        TLOG_DEBUG(TLVL_BASIC) << "Opened HDF5 file read-only.";
      } else {
        TLOG_DEBUG(TLVL_BASIC) << "Created HDF5 file (" << unique_filename << ").";

        if (!m_file_handle->get_file_ptr()->hasAttribute("run_number")) {
          m_file_handle->get_file_ptr()->createAttribute("run_number", m_run_number);
        }
        if (!m_file_handle->get_file_ptr()->hasAttribute("file_index")) {
          m_file_handle->get_file_ptr()->createAttribute("file_index", m_file_index);
        }
        if (!m_file_handle->get_file_ptr()->hasAttribute("creation_timestamp")) {
          m_file_handle->get_file_ptr()->createAttribute("creation_timestamp", file_creation_timestamp);
        }
        if (!m_file_handle->get_file_ptr()->hasAttribute("application_name")) {
          m_file_handle->get_file_ptr()->createAttribute("application_name", m_application_name);
        }
      }
    } else {
      TLOG_DEBUG(TLVL_BASIC) << " Pointer file to  " << m_basic_name_of_open_file
                             << " was already opened with open_flags " << std::to_string(m_open_flags_of_open_file);
    }

}


  /**
   * @brief HDF5RawDataFile  write()
   * Method used to write constant data
   * into HDF5 format. Operational mode
   * defined in the configuration file.
   *
   */
   void HDF5RawDataFile::write(const KeyedDataBlock& data_block) {


    // check if there is sufficient space for this data block
    size_t current_free_space = get_free_space(m_path);
    if (current_free_space < (m_free_space_safety_factor_for_write * data_block.m_data_size)) {
      std::string msg = "writing a data block to file " + m_file_handle->get_file_ptr()->getName();
      throw msg;
    }

/*
    // check if a new file should be opened for this data block
    increment_file_index_if_needed(data_block.m_data_size);

    // determine the filename from Storage Key + configuration parameters
    std::string full_filename = m_key_translator_ptr->get_file_name(data_block.m_data_key, m_file_index);

    try {
      open_file_if_needed(full_filename, HighFive::File::OpenOrCreate);
    } catch (std::exception const& excpt) {
      throw "Cannot open file";
    } catch (...) { // NOLINT(runtime/exceptions)
      throw "Cannot open file";
    }

    // write the data block
    // do_write(data_block);
*/
  }


// =============================================================================================================
// =============================================================================================================
// =============================================================================================================








/**
 * @brief Return a vector of datasets 
 */
std::vector<std::string> HDF5RawDataFile::get_datasets() {
 
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
std::vector<std::string> HDF5RawDataFile::get_fragments(const unsigned& start_tr, const unsigned& num_trs) {

 std::vector<std::string> fragment_path; 
 std::vector<std::string> dataset_path = this->get_datasets(); 

 int trs_count = 0;
 for (auto& element : dataset_path) {
   if (element.find("Element") != std::string::npos && trs_count < start_tr+num_trs && trs_count >= start_tr) {
       fragment_path.push_back(element);
   }
   else if (element.find("Link") != std::string::npos && trs_count < start_tr+num_trs && trs_count >= start_tr) {
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
std::vector<std::string> HDF5RawDataFile::get_trh(const unsigned& start_tr, const unsigned& num_trs) {

 std::vector<std::string> trg_path; 
 
 std::vector<std::string> dataset_path = this->get_datasets(); 

 int trs_count = 0;
 for (auto& element : dataset_path) {
   if (element.find("TriggerRecordHeader") != std::string::npos && trs_count < start_tr+num_trs && trs_count >= start_tr) {
     trg_path.push_back(element);
   }
   trs_count += 1 ;
 }

 return trg_path;

}

/**
 * @brief Return a map with all the HDF5 attributes
 */
std::map<std::string, std::variant<std::string, int>> HDF5RawDataFile::get_attributes() {
  std::map<std::string, std::variant<std::string, int>> attributes_map;  

  std::vector<std::string> list_attribute_names = m_file_ptr->listAttributeNames();
  for(std::string& attribute_name : list_attribute_names) {
    if (m_file_ptr->hasAttribute(attribute_name.c_str())) {
      HighFive::Attribute high_five_attribute = m_file_ptr->getAttribute(attribute_name.c_str());
      HighFive::DataType attribute_data_type = high_five_attribute.getDataType();
      if (attribute_data_type.string() == "String64") {
        std::string attribute_string;
        high_five_attribute.read(attribute_string);
        attributes_map[attribute_name] = attribute_string; 
      } else {
        size_t attribute_val;
        high_five_attribute.read(attribute_val);
        attributes_map[attribute_name] = attribute_val;
      }

    }
  }
  
  return attributes_map;

}



std::unique_ptr<dunedaq::daqdataformats::Fragment> HDF5RawDataFile::get_frag_ptr(const std::string& dataset_name){
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

std::unique_ptr<dunedaq::daqdataformats::TriggerRecordHeader> HDF5RawDataFile::get_trh_ptr (const std::string& dataset_name) {
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

