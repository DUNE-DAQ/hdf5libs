/**
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 *
 */

#include "hdf5libs/HDF5RawDataFile.hpp"
#include "hdf5libs/hdf5filelayout/Nljs.hpp"

#include <sstream>

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

void HDF5RawDataFile::increment_file_index_if_needed(size_t size_of_next_write) {
  if ((m_recorded_size + size_of_next_write) > m_max_file_size && m_recorded_size > 0) {
    ++m_file_index;
    m_recorded_size = 0;
  }
}


HDF5RawDataFile::HDF5RawDataFile(const nlohmann::json& conf)
  : m_file_name("test")
  , m_open_flags_of_open_file(0)
  , m_run_number(0)
 {

   //m_file_layout_ptr.reset(new HDF5FileLayout(conf.get<hdf5filelayout::FileLayoutParams>()));

  try {
    std::cout<< "Filename: " << m_file_name << std::endl;
 


 
    /*
    // AAA: TO BE FIXED, HARDCODING FOR TESTING ONLY
    m_config_params = conf.get<hdf5datastore::ConfParams>();
    //m_key_translator_ptr.reset(new HDF5KeyTranslator(m_config_params));
    m_operation_mode = m_config_params.mode;
    m_path = m_config_params.directory_path;
    m_max_file_size = m_config_params.max_file_size_bytes;
    m_disable_unique_suffix = m_config_params.disable_unique_filename_suffix;
    m_free_space_safety_factor_for_write = m_config_params.free_space_safety_factor_for_write;
    */
    m_operation_mode = "one-fragment-per-file";
    std::string file_path = "/afs/cern.ch/user/a/aabedabu/work_public/hdf5libs_dev/work/build/hdf5libs/apps";
    m_path = file_path;
    m_max_file_size = 100000000;
    m_disable_unique_suffix = true;
    m_free_space_safety_factor_for_write = 2;
    m_run_number = 666; 

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
 
  } catch (std::exception const& excpt) {
    throw "Issue with parameters";
  } catch (...) {
    throw "Issue with parameters";
  } 

 }

void HDF5RawDataFile::write(const daqdataformats::TriggerRecord& tr){

  // We can use const_cast here since we're about to call non-const
  // functions on the trigger record object but not actually modifying
  // its contents
  
  auto& tr_fake_nonconst = const_cast<daqdataformats::TriggerRecord&>(tr);

  write(tr_fake_nonconst.get_header_ref());

  for(auto const& frag_ptr : tr_fake_nonconst.get_fragments_ref()) {
    write(*frag_ptr);
  }
}

void HDF5RawDataFile::write(const daqdataformats::TriggerRecordHeader& trh){

  m_recorded_size += do_write(get_path_elements(trh),
			      static_cast<const char*>(trh.get_storage_location()),
			      trh.get_total_size_bytes());
}

void HDF5RawDataFile::write(const daqdataformats::Fragment& frag){

  m_recorded_size += do_write(get_path_elements(frag.get_header()),
			      static_cast<const char*>(frag.get_storage_location()),
			      frag.get_size());
}

template<typename T>
void HDF5RawDataFile::write_attribute(std::string name, T value)
{
  if(!m_file_handle->get_file_ptr()->hasAttribute(name))
    m_file_handle->get_file_ptr()->createAttribute(name,value);
}

template<typename T>
void HDF5RawDataFile::write_attribute(HighFive::Group* grp_ptr,std::string name, T value)
{
  if(!(grp_ptr->hasAttribute(name))){
    grp_ptr->createAttribute<T>(name,value);
  }
}

template<typename T>
void HDF5RawDataFile::write_attribute(HighFive::DataSet* d_ptr,std::string name, T value)
{
  if(!d_ptr->hasAttribute(name)){
    d_ptr->createAttribute<T>(name,value);
  }
}

void HDF5RawDataFile::write_file_layout()
{

  HighFive::File* hdf_file_ptr = m_file_handle->get_file_ptr();

  //create top level group if needed
  if (!hdf_file_ptr->exist("DUNEDAQFileLayout"))
    hdf_file_ptr->createGroup("DUNEDAQFileLayout");

  //get that group
  HighFive::Group fl_group = hdf_file_ptr->getGroup("DUNEDAQFileLayout");
  if (!fl_group.isValid()) {
    //throw InvalidHDF5Group(ERS_HERE, top_level_group_name, top_level_group_name);
  }
  
  //attribute writing for the top-level group
  write_attribute(&fl_group,
		  "trigger_record_name_prefix",
		  m_file_layout_ptr->get_trigger_record_name_prefix());
  write_attribute(&fl_group,
		  "digits_for_trigger_number",
		  m_file_layout_ptr->get_digits_for_trigger_number());
  write_attribute(&fl_group,
		  "digits_for_sequence_number",
		  m_file_layout_ptr->get_digits_for_sequence_number());
  write_attribute(&fl_group,
		  "trigger_record_header_dataset_name",
		  m_file_layout_ptr->get_trigger_header_dataset_name());
  
  //now go through and get list of paths for subgroups
  auto path_params_map = m_file_layout_ptr->get_path_params_map();

  for(auto p_iter = path_params_map.begin(); p_iter != path_params_map.end(); ++p_iter){

    std::string const& child_group_name = p_iter->second.detector_group_name;
    if (child_group_name.empty()) {
      //throw InvalidHDF5Group(ERS_HERE, child_group_name, child_group_name);
    }
    if (!fl_group.exist(child_group_name)) {
      fl_group.createGroup(child_group_name);
    }
    HighFive::Group child_group = fl_group.getGroup(child_group_name);
    write_attribute(&child_group,
		    "detector_group_system_type",
		    (int)p_iter->first);
    write_attribute(&child_group,
		    "detector_group_name",
		    p_iter->second.detector_group_name);
    write_attribute(&child_group,
		    "detector_group_type",
		    p_iter->second.detector_group_type);
    write_attribute(&child_group,
		    "region_name_prefix",
		    p_iter->second.region_name_prefix);
    write_attribute(&child_group,
		    "digits_for_region_number",
		    p_iter->second.digits_for_region_number);
    write_attribute(&child_group,
		    "element_name_prefix",
		    p_iter->second.element_name_prefix);
    write_attribute(&child_group,
		    "digits_for_element_number",
		    p_iter->second.digits_for_element_number);
    
  }

}


size_t HDF5RawDataFile::do_write(std::vector<std::string> const& group_and_dataset_path_elements,
				 const char* raw_data_ptr,
				 size_t raw_data_size_bytes)
{
  const std::string dataset_name = group_and_dataset_path_elements.back();

  HighFive::File* hdf_file_ptr = m_file_handle->get_file_ptr();

  //create top level group if needed
  std::string const& top_level_group_name = group_and_dataset_path_elements.at(0);
  if (!hdf_file_ptr->exist(top_level_group_name))
    hdf_file_ptr->createGroup(top_level_group_name);
  
  //setup sub_group to work with
  HighFive::Group sub_group = hdf_file_ptr->getGroup(top_level_group_name);
  if (!sub_group.isValid()) {
    //throw InvalidHDF5Group(ERS_HERE, top_level_group_name, top_level_group_name);
  }

  // Create the remaining subgroups
  for (size_t idx = 1; idx < group_and_dataset_path_elements.size() - 1; ++idx) {
    // group_dataset.size()-1 because the last element is the dataset
    std::string const& child_group_name = group_and_dataset_path_elements[idx];
    if (child_group_name.empty()) {
      //throw InvalidHDF5Group(ERS_HERE, child_group_name, child_group_name);
    }
    if (!sub_group.exist(child_group_name)) {
      sub_group.createGroup(child_group_name);
    }
    HighFive::Group child_group = sub_group.getGroup(child_group_name);
    if (!child_group.isValid()) {
      //throw InvalidHDF5Group(ERS_HERE, child_group_name, child_group_name);
    }
    sub_group = child_group;
  }

  // Create dataset
  HighFive::DataSpace data_space = HighFive::DataSpace({ raw_data_size_bytes, 1 });
  HighFive::DataSetCreateProps data_set_create_props;
  HighFive::DataSetAccessProps data_set_access_props;
  
  try {
      auto data_set =
        sub_group.createDataSet<char>(dataset_name, data_space, 
				      data_set_create_props, data_set_access_props);
      if (data_set.isValid()) {
        data_set.write_raw(raw_data_ptr);
        hdf_file_ptr->flush();
	return raw_data_size_bytes;
      } 
      else {
        //throw InvalidHDF5Dataset(ERS_HERE, get_name(), dataset_name, hdf_file_ptr->getName());
      }
  } catch (std::exception const& excpt) {
    //std::string description = "DataSet " + dataset_name;
    //std::string msg = "writing DataSet " + dataset_name + " to file " + hdf_file_ptr->getName();
    //throw GeneralDataStoreProblem(ERS_HERE, get_name(), msg, excpt);
  } catch (...) { // NOLINT(runtime/exceptions)
    // NOLINT here because we *ARE* re-throwing the exception!
    //std::string msg = "writing DataSet " + dataset_name + " to file " + hdf_file_ptr->getName();
    //throw GeneralDataStoreProblem(ERS_HERE, get_name(), msg);
  }

  return 0;
}

std::vector<std::string> HDF5RawDataFile::get_path_elements(const daqdataformats::TriggerRecordHeader& trh){

  std::vector<std::string> path_elements;

  //first the Trigger string
  path_elements.push_back(get_trigger_number_string(trh.get_trigger_number(),trh.get_sequence_number()));

  //then the TriggerRecordHeader dataset name
  path_elements.push_back(m_file_layout_ptr->get_trigger_header_dataset_name());


  return path_elements;

}

std::vector<std::string> HDF5RawDataFile::get_path_elements(const daqdataformats::FragmentHeader& fh){

  std::vector<std::string> path_elements;

  //first the Trigger string
  path_elements.push_back(get_trigger_number_string(fh.trigger_number,fh.sequence_number));

  //then get the path params from our file layout for this type
  auto path_params = m_file_layout_ptr->get_path_params(fh.element_id.system_type);

  //next is the detector group name
  path_elements.push_back(path_params.detector_group_name);
  
  //then the region
  std::ostringstream region_string;
  region_string << path_params.region_name_prefix
		<< std::setw(path_params.digits_for_region_number)
		<< std::setfill('0') << fh.element_id.region_id;
  path_elements.push_back(region_string.str());
  
  //finally the element
  std::ostringstream element_string;
  element_string << path_params.element_name_prefix
		 << std::setw(path_params.digits_for_element_number)
		 << std::setfill('0') << fh.element_id.element_id;
  path_elements.push_back(element_string.str());

  return path_elements;

}

std::string HDF5RawDataFile::get_trigger_number_string(daqdataformats::trigger_number_t trig_num,
						       daqdataformats::sequence_number_t ) {

  std::ostringstream trigger_number_string;
  trigger_number_string << m_file_layout_ptr->get_trigger_record_name_prefix()
			<< std::setw(m_file_layout_ptr->get_digits_for_trigger_number()) << std::setfill('0')
			<< trig_num;

  /* don't do this for now?
  if (data_key.m_max_sequence_number > 0) {
    trigger_number_string << "." << std::setw(m_data_record_params.digits_for_sequence_number) << std::setfill('0')
			  << data_key.m_this_sequence_number;
  }
  */
  return trigger_number_string.str();
}

void HDF5RawDataFile::open_file_if_needed(const std::string& file_name, unsigned open_flags = HighFive::File::ReadOnly) {

    
    if (m_file_handle.get() == nullptr || m_basic_name_of_open_file.compare(file_name) ||
        m_open_flags_of_open_file != open_flags) {

      std::string unique_filename = file_name;
      //time_t now = time(0);
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
   * @brief HDF5RawDataFile  get_path_elements()
   *
   *
   */ 
  std::vector<std::string> HDF5RawDataFile::get_path_elements(const StorageKey& data_key) {

    std::vector<std::string> path_list;
    // verify that the requested detector group type is in our map of parameters
    //if (data_key.get_group_type() !=StorageKey::DataRecordGroupType::kTriggerRecordHeader) {
    //    std::string msg = "Requested detector group type is in our map of parameters";
    //    throw msg;
    //}

    // add trigger number to the path
    std::ostringstream trigger_number_string;
    trigger_number_string <<  data_key.get_trigger_number();

    if (data_key.m_max_sequence_number > 0) {
      trigger_number_string << "." << data_key.m_this_sequence_number;
    }
 
    path_list.push_back(trigger_number_string.str());

    
    // add trigger number to the path
    trigger_number_string << data_key.get_trigger_number();
 
    if (data_key.m_max_sequence_number > 0) {
      trigger_number_string << "." << data_key.m_this_sequence_number;
    }
    path_list.push_back(trigger_number_string.str());

/*
    // AAA: taken from dfmodules

    if (data_key.get_group_type() != StorageKey::DataRecordGroupType::kTriggerRecordHeader) {
      // Add group type
      path_list.push_back(m_path_param_map[data_key.get_group_type()].detector_group_name);

      // next, we translate the region number
      std::ostringstream region_number_string;
      region_number_string << m_path_param_map[data_key.get_group_type()].region_name_prefix
                           << std::setw(m_path_param_map[data_key.get_group_type()].digits_for_region_number)
                           << std::setfill('0') << data_key.get_region_number();
      path_list.push_back(region_number_string.str());

      // Finally, add element number
      std::ostringstream element_number_string;
      element_number_string << m_path_param_map[data_key.get_group_type()].element_name_prefix
                            << std::setw(m_path_param_map[data_key.get_group_type()].digits_for_element_number)
                            << std::setfill('0') << data_key.get_element_number();
      path_list.push_back(element_number_string.str());
    } else {
      // Add TriggerRecordHeader instead of group type
      path_list.push_back("TriggerRecordHeader");
    }

    return path_list;


*/
  return path_list;

  }



  /**
   * @brief HDF5RawDataFile  do_write()
   * Method used to flush data to HDF5
   *
   */
  void HDF5RawDataFile::do_write(const KeyedDataBlock& data_block) {

    TLOG_DEBUG(TLVL_BASIC) << " Writing data with trigger number " << data_block.m_data_key.get_trigger_number() << " and group type "
                           << data_block.m_data_key.get_group_type() << " and region/element number "
                           << data_block.m_data_key.get_region_number() << " / "
                           << data_block.m_data_key.get_element_number();
    
    //HighFive::File* hdf_file_ptr = m_file_handle->get_file_ptr();

    
    std::vector<std::string> group_and_dataset_path_elements =
      get_path_elements(data_block.m_data_key);

    //for (auto& element : group_and_dataset_path_elements) {
    //  std::cout << element << std::endl;
    //}

    /*
    const std::string dataset_name = group_and_dataset_path_elements.back();

    HighFive::Group sub_group = HDF5FileUtils::get_subgroup(hdf_file_ptr, group_and_dataset_path_elements, true);

    // Create dataset
    HighFive::DataSpace data_space = HighFive::DataSpace({ data_block.m_data_size, 1 });
    HighFive::DataSetCreateProps data_set_create_props;
    HighFive::DataSetAccessProps data_set_access_props;

    try {
      auto data_set =
        sub_group.createDataSet<char>(dataset_name, data_space, data_set_create_props, data_set_access_props);
      if (data_set.isValid()) {
        data_set.write_raw(static_cast<const char*>(data_block.get_data_start()));
        hdf_file_ptr->flush();
        m_recorded_size += data_block.m_data_size;
      } else {
        throw InvalidHDF5Dataset(ERS_HERE, get_name(), dataset_name, hdf_file_ptr->getName());
      }
    } catch (std::exception const& excpt) {
      std::string description = "DataSet " + dataset_name;
      std::string msg = "writing DataSet " + dataset_name + " to file " + hdf_file_ptr->getName();
      throw GeneralDataStoreProblem(ERS_HERE, get_name(), msg, excpt);
    } catch (...) { // NOLINT(runtime/exceptions)
      // NOLINT here because we *ARE* re-throwing the exception!
      std::string msg = "writing DataSet " + dataset_name + " to file " + hdf_file_ptr->getName();
      throw GeneralDataStoreProblem(ERS_HERE, get_name(), msg);
    }
  */
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


    // check if a new file should be opened for this data block
    this->increment_file_index_if_needed(data_block.m_data_size);

    // determine the filename from Storage Key + configuration parameters
    std::string full_filename = "test.txt";
    //std::string full_filename = m_key_translator_ptr->get_file_name(data_block.m_data_key, m_file_index);

    try {
      open_file_if_needed(full_filename, HighFive::File::OpenOrCreate);
    } catch (std::exception const& excpt) {
      throw "Cannot open file";
    } catch (...) { // NOLINT(runtime/exceptions)
      throw "Cannot open file";
    }

    // write the data block
    do_write(data_block);
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

 unsigned int trs_count = 0;
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

 unsigned int trs_count = 0;
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

  HighFive::File* hdf_file_ptr = m_file_handle->get_file_ptr();

  std::vector<std::string> list_attribute_names = hdf_file_ptr->listAttributeNames();
  for(std::string& attribute_name : list_attribute_names) {
    //std::cout << attribute_name << std::endl; 
    if (hdf_file_ptr->hasAttribute(attribute_name.c_str())) {
      HighFive::Attribute high_five_attribute = hdf_file_ptr->getAttribute(attribute_name.c_str());
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



std::unique_ptr<daqdataformats::Fragment> HDF5RawDataFile::get_frag_ptr(const std::string& dataset_name){
  HighFive::Group parent_group = m_file_ptr->getGroup(m_top_level_group_name);
  HighFive::DataSet data_set = parent_group.getDataSet(dataset_name);
  HighFive::DataSpace data_space = data_set.getSpace();
  size_t data_size = data_set.getStorageSize();
  
  char* membuffer = new char[data_size];
  data_set.read(membuffer);
  //readDataset(dataset_path, membuffer);
  std::unique_ptr<daqdataformats::Fragment> frag(new dunedaq::daqdataformats::Fragment(membuffer, dunedaq::daqdataformats::Fragment::BufferAdoptionMode::kTakeOverBuffer));
  //delete[] membuffer;
  return std::move(frag);
} 

std::unique_ptr<daqdataformats::TriggerRecordHeader> HDF5RawDataFile::get_trh_ptr (const std::string& dataset_name) {
  HighFive::Group parent_group = m_file_ptr->getGroup(m_top_level_group_name);
  HighFive::DataSet data_set = parent_group.getDataSet(dataset_name);
  HighFive::DataSpace data_space = data_set.getSpace();
  size_t data_size = data_set.getStorageSize();

  char* membuffer = new char[data_size];
  data_set.read(membuffer);
  std::unique_ptr<daqdataformats::TriggerRecordHeader> trh(new dunedaq::daqdataformats::TriggerRecordHeader(membuffer,true));
  delete[] membuffer;
  return std::move(trh);
}

} // hdf5libs
} // dunedaq

