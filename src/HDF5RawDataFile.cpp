/**
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 *
 */

#include "hdf5libs/HDF5RawDataFile.hpp"
#include "hdf5libs/hdf5filelayout/Nljs.hpp"

#include <algorithm>
#include <filesystem>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace dunedaq {
namespace hdf5libs {

constexpr uint32_t MAX_FILELAYOUT_VERSION = 4294967295; // NOLINT(build/unsigned)

/**
 * @brief Constructor for writing a new file
 */
HDF5RawDataFile::HDF5RawDataFile(std::string file_name,
                                 daqdataformats::run_number_t run_number,
                                 size_t file_index,
                                 std::string application_name,
                                 const hdf5filelayout::FileLayoutParams& fl_params,
                                 std::string inprogress_filename_suffix,
                                 unsigned open_flags)
  : m_bare_file_name(file_name)
  , m_open_flags(open_flags)
{

  // check and make sure that the file isn't ReadOnly
  if (m_open_flags == HighFive::File::ReadOnly) {
    throw IncompatibleOpenFlags(ERS_HERE, file_name, m_open_flags);
  }

  auto filename_to_open = m_bare_file_name + inprogress_filename_suffix;

  // do the file open
  try {
    m_file_ptr.reset(new HighFive::File(filename_to_open, m_open_flags));
  } catch (std::exception const& excpt) {
    throw FileOpenFailed(ERS_HERE, filename_to_open, excpt.what());
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

  // write the record type
  m_record_type = fl_params.record_name_prefix;
  write_attribute("record_type", m_record_type);
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

    // rename file to the bare name
    std::filesystem::rename(m_file_ptr->getName(), m_bare_file_name);
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
 * @brief Write a TimeSlice to the file.
 */
void
HDF5RawDataFile::write(const daqdataformats::TimeSlice& ts)
{
  write(ts.get_header());

  for (auto const& frag_ptr : ts.get_fragments_ref()) {
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
 * @brief Write a TimeSliceHeader to the file.
 */
void
HDF5RawDataFile::write(const daqdataformats::TimeSliceHeader& tsh)
{

  m_recorded_size +=
    do_write(m_file_layout_ptr->get_path_elements(tsh), (const char*)(&tsh), sizeof(daqdataformats::TimeSliceHeader));
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
 * @brief write the file layout
 */
void
HDF5RawDataFile::write_file_layout()
{
  hdf5filelayout::data_t fl_json;
  hdf5filelayout::to_json(fl_json, m_file_layout_ptr->get_file_layout_params());
  write_attribute("filelayout_params", fl_json.dump());
  write_attribute("filelayout_version", m_file_layout_ptr->get_version());
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

  if (m_file_ptr->hasAttribute("record_type"))
    m_record_type = get_attribute<std::string>("record_type");
  else
    m_record_type = m_file_layout_ptr->get_record_name_prefix();

  check_file_layout();
}

void
HDF5RawDataFile::read_file_layout()
{
  hdf5filelayout::FileLayoutParams fl_params;
  uint32_t version = 0; // NOLINT(build/unsigned)

  std::string fl_str;
  try {
    fl_str = get_attribute<std::string>("filelayout_params");
    hdf5filelayout::data_t fl_json = nlohmann::json::parse(fl_str);
    hdf5filelayout::from_json(fl_json, fl_params);

    version = get_attribute<uint32_t>("filelayout_version"); // NOLINT(build/unsigned)

  } catch (InvalidHDF5Attribute const&) {
    ers::info(MissingFileLayout(ERS_HERE, version));
  }

  // now reset the HDF5Filelayout object
  m_file_layout_ptr.reset(new HDF5FileLayout(fl_params, version));
}

void
HDF5RawDataFile::check_file_layout()
{
  if (get_version() < 2)
    return;

  std::string record_type = get_attribute<std::string>("record_type");
  if (record_type.compare(m_file_layout_ptr->get_record_name_prefix()) != 0)
    throw BadRecordType(ERS_HERE, record_type, m_file_layout_ptr->get_record_name_prefix());
}

void
HDF5RawDataFile::check_record_type(std::string rt_name)
{
  if (get_version() < 2)
    return;

  if (m_file_layout_ptr->get_record_name_prefix().compare(rt_name) != 0)
    throw WrongRecordTypeRequested(ERS_HERE, rt_name, m_file_layout_ptr->get_record_name_prefix());
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
  if (!parent_group.isValid())
    throw InvalidHDF5Group(ERS_HERE, top_level_group_name);

  explore_subgroup(parent_group, top_level_group_name, path_list);

  return path_list;
}

/**
 * @brief Return all of the record numbers in the file.
 */
HDF5RawDataFile::record_id_set // NOLINT(build/unsigned)
HDF5RawDataFile::get_all_record_ids()
{
  if (!m_record_ids.empty())
    return m_record_ids;

  // records are at the top level

  HighFive::Group parent_group = m_file_ptr->getGroup(m_file_ptr->getPath());

  std::vector<std::string> childNames = parent_group.listObjectNames();
  const std::string record_prefix = m_file_layout_ptr->get_record_name_prefix();
  const size_t record_prefix_size = record_prefix.size();

  for (auto const& name : childNames) {
    auto loc = name.find(record_prefix);

    if (loc == std::string::npos)
      continue;

    auto rec_num_string = name.substr(loc + record_prefix_size);

    loc = rec_num_string.find(".");
    if (loc == std::string::npos) {
      m_record_ids.insert(std::make_pair(std::stoi(rec_num_string), 0));
    } else {
      auto seq_num_string = rec_num_string.substr(loc + 1);
      rec_num_string.resize(loc); // remove anything from '.' onwards
      m_record_ids.insert(std::make_pair(std::stoi(rec_num_string), std::stoi(seq_num_string)));
    }

  } // end loop over childNames

  return m_record_ids;
}

std::set<uint64_t>
HDF5RawDataFile::get_all_record_numbers() // NOLINT(build/unsigned)
{
  ers::warning(DeprecatedUsage(ERS_HERE,
                               "get_all_record_numbers()",
                               "Use get_all_record_ids(), which returns a record_number,sequence_number pair."));

  std::set<uint64_t> record_numbers; // NOLINT(build/unsigned)
  for (auto const& rid : get_all_record_ids())
    record_numbers.insert(rid.first);

  return record_numbers;
}

HDF5RawDataFile::record_id_set
HDF5RawDataFile::get_all_trigger_record_ids()
{
  check_record_type("TriggerRecord");
  return get_all_record_ids();
}

std::set<daqdataformats::trigger_number_t>
HDF5RawDataFile::get_all_trigger_record_numbers()
{
  ers::warning(
    DeprecatedUsage(ERS_HERE,
                    "get_all_trigger_record_numbers()",
                    "Use get_all_trigger_record_ids(), which returns a record_number,sequence_number pair."));

  return get_all_record_numbers();
}

HDF5RawDataFile::record_id_set
HDF5RawDataFile::get_all_timeslice_ids()
{
  check_record_type("TimeSlice");
  return get_all_record_ids();
}

std::set<daqdataformats::timeslice_number_t>
HDF5RawDataFile::get_all_timeslice_numbers()
{
  check_record_type("TimeSlice");
  return get_all_record_numbers();
}

/**
 * @brief Return a vector of dataset names that correspond to record headers
 */
std::vector<std::string>
HDF5RawDataFile::get_record_header_dataset_paths()
{

  std::vector<std::string> rec_paths;

  if (get_version() >= 2) {

    for (auto const& rec_id : get_all_record_ids())
      rec_paths.push_back(m_file_ptr->getPath() +
                          m_file_layout_ptr->get_record_header_path(rec_id.first, rec_id.second));

  } else {

    for (auto const& path : get_dataset_paths()) {
      if (path.find(m_file_layout_ptr->get_record_header_dataset_name()) != std::string::npos) {
        rec_paths.push_back(path);
      }
    }
  }

  return rec_paths;
}

std::vector<std::string>
HDF5RawDataFile::get_trigger_record_header_dataset_paths()
{
  check_record_type("TriggerRecord");
  return get_record_header_dataset_paths();
}

std::vector<std::string>
HDF5RawDataFile::get_timeslice_header_dataset_paths()
{
  check_record_type("TimeSlice");
  return get_record_header_dataset_paths();
}

std::string
HDF5RawDataFile::get_record_header_dataset_path(const record_id_t rid)
{
  auto rec_id = get_all_record_ids().find(rid);
  if (rec_id == get_all_record_ids().end())
    throw RecordIDNotFound(ERS_HERE, rid.first, rid.second);

  return (m_file_ptr->getPath() + m_file_layout_ptr->get_record_header_path(rid.first, rid.second));
}

std::string
HDF5RawDataFile::get_record_header_dataset_path(const uint64_t rec_num, // NOLINT (build/unsigned)
                                                const daqdataformats::sequence_number_t seq_num)
{
  return get_record_header_dataset_path(std::make_pair(rec_num, seq_num));
}

std::string
HDF5RawDataFile::get_trigger_record_header_dataset_path(const record_id_t rid)
{
  check_record_type("TriggerRecord");
  return get_record_header_dataset_path(rid);
}

std::string
HDF5RawDataFile::get_trigger_record_header_dataset_path(const daqdataformats::trigger_number_t trig_num,
                                                        const daqdataformats::sequence_number_t seq_num)
{
  check_record_type("TriggerRecord");
  return get_record_header_dataset_path(trig_num, seq_num);
}

std::string
HDF5RawDataFile::get_timeslice_header_dataset_path(const record_id_t rid)
{
  check_record_type("TimeSlice");
  return get_record_header_dataset_path(rid.first, 0);
}

std::string
HDF5RawDataFile::get_timeslice_header_dataset_path(const daqdataformats::timeslice_number_t ts_num)
{
  check_record_type("TimeSlice");
  return get_record_header_dataset_path(ts_num);
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
    if (path.find(m_file_layout_ptr->get_record_header_dataset_name()) == std::string::npos)
      frag_paths.push_back(path);
  }

  return frag_paths;
}

// get all fragment dataset paths for given record ID
std::vector<std::string>
HDF5RawDataFile::get_fragment_dataset_paths(const record_id_t rid)
{
  std::vector<std::string> frag_paths;

  auto rec_id = get_all_record_ids().find(rid);
  if (rec_id == get_all_record_ids().end())
    throw RecordIDNotFound(ERS_HERE, rid.first, rid.second);

  std::string record_group_path =
    m_file_ptr->getPath() + m_file_layout_ptr->get_record_number_string(rid.first, rid.second);

  for (auto const& path : get_dataset_paths(record_group_path)) {
    if (path.find(m_file_layout_ptr->get_record_header_dataset_name()) == std::string::npos)
      frag_paths.push_back(path);
  }
  return frag_paths;
}

// get all fragment dataset paths for given record ID
std::vector<std::string>
HDF5RawDataFile::get_fragment_dataset_paths(const uint64_t rec_num, // NOLINT (build/unsigned)
                                            const daqdataformats::sequence_number_t seq_num)
{
  return get_fragment_dataset_paths(std::make_pair(rec_num, seq_num));
}

// get all fragment dataset paths for a SystemType
std::vector<std::string>
HDF5RawDataFile::get_fragment_dataset_paths(const daqdataformats::SourceID::Subsystem type)
{
  std::vector<std::string> frag_paths;
  for (auto const& rid : get_all_record_ids()) {
    auto datasets =
      get_dataset_paths(m_file_ptr->getPath() + m_file_layout_ptr->get_fragment_type_path(rid.first, rid.second, type));
    frag_paths.insert(frag_paths.end(), datasets.begin(), datasets.end());
  }
  return frag_paths;
}

std::vector<std::string>
HDF5RawDataFile::get_fragment_dataset_paths(const std::string typestring)
{
  daqdataformats::SourceID::Subsystem type = daqdataformats::SourceID::string_to_subsystem(typestring);
  return get_fragment_dataset_paths(type);
}

std::vector<std::string>
HDF5RawDataFile::get_fragment_dataset_paths(const record_id_t rid, const daqdataformats::SourceID::Subsystem type)
{
  std::vector<std::string> frag_paths;

  auto rec_id = get_all_record_ids().find(rid);
  if (rec_id == get_all_record_ids().end())
    throw RecordIDNotFound(ERS_HERE, rid.first, rid.second);

  return get_dataset_paths(m_file_ptr->getPath() +
                           m_file_layout_ptr->get_fragment_type_path(rid.first, rid.second, type));
}

std::vector<std::string>
HDF5RawDataFile::get_fragment_dataset_paths(const record_id_t rid, const std::string typestring)
{
  daqdataformats::SourceID::Subsystem type = daqdataformats::SourceID::string_to_subsystem(typestring);
  return get_fragment_dataset_paths(rid, type);
}

// get all fragment dataset paths for a SourceID
std::vector<std::string>
HDF5RawDataFile::get_fragment_dataset_paths(const daqdataformats::SourceID element_id)
{
  std::vector<std::string> frag_paths;

  for (auto const& rid : get_all_record_ids())
    frag_paths.push_back(m_file_ptr->getPath() +
                         m_file_layout_ptr->get_fragment_path(rid.first, rid.second, element_id));

  return frag_paths;
}

std::vector<std::string>
HDF5RawDataFile::get_fragment_dataset_paths(const daqdataformats::SourceID::Subsystem type,
                                            const uint32_t element_id) // NOLINT(build/unsigned)
{
  return get_fragment_dataset_paths(daqdataformats::SourceID(type, element_id));
}
std::vector<std::string>
HDF5RawDataFile::get_fragment_dataset_paths(const std::string typestring,
                                            const uint32_t element_id) // NOLINT(build/unsigned)
{
  return get_fragment_dataset_paths(
    daqdataformats::SourceID(daqdataformats::SourceID::string_to_subsystem(typestring), element_id));
}

std::set<daqdataformats::SourceID>
HDF5RawDataFile::get_source_ids(std::vector<std::string> const& frag_dataset_paths)
{
  std::set<daqdataformats::SourceID> source_ids;
  std::vector<std::string> path_elements;
  std::string s;
  for (auto const& frag_dataset : frag_dataset_paths) {
    path_elements.clear();
    std::istringstream iss(frag_dataset);
    while (std::getline(iss, s, '/')) {
      if (s.size() > 0)
        path_elements.push_back(s);
    }
    source_ids.insert(m_file_layout_ptr->get_source_id_from_path_elements(path_elements));
  }

  return source_ids;
}

std::unique_ptr<char[]>
HDF5RawDataFile::get_dataset_raw_data(const std::string& dataset_path)
{

  HighFive::Group parent_group = m_file_ptr->getGroup("/");
  HighFive::DataSet data_set = parent_group.getDataSet(dataset_path);

  if (!data_set.isValid())
    throw InvalidHDF5Dataset(ERS_HERE, dataset_path, get_file_name());

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
HDF5RawDataFile::get_frag_ptr(const record_id_t rid, const daqdataformats::SourceID element_id)
{
  if (get_version() < 2)
    throw IncompatibleFileLayoutVersion(ERS_HERE, get_version(), 2, MAX_FILELAYOUT_VERSION);

  return get_frag_ptr(m_file_layout_ptr->get_fragment_path(rid.first, rid.second, element_id));
}

std::unique_ptr<daqdataformats::Fragment>
HDF5RawDataFile::get_frag_ptr(const uint64_t rec_num, // NOLINT(build/unsigned)
                              const daqdataformats::sequence_number_t seq_num,
                              const daqdataformats::SourceID element_id)
{
  if (get_version() < 2)
    throw IncompatibleFileLayoutVersion(ERS_HERE, get_version(), 2, MAX_FILELAYOUT_VERSION);

  return get_frag_ptr(m_file_layout_ptr->get_fragment_path(rec_num, seq_num, element_id));
}

std::unique_ptr<daqdataformats::Fragment>
HDF5RawDataFile::get_frag_ptr(const record_id_t rid,
                              const daqdataformats::SourceID::Subsystem type,
                              const uint32_t element_id) // NOLINT(build/unsigned)
{
  if (get_version() < 2)
    throw IncompatibleFileLayoutVersion(ERS_HERE, get_version(), 2, MAX_FILELAYOUT_VERSION);

  return get_frag_ptr(m_file_layout_ptr->get_fragment_path(rid.first, rid.second, type, element_id));
}

std::unique_ptr<daqdataformats::Fragment>
HDF5RawDataFile::get_frag_ptr(const uint64_t rec_num, // NOLINT(build/unsigned)
                              const daqdataformats::sequence_number_t seq_num,
                              const daqdataformats::SourceID::Subsystem type,
                              const uint32_t element_id) // NOLINT(build/unsigned)
{
  if (get_version() < 2)
    throw IncompatibleFileLayoutVersion(ERS_HERE, get_version(), 2, MAX_FILELAYOUT_VERSION);

  return get_frag_ptr(m_file_layout_ptr->get_fragment_path(rec_num, seq_num, type, element_id));
}

std::unique_ptr<daqdataformats::Fragment>
HDF5RawDataFile::get_frag_ptr(const record_id_t rid,
                              const std::string typestring,
                              const uint32_t element_id) // NOLINT(build/unsigned)
{
  if (get_version() < 2)
    throw IncompatibleFileLayoutVersion(ERS_HERE, get_version(), 2, MAX_FILELAYOUT_VERSION);

  return get_frag_ptr(m_file_layout_ptr->get_fragment_path(rid.first, rid.second, typestring, element_id));
}

std::unique_ptr<daqdataformats::Fragment>
HDF5RawDataFile::get_frag_ptr(const uint64_t rec_num, // NOLINT(build/unsigned)
                              const daqdataformats::sequence_number_t seq_num,
                              const std::string typestring,
                              const uint32_t element_id) // NOLINT(build/unsigned)
{
  if (get_version() < 2)
    throw IncompatibleFileLayoutVersion(ERS_HERE, get_version(), 2, MAX_FILELAYOUT_VERSION);

  return get_frag_ptr(m_file_layout_ptr->get_fragment_path(rec_num, seq_num, typestring, element_id));
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
  if (get_version() < 2)
    throw IncompatibleFileLayoutVersion(ERS_HERE, get_version(), 2, MAX_FILELAYOUT_VERSION);

  return get_trh_ptr(m_file_layout_ptr->get_trigger_record_header_path(trig_num, seq_num));
}

std::unique_ptr<daqdataformats::TimeSliceHeader>
HDF5RawDataFile::get_tsh_ptr(const std::string& dataset_name)
{
  auto membuffer = get_dataset_raw_data(dataset_name);
  auto tsh_ptr = std::make_unique<daqdataformats::TimeSliceHeader>(
    *(reinterpret_cast<daqdataformats::TimeSliceHeader*>(membuffer.release()))); // NOLINT
  return std::move(tsh_ptr);
}

std::unique_ptr<daqdataformats::TimeSliceHeader>
HDF5RawDataFile::get_tsh_ptr(const daqdataformats::timeslice_number_t ts_num)
{
  if (get_version() < 2)
    throw IncompatibleFileLayoutVersion(ERS_HERE, get_version(), 2, MAX_FILELAYOUT_VERSION);

  return get_tsh_ptr(m_file_layout_ptr->get_trigger_record_header_path(ts_num));
}

daqdataformats::TriggerRecord
HDF5RawDataFile::get_trigger_record(const daqdataformats::trigger_number_t trig_num,
                                    const daqdataformats::sequence_number_t seq_num)
{
  daqdataformats::TriggerRecord trigger_record(*get_trh_ptr(trig_num, seq_num));
  for (auto const& frag_path : get_fragment_dataset_paths(trig_num, seq_num)) {
    trigger_record.add_fragment(get_frag_ptr(frag_path));
  }

  return trigger_record;
}

daqdataformats::TimeSlice
HDF5RawDataFile::get_timeslice(const daqdataformats::timeslice_number_t ts_num)
{
  daqdataformats::TimeSlice timeslice(*get_tsh_ptr(ts_num));
  for (auto const& frag_path : get_fragment_dataset_paths(ts_num)) {
    timeslice.add_fragment(get_frag_ptr(frag_path));
  }

  return timeslice;
}

} // namespace hdf5libs
} // namespace dunedaq
