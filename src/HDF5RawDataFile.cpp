/**
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 *
 */

#include "hdf5libs/HDF5RawDataFile.hpp"
#include "hdf5libs/hdf5filelayout/Nljs.hpp"

#include <algorithm>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>


namespace dunedaq {
namespace hdf5libs {

constexpr uint32_t MAX_FILELAYOUT_VERSION=4294967295; // NOLINT(build/unsigned)

/**
 * @brief Constructor for writing a new file
 */
HDF5RawDataFile::HDF5RawDataFile(std::string file_name,
                                 daqdataformats::run_number_t run_number,
                                 size_t file_index,
                                 std::string application_name,
                                 const hdf5filelayout::FileLayoutParams& fl_params,
                                 unsigned open_flags)
  : m_open_flags(open_flags)
{

  // check and make sure that the file isn't ReadOnly
  if (m_open_flags == HighFive::File::ReadOnly) {
    throw IncompatibleOpenFlags(ERS_HERE,file_name,m_open_flags);
  }

  // do the file open
  try {
    m_file_ptr.reset(new HighFive::File(file_name, m_open_flags));
  } catch (std::exception const& excpt) {
    throw FileOpenFailed(ERS_HERE, file_name, excpt.what());
  }

  m_recorded_size = 0;

  int64_t timestamp =
    std::chrono::duration_cast<std::chrono::milliseconds>(system_clock::now().time_since_epoch()).count();
  std::string file_creation_timestamp = std::to_string(timestamp);

  TLOG_DEBUG(TLVL_BASIC) << "Created HDF5 file (" << file_name << ") at time " << file_creation_timestamp << " .";

  // write some file attributes
  write_attribute("run_number", run_number);
  write_attribute("file_index", file_index);
  write_attribute("creation_timestamp", file_creation_timestamp);
  write_attribute("application_name", application_name);

  // set the file layout contents
  m_file_layout_ptr.reset(new HDF5FileLayout(fl_params));
  write_file_layout();
}

HDF5RawDataFile::~HDF5RawDataFile()
{
  if (m_file_ptr.get() != nullptr && m_open_flags != HighFive::File::ReadOnly) {
    write_attribute("recorded_size", m_recorded_size);

    int64_t timestamp =
      std::chrono::duration_cast<std::chrono::milliseconds>(system_clock::now().time_since_epoch()).count();
    std::string file_closing_timestamp = std::to_string(timestamp);
    write_attribute("closing_timestamp", file_closing_timestamp);

    m_file_ptr->flush();
  }

  // explicit destruction; not really needed, but nice to be clear...
  m_file_ptr.reset();
  m_file_layout_ptr.reset();
}

/**
 * @brief Write a TriggerRecord to the file.
 */
void
HDF5RawDataFile::write(const daqdataformats::TriggerRecord& tr)
{
  write(tr.get_header_ref());

  for (auto const& frag_ptr : tr.get_fragments_ref()) {
    write(*frag_ptr);
  }
}

/**
 * @brief Write a TriggerRecordHeader to the file.
 */
void
HDF5RawDataFile::write(const daqdataformats::TriggerRecordHeader& trh)
{

  m_recorded_size += do_write(m_file_layout_ptr->get_path_elements(trh),
                              static_cast<const char*>(trh.get_storage_location()),
                              trh.get_total_size_bytes());
}

/**
 * @brief Write a Fragment to the file.
 */
void
HDF5RawDataFile::write(const daqdataformats::Fragment& frag)
{

  m_recorded_size += do_write(m_file_layout_ptr->get_path_elements(frag.get_header()),
                              static_cast<const char*>(frag.get_storage_location()),
                              frag.get_size());
}

/**
 * @brief Constructor for reading a file
 */
HDF5RawDataFile::HDF5RawDataFile(const std::string& file_name)
  : m_open_flags(HighFive::File::ReadOnly)
{

  // do the file open
  try {
    m_file_ptr = std::make_unique<HighFive::File>(file_name, m_open_flags);
  } catch (std::exception const& excpt) {
    throw FileOpenFailed(ERS_HERE, file_name, excpt.what());
  }

  if (m_file_ptr->hasAttribute("recorded_size"))
    m_recorded_size = get_attribute<size_t>("recorded_size");
  else
    m_recorded_size = 0;

  read_file_layout();
}

/**
 * @brief write the file layout
 */
void
HDF5RawDataFile::write_file_layout()
{
  hdf5filelayout::data_t fl_json;
  hdf5filelayout::to_json(fl_json,m_file_layout_ptr->get_file_layout_params());
  write_attribute("filelayout_params",fl_json.dump());
  write_attribute("filelayout_version",m_file_layout_ptr->get_version());
}

/**
 * @brief write bytes to a dataset in the file, at the appropriate path
 */
size_t
HDF5RawDataFile::do_write(std::vector<std::string> const& group_and_dataset_path_elements,
                          const char* raw_data_ptr,
                          size_t raw_data_size_bytes)
{
  const std::string dataset_name = group_and_dataset_path_elements.back();

  // create top level group if needed
  std::string const& top_level_group_name = group_and_dataset_path_elements.at(0);
  if (!m_file_ptr->exist(top_level_group_name))
    m_file_ptr->createGroup(top_level_group_name);

  // setup sub_group to work with
  HighFive::Group sub_group = m_file_ptr->getGroup(top_level_group_name);
  if (!sub_group.isValid()) {
    throw InvalidHDF5Group(ERS_HERE, top_level_group_name);
  }

  // Create the remaining subgroups
  for (size_t idx = 1; idx < group_and_dataset_path_elements.size() - 1; ++idx) {
    // group_dataset.size()-1 because the last element is the dataset
    std::string const& child_group_name = group_and_dataset_path_elements[idx];
    if (child_group_name.empty()) {
      throw InvalidHDF5Group(ERS_HERE, child_group_name);
    }
    if (!sub_group.exist(child_group_name)) {
      sub_group.createGroup(child_group_name);
    }
    HighFive::Group child_group = sub_group.getGroup(child_group_name);
    if (!child_group.isValid()) {
      throw InvalidHDF5Group(ERS_HERE, child_group_name);
    }
    sub_group = child_group;
  }

  // Create dataset
  HighFive::DataSpace data_space = HighFive::DataSpace({ raw_data_size_bytes, 1 });
  HighFive::DataSetCreateProps data_set_create_props;
  HighFive::DataSetAccessProps data_set_access_props;

  auto data_set = sub_group.createDataSet<char>(dataset_name, data_space, data_set_create_props, data_set_access_props);
  if (data_set.isValid()) {
    data_set.write_raw(raw_data_ptr);
    m_file_ptr->flush();
    return raw_data_size_bytes;
  } else {
    throw InvalidHDF5Dataset(ERS_HERE, dataset_name, m_file_ptr->getName());
  }

  return 0;
}

size_t
get_free_space(const std::string& the_path)
{
  struct statvfs vfs_results;
  int retval = statvfs(the_path.c_str(), &vfs_results);
  if (retval < 0) {
    return 0;
  }

  return vfs_results.f_bfree * vfs_results.f_bsize;
}

void
HDF5RawDataFile::read_file_layout()
{
  hdf5filelayout::FileLayoutParams fl_params;
  uint32_t version = 0; // NOLINT(build/unsigned)

  std::string fl_str;
  try{
    fl_str = get_attribute<std::string>("filelayout_params");
    hdf5filelayout::data_t fl_json = nlohmann::json::parse(fl_str);
    hdf5filelayout::from_json(fl_json,fl_params);

    version = get_attribute<uint32_t>("filelayout_version"); // NOLINT(build/unsigned)

  }catch(InvalidHDF5Attribute const&){
    ers::info(MissingFileLayout(ERS_HERE,version));
  }
  
  // now reset the HDF5Filelayout object
  m_file_layout_ptr.reset(new HDF5FileLayout(fl_params, version));
}

// HDF5 Utility function to recursively traverse a file
void
HDF5RawDataFile::explore_subgroup(const HighFive::Group& parent_group,
				  std::string relative_path,
				  std::vector<std::string>& path_list)
{
  if (relative_path.size() > 0 && relative_path.compare(relative_path.size() - 1, 1, "/") == 0)
    relative_path.pop_back();

  std::vector<std::string> childNames = parent_group.listObjectNames();

  for (auto& child_name : childNames) {
    std::string full_path = relative_path + "/" + child_name;
    HighFive::ObjectType child_type = parent_group.getObjectType(child_name);

    if (child_type == HighFive::ObjectType::Dataset) {
      path_list.push_back(full_path);
    } else if (child_type == HighFive::ObjectType::Group) {
      HighFive::Group child_group = parent_group.getGroup(child_name);
      // start the recusion
      std::string new_path = relative_path + "/" + child_name;
      explore_subgroup(child_group, new_path, path_list);
    }
  }
}

/**
 * @brief Return a vector of dataset names
 */
std::vector<std::string>
HDF5RawDataFile::get_dataset_paths(std::string top_level_group_name)
{
  if (top_level_group_name.empty())
    top_level_group_name = m_file_ptr->getPath();

  // Vector containing the path list to the HDF5 datasets
  std::vector<std::string> path_list;

  HighFive::Group parent_group = m_file_ptr->getGroup(top_level_group_name);
  if(!parent_group.isValid())
    throw InvalidHDF5Group(ERS_HERE,top_level_group_name);

  explore_subgroup(parent_group, top_level_group_name, path_list);

  return path_list;
}

/**
 * @brief Return all of the record numbers in the file.
 */
std::set<daqdataformats::trigger_number_t>
HDF5RawDataFile::get_all_record_numbers()
{
  std::set<daqdataformats::trigger_number_t> record_numbers;

  // records are at the top level

  HighFive::Group parent_group = m_file_ptr->getGroup(m_file_ptr->getPath());

  std::vector<std::string> childNames = parent_group.listObjectNames();
  const std::string record_prefix = m_file_layout_ptr->get_trigger_record_name_prefix();
  const size_t record_prefix_size = record_prefix.size();

  for (auto const& name : childNames) {
    auto loc = name.find(record_prefix);

    if (loc == std::string::npos)
      continue;

    if (name.find(".") && get_version() < 2)
      throw IncompatibleFileLayoutVersion(ERS_HERE,get_version(),2,MAX_FILELAYOUT_VERSION);

    auto trstring = name.substr(loc + record_prefix_size);
    trstring.resize(m_file_layout_ptr->get_digits_for_trigger_number());
    record_numbers.insert(std::stoi(trstring));
  }

  return record_numbers;
}

std::set<daqdataformats::trigger_number_t>
HDF5RawDataFile::get_all_trigger_record_numbers()
{
  return get_all_record_numbers();
}

/**
 * @brief Return a vector of dataset names that correspond to TriggerRecordHeaders
 */
std::vector<std::string>
HDF5RawDataFile::get_trigger_record_header_dataset_paths()
{

  std::vector<std::string> trh_paths;

  if(get_version()>=2){
  
    auto record_numbers = get_all_trigger_record_numbers();
    
    for (auto const& trig_num : get_all_trigger_record_numbers())
      trh_paths.push_back(m_file_ptr->getPath() + m_file_layout_ptr->get_trigger_record_header_path(trig_num));
    
  }
  else{
    for(auto const& path : get_dataset_paths())
      if(path.find(m_file_layout_ptr->get_trigger_header_dataset_name()) != std::string::npos)
	trh_paths.push_back(path);
  }

  return trh_paths;

}

/**
 * @brief Return a vector of dataset names that correspond to Fragemnts
 * Note: this gets all datsets, and then removes those that look like TriggerRecordHeader ones
 *       one could instead loop through all system types and ask for appropriate datsets in those
 *       however, probably that's more time consuming
 */
std::vector<std::string>
HDF5RawDataFile::get_all_fragment_dataset_paths()
{
  std::vector<std::string> frag_paths;

  for (auto const& path : get_dataset_paths()) {
      if (path.find(m_file_layout_ptr->get_trigger_header_dataset_name()) == std::string::npos)
        frag_paths.push_back(path);
  }

  return frag_paths;
}

std::unique_ptr<char[]>
HDF5RawDataFile::get_dataset_raw_data(const std::string& dataset_path)
{

  HighFive::Group parent_group = m_file_ptr->getGroup("/");
  HighFive::DataSet data_set = parent_group.getDataSet(dataset_path);
  
  if(!data_set.isValid())
    throw InvalidHDF5Dataset(ERS_HERE,dataset_path,get_file_name());

  size_t data_size = data_set.getStorageSize();
    
  auto membuffer = std::make_unique<char[]>(data_size);
  data_set.read(membuffer.get());
  return std::move(membuffer);

}

std::unique_ptr<daqdataformats::Fragment>
HDF5RawDataFile::get_frag_ptr(const std::string& dataset_name)
{
  auto membuffer = get_dataset_raw_data(dataset_name);
  auto frag_ptr = std::make_unique<daqdataformats::Fragment>(
    membuffer.release(), dunedaq::daqdataformats::Fragment::BufferAdoptionMode::kTakeOverBuffer);
  return std::move(frag_ptr);
}

std::unique_ptr<daqdataformats::Fragment>
HDF5RawDataFile::get_frag_ptr(const daqdataformats::trigger_number_t trig_num,
                              const daqdataformats::sequence_number_t seq_num,
                              const daqdataformats::GeoID element_id)
{
  if(get_version() < 2)
    throw IncompatibleFileLayoutVersion(ERS_HERE,get_version(),2,MAX_FILELAYOUT_VERSION);

  return get_frag_ptr(m_file_layout_ptr->get_fragment_path(trig_num, seq_num, element_id));
}

std::unique_ptr<daqdataformats::Fragment>
HDF5RawDataFile::get_frag_ptr(const daqdataformats::trigger_number_t trig_num,
                              const daqdataformats::sequence_number_t seq_num,
                              const daqdataformats::GeoID::SystemType type,
                              const uint16_t region_id, // NOLINT(build/unsigned)
                              const uint32_t element_id) // NOLINT(build/unsigned)
{
  if(get_version() < 2)
    throw IncompatibleFileLayoutVersion(ERS_HERE,get_version(),2,MAX_FILELAYOUT_VERSION);

  return get_frag_ptr(m_file_layout_ptr->get_fragment_path(trig_num, seq_num, type, region_id, element_id));
}

std::unique_ptr<daqdataformats::Fragment>
HDF5RawDataFile::get_frag_ptr(const daqdataformats::trigger_number_t trig_num,
                              const daqdataformats::sequence_number_t seq_num,
                              const std::string typestring,
                              const uint16_t region_id, // NOLINT(build/unsigned)
                              const uint32_t element_id) // NOLINT(build/unsigned)
{
  if(get_version() < 2)
    throw IncompatibleFileLayoutVersion(ERS_HERE,get_version(),2,MAX_FILELAYOUT_VERSION);

  return get_frag_ptr(m_file_layout_ptr->get_fragment_path(trig_num, seq_num, typestring, region_id, element_id));
}

std::unique_ptr<daqdataformats::TriggerRecordHeader>
HDF5RawDataFile::get_trh_ptr(const std::string& dataset_name)
{
  auto membuffer = get_dataset_raw_data(dataset_name);
  auto trh_ptr = std::make_unique<daqdataformats::TriggerRecordHeader>(membuffer.release(), true);
  return std::move(trh_ptr);
}

std::unique_ptr<daqdataformats::TriggerRecordHeader>
HDF5RawDataFile::get_trh_ptr(const daqdataformats::trigger_number_t trig_num,
                             const daqdataformats::sequence_number_t seq_num)
{
  if(get_version() < 2)
    throw IncompatibleFileLayoutVersion(ERS_HERE,get_version(),2,MAX_FILELAYOUT_VERSION);

  return get_trh_ptr(m_file_layout_ptr->get_trigger_record_header_path(trig_num, seq_num));
}

} // namespace hdf5libs
} // namespace dunedaq
