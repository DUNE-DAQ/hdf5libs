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


/*
 * @brief Constructor for writing a new file
 */
HDF5RawDataFile::HDF5RawDataFile(std::string file_name,
				 daqdataformats::run_number_t run_number,
				 size_t file_index,
				 std::string application_name,
				 const nlohmann::json& fl_params_conf,
				 unsigned open_flags)
{

  //check and make sure that the file isn't ReadOnly
  if (open_flags == HighFive::File::ReadOnly) {
    //throw wrong accessor
    return;
  }
  
  //do the file open
  try {
    m_file_handle.reset(new HDF5FileHandle(file_name, open_flags));
  } catch (std::exception const& excpt) {
    throw "Issue when opening file a file";
  } catch (...) { // NOLINT(runtime/exceptions)
    throw "Issue when opening file a file";
  }
  
  m_recorded_size = 0;
  
  int64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(system_clock::now().time_since_epoch()).count();
  std::string file_creation_timestamp = std::to_string(timestamp);
  
  TLOG_DEBUG(TLVL_BASIC) << "Created HDF5 file (" << file_name 
			 << ") at time " << file_creation_timestamp << " .";

  //write some file attributes
  write_attribute("run_number",run_number);
  write_attribute("file_index",file_index);
  write_attribute("creation_timestamp",file_creation_timestamp);
  write_attribute("application_name",application_name);
  
  //set the file layout contents
  m_file_layout_ptr.reset(new HDF5FileLayout(fl_params_conf.get<hdf5filelayout::FileLayoutParams>()));
  write_file_layout();
}

/*
 * @brief Write a TriggerRecord to the file.
 */
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

/*
 * @brief Write a TriggerRecordHeader to the file.
 */
void HDF5RawDataFile::write(const daqdataformats::TriggerRecordHeader& trh){

  m_recorded_size += do_write(get_path_elements(trh),
			      static_cast<const char*>(trh.get_storage_location()),
			      trh.get_total_size_bytes());
}

/*
 * @brief Write a Fragment to the file.
 */
void HDF5RawDataFile::write(const daqdataformats::Fragment& frag){

  m_recorded_size += do_write(get_path_elements(frag.get_header()),
			      static_cast<const char*>(frag.get_storage_location()),
			      frag.get_size());
}

/*
 * @brief write attribute to file
 */
template<typename T>
void HDF5RawDataFile::write_attribute(std::string name, T value)
{
  if(!m_file_handle->get_file_ptr()->hasAttribute(name))
    m_file_handle->get_file_ptr()->createAttribute(name,value);
}

/*
 * @brief write attribute to group
 */
template<typename T>
void HDF5RawDataFile::write_attribute(HighFive::Group* grp_ptr,std::string name, T value)
{
  if(!(grp_ptr->hasAttribute(name))){
    grp_ptr->createAttribute<T>(name,value);
  }
}

/*
 * @brief write attribute to dataset
 */
template<typename T>
void HDF5RawDataFile::write_attribute(HighFive::DataSet* d_ptr,std::string name, T value)
{
  if(!d_ptr->hasAttribute(name)){
    d_ptr->createAttribute<T>(name,value);
  }
}


/*
 * @brief Constructor for reading a file
 */
HDF5RawDataFile::HDF5RawDataFile(const std::string& file_name)
{

  //do the file open
  try {
    m_file_handle.reset(new HDF5FileHandle(file_name, HighFive::File::ReadOnly));
  } catch (std::exception const& excpt) {
    throw "Issue when opening file a file";
  } catch (...) { // NOLINT(runtime/exceptions)
    throw "Issue when opening file a file";
  }

  read_file_layout();
}



/*
 * @brief write the file layout
 */
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
		  "version_number",
		  m_file_layout_ptr->get_version());
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


/*
 * @brief write bytes to a dataset in the file, at the appropriate path
 */
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

/*
 * @brief get the correct path for the TriggerRecordHeader
 */
std::vector<std::string> HDF5RawDataFile::get_path_elements(const daqdataformats::TriggerRecordHeader& trh){

  std::vector<std::string> path_elements;

  //first the Trigger string
  path_elements.push_back(get_trigger_number_string(trh.get_trigger_number(),trh.get_sequence_number()));

  //then the TriggerRecordHeader dataset name
  path_elements.push_back(m_file_layout_ptr->get_trigger_header_dataset_name());


  return path_elements;

}

/*
 * @brief get the correct path for the Fragment
 */
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

/*
 * @brief get string for Trigger number
 */
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

  

template<typename T>
T HDF5RawDataFile::get_attribute(std::string name)
{
  if(!m_file_handle->get_file_ptr()->hasAttribute(name)){
    //throw that we don't have that attribute
  }
  auto attr = m_file_handle->get_file_ptr()->getAttribute(name);
  T value;
  attr.read(&value);
  return value;
}

template<typename T>
T HDF5RawDataFile::get_attribute(HighFive::Group* grp_ptr,std::string name)
{
  if(!(grp_ptr->hasAttribute(name))){
    //throw that we don't have that attribute
  }
  auto attr = grp_ptr->getAttribute(name);
  T value;
  attr.read(&value);
  return value;
}

template<typename T>
T HDF5RawDataFile::get_attribute(HighFive::DataSet* d_ptr,std::string name)
{
  if(!d_ptr->hasAttribute(name)){
    //throw that we don't have that attribute
  }
  auto attr = d_ptr->getAttribute(name);
  T value;
  attr.read(&value);
  return value;
}


void HDF5RawDataFile::read_file_layout()
{
  HighFive::File* hdf_file_ptr = m_file_handle->get_file_ptr();

  hdf5filelayout::FileLayoutParams fl_params;
  uint32_t version = 0;

  //get that group
  HighFive::Group fl_group = hdf_file_ptr->getGroup("DUNEDAQFileLayout");
  if (!fl_group.isValid()) {
    //warning message that we must be reading an old file?
    //now reset the HDF5Filelayout object, version 0
    m_file_layout_ptr.reset(new HDF5FileLayout(fl_params,version));
    return;
  }

  version = get_attribute<uint32_t>(&fl_group,"version_number");

  fl_params.trigger_record_name_prefix = get_attribute<std::string>(&fl_group,"trigger_record_name_prefix");
  fl_params.digits_for_trigger_number  = get_attribute<int32_t>(&fl_group,"digits_for_trigger_number");
  fl_params.digits_for_sequence_number = get_attribute<int32_t>(&fl_group,"digits_for_sequence_number");
  fl_params.trigger_record_header_dataset_name = get_attribute<std::string>(&fl_group,"trigger_record_header_dataset_name");

  //following code is to get the subgroups of fl_group, and then fill path_params for them

  //get list of objects that the fl_group has
  auto object_names = fl_group.listObjectNames();
  
  //now loop through groups
  for(auto const& oname : object_names){
    if(fl_group.getObjectType(oname)!=HighFive::ObjectType::Group) continue;
    HighFive::Group child_group = fl_group.getGroup(oname);

    hdf5filelayout::PathParams path_params;
    path_params.detector_group_type       = get_attribute<std::string>(&child_group,"detector_group_type");
    path_params.detector_group_name       = get_attribute<std::string>(&child_group,"detector_group_name");
    path_params.region_name_prefix        = get_attribute<std::string>(&child_group,"region_name_prefix");
    path_params.digits_for_region_number  = get_attribute<int32_t>(&child_group,"digits_for_region_number");
    path_params.element_name_prefix       = get_attribute<std::string>(&child_group,"element_name_prefix");
    path_params.digits_for_element_number = get_attribute<int32_t>(&child_group,"digits_for_element_number");

    fl_params.path_param_list.push_back(path_params);
  }

  //now reset the HDF5Filelayout object
  m_file_layout_ptr.reset(new HDF5FileLayout(fl_params,version));
}



/**
 * @brief Return a vector of datasets 
 */
std::vector<std::string> HDF5RawDataFile::get_datasets() {
 
  // Vector containing the path list to the HDF5 datasets
  std::vector<std::string> path_list;

  HighFive::File* hdf_file_ptr = m_file_handle->get_file_ptr();

  std::string top_level_group_name = hdf_file_ptr->getPath();
  if (hdf_file_ptr->getObjectType(top_level_group_name) == HighFive::ObjectType::Group) {
    HighFive::Group parent_group = hdf_file_ptr->getGroup(top_level_group_name);
    exploreSubGroup(parent_group, top_level_group_name, path_list);
  }

  return path_list;

}

/**
 * @brief Return a vector of datasets that correspond to a fragment
 */
std::vector<std::string> HDF5RawDataFile::get_fragments(const unsigned& start_tr, const unsigned& num_trs) {

 std::vector<std::string> fragment_path; 

 if(m_file_layout_ptr->get_version() < 1){

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
 }

 else{
   //use file layout ...
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

