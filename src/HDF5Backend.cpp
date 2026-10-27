/**
 * @file HDF5Backend.cpp
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "hdf5libs/HDF5Backend.hpp"
#include "hdf5libs/HDF5SourceIDHandler.hpp"

#include "hdf5libs/Issues.hpp"
#include "logging/Logging.hpp" // before Issues.hpp, so that TLOG_DEBUG << issue works

#include <highfive/H5DataSet.hpp>
#include <highfive/H5Group.hpp>
#include <nlohmann/json.hpp>

#include <sys/stat.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <exception>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace dunedaq::hdf5libs {

namespace {

template<typename T>
T
read_attribute_or(const HighFive::File& file, const std::string& name, const T& default_value)
{
  if (!file.hasAttribute(name)) {
    return default_value;
  }
  T value;
  file.getAttribute(name).read(value);
  return value;
}

// Timestamps are integers from layout version 6 on, and decimal strings before that
uint64_t // NOLINT(build/unsigned)
read_timestamp_attribute(const HighFive::File& file, const std::string& name)
{
  if (!file.hasAttribute(name)) {
    return 0;
  }
  auto attribute = file.getAttribute(name);
  if (attribute.getDataType().getClass() == HighFive::DataTypeClass::String) {
    std::string value;
    attribute.read(value);
    try {
      return std::stoull(value);
    } catch (std::exception const&) {
      return 0;
    }
  }
  size_t value = 0;
  attribute.read(value);
  return value;
}

template<typename T>
void
write_attribute_if_absent(HighFive::File& file, const std::string& name, const T& value)
{
  if (!file.hasAttribute(name)) {
    file.createAttribute(name, value);
  }
}

std::vector<std::string>
split_path(const std::string& path)
{
  std::vector<std::string> elements;
  std::istringstream iss(path);
  std::string element;
  while (std::getline(iss, element, '/')) {
    if (!element.empty()) {
      elements.push_back(element);
    }
  }
  return elements;
}

void
add_dataset_paths(const HighFive::Group& group, std::string group_path, std::vector<std::string>& paths)
{
  if (!group_path.empty() && group_path.back() == '/') {
    group_path.pop_back();
  }
  for (auto const& child_name : group.listObjectNames()) {
    std::string child_path = group_path + "/" + child_name;
    auto child_type = group.getObjectType(child_name);
    if (child_type == HighFive::ObjectType::Dataset) {
      paths.push_back(child_path);
    } else if (child_type == HighFive::ObjectType::Group) {
      add_dataset_paths(group.getGroup(child_name), child_path, paths);
    }
  }
}
} // namespace

std::vector<std::string>
collect_dataset_paths(const HighFive::Group& group, const std::string& group_path)
{
  std::vector<std::string> paths;
  add_dataset_paths(group, group_path, paths);
  return paths;
}

HDF5FileLayoutParameters
default_file_layout_parameters(rawdatafilelibs::RecordType record_type)
{
  HDF5FileLayoutParameters params;
  if (record_type == rawdatafilelibs::RecordType::kTimeSlice) {
    params.record_name_prefix = "TimeSlice";
    params.digits_for_sequence_number = 0;
    params.record_header_dataset_name = "TimeSliceHeader";
  }
  return params;
}

// ---------------------------------------------------------------------------
// Reading

HDF5ReaderBackend::HDF5ReaderBackend(bool allow_writing)
  : m_allow_writing(allow_writing)
{
}

void
HDF5ReaderBackend::open(const std::string& path)
{
  m_file =
    std::make_unique<HighFive::File>(path, m_allow_writing ? HighFive::File::ReadWrite : HighFive::File::ReadOnly);

  // Files written before the layout was stored get the version 0 layout
  HDF5FileLayoutParameters params;
  uint32_t version = 0; // NOLINT(build/unsigned)
  if (m_file->hasAttribute("filelayout_params") && m_file->hasAttribute("filelayout_version")) {
    std::string params_string;
    m_file->getAttribute("filelayout_params").read(params_string);
    params = HDF5FileLayoutParameters(nlohmann::json::parse(params_string));
    m_file->getAttribute("filelayout_version").read(version);
  } else {
    ers::info(MissingFileLayout(ERS_HERE, version));
  }
  m_file_layout = std::make_unique<HDF5FileLayout>(params, version);

  m_record_type_name = read_attribute_or<std::string>(*m_file, "record_type", m_file_layout->get_record_name_prefix());
  if (version >= 2 && m_record_type_name != m_file_layout->get_record_name_prefix()) {
    throw BadRecordType(ERS_HERE, m_record_type_name, m_file_layout->get_record_name_prefix());
  }
}

void
HDF5ReaderBackend::check_open() const
{
  if (m_file == nullptr) {
    throw std::logic_error("the HDF5 file is not open");
  }
}

HighFive::File&
HDF5ReaderBackend::get_file()
{
  check_open();
  return *m_file;
}

const HDF5FileLayout&
HDF5ReaderBackend::get_file_layout() const
{
  check_open();
  return *m_file_layout;
}

rawdatafilelibs::FileMetadata
HDF5ReaderBackend::read_metadata()
{
  check_open();
  rawdatafilelibs::FileMetadata metadata;
  metadata.run_number = read_attribute_or<daqdataformats::run_number_t>(*m_file, "run_number", 0);
  metadata.file_index = read_attribute_or<size_t>(*m_file, "file_index", 0);
  metadata.application_name = read_attribute_or<std::string>(*m_file, "application_name", "");
  metadata.record_type = rawdatafilelibs::string_to_record_type(m_record_type_name);
  metadata.creation_timestamp_ms = read_timestamp_attribute(*m_file, "creation_timestamp");
  metadata.closing_timestamp_ms = read_timestamp_attribute(*m_file, "closing_timestamp");
  metadata.recovery_timestamp_ms = read_timestamp_attribute(*m_file, "file_recovery_timestamp");
  metadata.operational_environment = read_attribute_or<std::string>(*m_file, "operational_environment", "");
  metadata.offline_data_stream = read_attribute_or<std::string>(*m_file, "offline_data_stream", "");
  metadata.run_is_for_test_purposes =
    (read_attribute_or<std::string>(*m_file, "run_was_for_test_purposes", "false") == "true");
  metadata.compression_level = read_attribute_or<unsigned>(*m_file, "compression_level", 0); // NOLINT(build/unsigned)
  metadata.layout_version = m_file_layout->get_version();
  metadata.stats.recorded_size = read_attribute_or<size_t>(*m_file, "recorded_size", 0);
  metadata.stats.uncompressed_raw_data_size = read_attribute_or<size_t>(*m_file, "uncompressed_raw_data_size", 0);
  metadata.stats.total_file_size = read_attribute_or<size_t>(*m_file, "total_file_size", 0);
  return metadata;
}

daqdataformats::SourceIDGeoIDMap
HDF5ReaderBackend::read_geo_id_map()
{
  check_open();
  HDF5SourceIDHandler::source_id_geo_id_map_t encoded_map;
  HDF5SourceIDHandler(m_file_layout->get_version()).fetch_file_level_geo_id_info(*m_file, encoded_map);
  return daqdataformats::SourceIDGeoIDMap::from_encoded(encoded_map);
}

std::set<daqdataformats::RecordID>
HDF5ReaderBackend::read_record_ids()
{
  check_open();
  // records are the top-level groups named <prefix><number>[.<sequence>]
  std::set<RecordID> record_ids;
  const std::string record_prefix = m_file_layout->get_record_name_prefix();
  auto all_digits = [](const std::string& text) {
    return !text.empty() && std::all_of(text.begin(), text.end(), [](unsigned char c) { return std::isdigit(c); });
  };
  for (auto const& name : m_file->listObjectNames()) {
    // the whole name must be the prefix followed by digits, optionally "." and more digits
    if (name.rfind(record_prefix, 0) != 0) {
      continue;
    }
    std::string number_string = name.substr(record_prefix.size());
    std::string sequence_string = "0";
    auto dot = number_string.find('.');
    if (dot != std::string::npos) {
      sequence_string = number_string.substr(dot + 1);
      number_string.resize(dot);
    }
    if (!all_digits(number_string) || !all_digits(sequence_string)) {
      TLOG_DEBUG(7) << "Ignoring top-level group " << name << ", which isn't a record";
      continue;
    }
    try {
      record_ids.insert(RecordID{ std::stoull(number_string),
                                  static_cast<daqdataformats::sequence_number_t>(std::stoul(sequence_string)) });
    } catch (std::exception const&) {
      TLOG_DEBUG(7) << "Ignoring top-level group " << name << ", which isn't a record";
    }
  }
  return record_ids;
}

const HDF5ReaderBackend::RecordContents&
HDF5ReaderBackend::load_record(const RecordID& record_id)
{
  check_open();
  auto iter = m_records.find(record_id);
  if (iter != m_records.end()) {
    return iter->second;
  }

  const std::string group_name = m_file_layout->get_record_number_string(record_id.number, record_id.sequence);
  if (!m_file->exist(group_name)) {
    throw InvalidHDF5Group(ERS_HERE, group_name);
  }
  HighFive::Group record_group = m_file->getGroup(group_name);

  if (is_legacy_layout()) {
    return m_records.emplace(record_id, scan_legacy_record(record_id, record_group)).first->second;
  }

  // the record's stored SourceID maps are the source of truth for what it contains
  HDF5SourceIDHandler handler(m_file_layout->get_version());
  RecordContents contents;
  handler.fetch_source_id_path_info(record_group, contents.dataset_paths);
  HDF5SourceIDHandler::fragment_type_source_id_map_t fragment_type_map;
  handler.fetch_fragment_type_source_id_info(record_group, fragment_type_map);
  HDF5SourceIDHandler::subdetector_source_id_map_t subdetector_map;
  handler.fetch_subdetector_source_id_info(record_group, subdetector_map);
  const SourceID header_source_id = handler.fetch_record_header_source_id(record_group);

  std::map<SourceID, daqdataformats::FragmentInfo> fragment_info;
  for (auto const& [fragment_type, source_ids] : fragment_type_map) {
    for (auto const& source_id : source_ids) {
      fragment_info[source_id].fragment_type = fragment_type;
    }
  }
  for (auto const& [subdetector, source_ids] : subdetector_map) {
    for (auto const& source_id : source_ids) {
      fragment_info[source_id].subdetector = subdetector;
    }
  }

  contents.index = daqdataformats::RecordIndex(record_id, header_source_id);
  for (auto const& [source_id, path] : contents.dataset_paths) {
    if (source_id != header_source_id) {
      contents.index.add_fragment(source_id, fragment_info[source_id]);
    }
  }
  return m_records.emplace(record_id, std::move(contents)).first->second;
}

HDF5ReaderBackend::RecordContents
HDF5ReaderBackend::scan_legacy_record(const RecordID& record_id, const HighFive::Group& record_group)
{
  // Layout versions <= 2 store no SourceID maps: the header is the dataset named after
  // record_header_dataset_name, and each fragment's SourceID comes from its path
  // (<record>/<detector group>/<region>/<element>), as configured by the file layout.
  const std::string group_path = "/" + m_file_layout->get_record_number_string(record_id.number, record_id.sequence);
  auto paths = collect_dataset_paths(record_group, group_path);

  RecordContents contents;
  const SourceID header_source_id; // unknown in these files
  contents.index = daqdataformats::RecordIndex(record_id, header_source_id);
  for (auto const& path : paths) {
    if (path.find(m_file_layout->get_record_header_dataset_name()) != std::string::npos) {
      contents.dataset_paths[header_source_id] = path;
      continue;
    }
    try {
      SourceID source_id = m_file_layout->get_source_id_from_path_elements(split_path(path));
      contents.dataset_paths[source_id] = path;
      contents.index.add_fragment(source_id, daqdataformats::FragmentInfo{});
    } catch (std::exception const&) {
      TLOG_DEBUG(7) << "Ignoring dataset " << path << ", whose SourceID can't be worked out from its path";
    }
  }
  return contents;
}

daqdataformats::RecordIndex
HDF5ReaderBackend::read_index(const RecordID& record_id)
{
  return load_record(record_id).index;
}

const std::string&
HDF5ReaderBackend::get_dataset_path(const RecordID& record_id, const SourceID& source_id)
{
  auto const& paths = load_record(record_id).dataset_paths;
  auto iter = paths.find(source_id);
  if (iter == paths.end()) {
    throw SourceIDNotFound(ERS_HERE, source_id.to_string(), record_id.number, record_id.sequence);
  }
  return iter->second;
}

std::vector<std::string>
HDF5ReaderBackend::get_dataset_paths(const std::string& group_path)
{
  check_open();
  const std::string top = group_path.empty() ? m_file->getPath() : group_path;
  if (top != "/" && !m_file->exist(top)) {
    throw InvalidHDF5Group(ERS_HERE, top);
  }
  return collect_dataset_paths(m_file->getGroup(top), top);
}

rawdatafilelibs::Buffer
HDF5ReaderBackend::read_dataset(const std::string& dataset_path)
{
  check_open();
  HighFive::DataSet data_set = m_file->getGroup("/").getDataSet(dataset_path);
  if (!data_set.isValid()) {
    throw InvalidHDF5Dataset(ERS_HERE, dataset_path, m_file->getName());
  }
  auto buffer = rawdatafilelibs::allocate_buffer(data_set.getSpace().getElementCount() * sizeof(char));
  data_set.read(buffer.data.get());
  return buffer;
}

rawdatafilelibs::Buffer
HDF5ReaderBackend::read_blob(const RecordID& record_id, const SourceID& source_id)
{
  return read_dataset(get_dataset_path(record_id, source_id));
}

void
HDF5ReaderBackend::close()
{
  m_records.clear();
  if (m_file == nullptr) {
    return;
  }
  auto file = std::move(m_file); // released even if flushing throws
  if (m_allow_writing) {
    file->flush();
  }
}

// ---------------------------------------------------------------------------
// Writing

HDF5WriterBackend::HDF5WriterBackend(HDF5WriterOptions options)
  : m_options(std::move(options))
{
}

HighFive::File&
HDF5WriterBackend::get_file()
{
  if (m_file == nullptr) {
    throw std::logic_error("the HDF5 file is not open");
  }
  return *m_file;
}

const HDF5FileLayout&
HDF5WriterBackend::get_file_layout() const
{
  if (m_file_layout == nullptr) {
    throw std::logic_error("the HDF5 file has not been opened");
  }
  return *m_file_layout;
}

void
HDF5WriterBackend::open(const std::string& path,
                        const rawdatafilelibs::FileMetadata& metadata,
                        const daqdataformats::SourceIDGeoIDMap& geo_id_map)
{
  auto params = m_options.file_layout_parameters.value_or(default_file_layout_parameters(metadata.record_type));
  const std::string record_type_name = rawdatafilelibs::record_type_to_string(metadata.record_type);
  if (params.record_name_prefix != record_type_name) {
    throw LayoutDoesNotMatchRecordType(ERS_HERE, params.record_name_prefix, record_type_name, path);
  }
  m_file_layout = std::make_unique<HDF5FileLayout>(params);
  m_compression_level = metadata.compression_level;

  // never reuse an existing file: Excl fails, leaving it untouched, if path exists
  m_file = std::make_unique<HighFive::File>(path, HighFive::File::Excl);

  TLOG_DEBUG(2) << "Created HDF5 file " << path << " at time " << metadata.creation_timestamp_ms;

  // the same attributes, with the same types and in the same order, as HDF5RawDataFile has always written
  write_attribute_if_absent(*m_file, "run_number", metadata.run_number);
  write_attribute_if_absent(*m_file, "file_index", metadata.file_index);
  write_attribute_if_absent(*m_file, "creation_timestamp", static_cast<size_t>(metadata.creation_timestamp_ms));
  write_attribute_if_absent(*m_file, "application_name", metadata.application_name);
  // the layout may have corrected the parameters, so store its version of them
  write_attribute_if_absent(*m_file, "filelayout_params", m_file_layout->get_file_layout_params().to_json().dump());
  write_attribute_if_absent(*m_file, "filelayout_version", m_file_layout->get_version());
  HDF5SourceIDHandler::store_file_level_geo_id_info(*m_file, geo_id_map.to_encoded());
  write_attribute_if_absent(*m_file, "record_type", params.record_name_prefix);
  write_attribute_if_absent(*m_file, "compression_level", m_compression_level);

  if (m_options.write_run_context_attributes) {
    write_attribute_if_absent(*m_file, "operational_environment", metadata.operational_environment);
    write_attribute_if_absent(*m_file, "offline_data_stream", metadata.offline_data_stream);
    write_attribute_if_absent(
      *m_file, "run_was_for_test_purposes", std::string(metadata.run_is_for_test_purposes ? "true" : "false"));
  }
}

bool
HDF5WriterBackend::record_exists(const daqdataformats::RecordID& record_id)
{
  return get_file().exist(get_file_layout().get_record_number_string(record_id.number, record_id.sequence));
}

std::pair<std::string, HighFive::Group>
HDF5WriterBackend::write_dataset(const std::vector<std::string>& path_elements, const void* data, size_t size)
{
  auto& file = get_file();

  std::string const& top_level_group_name = path_elements.at(0);
  if (!file.exist(top_level_group_name)) {
    file.createGroup(top_level_group_name);
  }
  HighFive::Group top_level_group = file.getGroup(top_level_group_name);
  if (!top_level_group.isValid()) {
    throw InvalidHDF5Group(ERS_HERE, top_level_group_name);
  }

  // the elements between the first and the last are subgroups; the last is the dataset
  HighFive::Group sub_group = top_level_group;
  for (size_t idx = 1; idx + 1 < path_elements.size(); ++idx) {
    std::string const& child_group_name = path_elements[idx];
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

  HighFive::DataSpace data_space({ size, 1 });
  HighFive::DataSetCreateProps create_props;
  HighFive::DataSetAccessProps access_props;
  if (m_compression_level > 0) {
    create_props.add(HighFive::Chunking(std::vector<hsize_t>{ size, 1 }));
    create_props.add(HighFive::Deflate(m_compression_level));
  }

  m_stats.uncompressed_raw_data_size += size;

  auto data_set = sub_group.createDataSet<char>(path_elements.back(), data_space, create_props, access_props);
  if (!data_set.isValid()) {
    throw InvalidHDF5Dataset(ERS_HERE, path_elements.back(), file.getName());
  }
  data_set.write_raw(static_cast<const char*>(data));
  m_stats.total_file_size = file.getFileSize();
  file.flush();
  m_stats.recorded_size += data_set.getStorageSize();
  return { data_set.getPath(), top_level_group };
}

void
HDF5WriterBackend::write_record(const daqdataformats::RecordIndex& index,
                                const rawdatafilelibs::BlobView& header,
                                const std::vector<rawdatafilelibs::BlobView>& fragments)
{
  auto const& layout = get_file_layout();
  auto const& record_id = index.get_record_id();

  HDF5SourceIDHandler::source_id_path_map_t path_map;
  auto [header_path, record_group] =
    write_dataset(layout.get_record_header_path_elements(record_id, header.source_id), header.data, header.size);
  path_map[header.source_id] = header_path;
  HDF5SourceIDHandler::store_record_header_source_id(record_group, header.source_id);

  for (auto const& fragment : fragments) {
    auto info = index.get_fragment_info(fragment.source_id).value_or(daqdataformats::FragmentInfo{});
    auto [path, group] =
      write_dataset(layout.get_fragment_path_elements(record_id, fragment.source_id, info.fragment_type),
                    fragment.data,
                    fragment.size);
    path_map[fragment.source_id] = path;
  }

  // the record-level maps that readers use as the record's index
  HDF5SourceIDHandler::store_record_level_path_info(record_group, path_map);
  HDF5SourceIDHandler::store_record_level_fragment_type_map(record_group, index.get_source_ids_by_fragment_type());
  HDF5SourceIDHandler::store_record_level_subdetector_map(record_group, index.get_source_ids_by_subdetector());
}

void
HDF5WriterBackend::close(const rawdatafilelibs::FileMetadata& final_metadata)
{
  if (m_file == nullptr) {
    return;
  }
  auto file = std::move(m_file); // released even if writing the closing attributes throws
  write_attribute_if_absent(*file, "recorded_size", final_metadata.stats.recorded_size);
  write_attribute_if_absent(*file, "uncompressed_raw_data_size", final_metadata.stats.uncompressed_raw_data_size);
  write_attribute_if_absent(*file, "total_file_size", final_metadata.stats.total_file_size);
  write_attribute_if_absent(*file, "closing_timestamp", static_cast<size_t>(final_metadata.closing_timestamp_ms));
  file->flush();
}

rawdatafilelibs::RecoveryScan
HDF5WriterBackend::scan_for_recovery(const std::string& path)
{
  HDF5ReaderBackend reader;
  reader.open(path);

  rawdatafilelibs::RecoveryScan scan;
  scan.stored = reader.read_metadata();
  auto& file = reader.get_file();
  scan.has_closing_timestamp = file.hasAttribute("closing_timestamp");
  scan.has_stats = file.hasAttribute("recorded_size") && file.hasAttribute("uncompressed_raw_data_size") &&
                   file.hasAttribute("total_file_size");
  scan.has_recovery_timestamp = file.hasAttribute("file_recovery_timestamp");

  // count every dataset (record header or fragment) that can still be read. If the records can't
  // even be listed, the scan fails (and so does recovery), rather than report a file with no records.
  for (auto const& record_id : reader.read_record_ids()) {
    bool complete = true;
    auto count = [&](const daqdataformats::SourceID& source_id) {
      try {
        auto data_set = file.getDataSet(reader.get_dataset_path(record_id, source_id));
        scan.calculated_stats.recorded_size += data_set.getStorageSize();
        scan.calculated_stats.uncompressed_raw_data_size += data_set.getElementCount();
      } catch (std::exception const&) {
        complete = false;
      }
    };
    try {
      auto index = reader.read_index(record_id);
      count(index.get_header_source_id());
      for (auto const& source_id : index.get_fragment_source_ids()) {
        count(source_id);
      }
    } catch (std::exception const&) {
      complete = false;
    }
    ++(complete ? scan.readable_records : scan.unreadable_records);
  }
  scan.calculated_stats.total_file_size = file.getFileSize();
  reader.close();

  struct stat stat_results;
  if (stat(path.c_str(), &stat_results) == 0) {
    scan.last_modified_ms = static_cast<uint64_t>(stat_results.st_mtime) * 1000; // NOLINT(build/unsigned)
  }
  return scan;
}

void
HDF5WriterBackend::store_recovery_metadata(const std::string& path,
                                           const rawdatafilelibs::RecoveryScan& scan,
                                           const rawdatafilelibs::FileMetadata& final_metadata)
{
  HDF5ReaderBackend reader(true); // read-write
  reader.open(path);
  auto& file = reader.get_file();
  if (!scan.has_stats) {
    write_attribute_if_absent(file, "recorded_size", final_metadata.stats.recorded_size);
    write_attribute_if_absent(file, "uncompressed_raw_data_size", final_metadata.stats.uncompressed_raw_data_size);
    write_attribute_if_absent(file, "total_file_size", final_metadata.stats.total_file_size);
  }
  if (!scan.has_closing_timestamp) {
    if (reader.get_file_layout_version() >= 6) {
      write_attribute_if_absent(file, "closing_timestamp", static_cast<size_t>(final_metadata.closing_timestamp_ms));
    } else {
      write_attribute_if_absent(file, "closing_timestamp", std::to_string(final_metadata.closing_timestamp_ms));
    }
  }
  if (!scan.has_recovery_timestamp) {
    write_attribute_if_absent(
      file, "file_recovery_timestamp", static_cast<int64_t>(final_metadata.recovery_timestamp_ms)); // as before
  }
  file.flush();
  reader.close();
}

} // namespace dunedaq::hdf5libs
