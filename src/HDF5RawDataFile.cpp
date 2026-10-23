/**
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 *
 * HDF5RawDataFile is a deprecated facade over rawdatafilelibs::RawDataFileReader/Writer and the
 * HDF5 backend. It keeps its earlier behaviour and errors; see the header for the differences.
 */

#include "hdf5libs/HDF5RawDataFile.hpp"

#include "rawdatafilelibs/FileUtils.hpp"
#include "rawdatafilelibs/Issues.hpp"

#include "logging/Logging.hpp"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace dunedaq {
namespace hdf5libs {

void
rename_without_replacing(const std::string& from, const std::string& to)
{
  rawdatafilelibs::rename_without_replacing(from, to);
}

namespace {

daqdataformats::RecordID
to_record_id(const HDF5RawDataFile::record_id_t& rid)
{
  return daqdataformats::RecordID{ rid.first, rid.second };
}

std::vector<uint64_t> // NOLINT(build/unsigned)
encoded_geo_ids(const daqdataformats::SourceIDGeoIDMap::geo_id_list_t& geo_ids)
{
  std::vector<uint64_t> encoded; // NOLINT(build/unsigned)
  for (auto const& geo_id : geo_ids) {
    encoded.push_back(geo_id.encode());
  }
  return encoded;
}

template<typename GeoIDSet>
std::set<uint64_t> // NOLINT(build/unsigned)
encoded_geo_id_set(const GeoIDSet& geo_ids)
{
  std::set<uint64_t> encoded; // NOLINT(build/unsigned)
  for (auto const& geo_id : geo_ids) {
    encoded.insert(geo_id.encode());
  }
  return encoded;
}

} // namespace

/**
 * @brief Constructor for writing a new file
 */
HDF5RawDataFile::HDF5RawDataFile(const std::string file_name,
                                 const daqdataformats::run_number_t run_number,
                                 const size_t file_index,
                                 const std::string application_name,
                                 const HDF5FileLayoutParameters fl_params,
                                 const HDF5SourceIDHandler::source_id_geo_id_map_t srcid_geoid_map,
                                 const unsigned compression_level,
                                 const std::string inprogress_filename_suffix,
                                 const unsigned open_flags)
  : m_bare_file_name(file_name)
{
  // check and make sure that the file isn't ReadOnly
  if (open_flags == HighFive::File::ReadOnly) {
    throw IncompatibleOpenFlags(ERS_HERE, file_name, open_flags);
  }
  if (open_flags != HighFive::File::Excl) {
    ers::warning(DeprecatedUsage(ERS_HERE,
                                 "HDF5RawDataFile open_flags",
                                 "Files are now always created exclusively (HighFive::File::Excl), and an existing "
                                 "file is never reused or overwritten; the given open_flags are ignored."));
  }

  rawdatafilelibs::FileMetadata metadata;
  metadata.run_number = run_number;
  metadata.file_index = file_index;
  metadata.application_name = application_name;
  metadata.record_type = rawdatafilelibs::string_to_record_type(fl_params.record_name_prefix);
  metadata.compression_level = compression_level;
  m_geo_id_map = daqdataformats::SourceIDGeoIDMap::from_encoded(srcid_geoid_map);

  // this constructor's callers write the run-context attributes themselves, if at all
  open_for_writing(file_name, metadata, fl_params, inprogress_filename_suffix, false);
}

HDF5RawDataFile::HDF5RawDataFile(const std::string& file_name,
                                 const rawdatafilelibs::FileMetadata& metadata,
                                 const HDF5FileLayoutParameters& fl_params,
                                 const daqdataformats::SourceIDGeoIDMap& geo_id_map,
                                 const std::string& inprogress_filename_suffix)
  : m_bare_file_name(file_name)
  , m_geo_id_map(geo_id_map)
{
  open_for_writing(file_name, metadata, fl_params, inprogress_filename_suffix, true);
}

void
HDF5RawDataFile::open_for_writing(const std::string& file_name,
                                  rawdatafilelibs::FileMetadata metadata,
                                  const HDF5FileLayoutParameters& fl_params,
                                  const std::string& inprogress_filename_suffix,
                                  bool write_run_context_attributes)
{
  if (metadata.record_type == rawdatafilelibs::RecordType::kUnknown) {
    metadata.record_type = rawdatafilelibs::string_to_record_type(fl_params.record_name_prefix);
  }
  if (metadata.record_type == rawdatafilelibs::RecordType::kUnknown) {
    // the layout check reports an unknown record name (InvalidRecordName), as before
    HDF5FileLayout layout_check(fl_params);
  }

  HDF5WriterOptions options;
  options.file_layout_parameters = fl_params;
  options.write_run_context_attributes = write_run_context_attributes;
  auto backend = std::make_unique<HDF5WriterBackend>(options);
  auto* backend_ptr = backend.get();

  try {
    m_writer = std::make_unique<rawdatafilelibs::RawDataFileWriter>(
      std::move(backend), file_name, metadata, m_geo_id_map, inprogress_filename_suffix);
  } catch (rawdatafilelibs::FileOpenFailed const& excpt) {
    throw FileOpenFailed(ERS_HERE, file_name + inprogress_filename_suffix, excpt.message(), excpt);
  }
  m_writer_backend = backend_ptr;

  m_file_layout_ptr = std::make_unique<HDF5FileLayout>(m_writer_backend->get_file_layout());
  m_compression_level = metadata.compression_level;
  m_record_type = fl_params.record_name_prefix;

  TLOG_DEBUG(TLVL_BASIC) << "Created HDF5 file (" << file_name << ") at time "
                         << m_writer->get_metadata().creation_timestamp_ms << " .";
}

/**
 * @brief Constructor for reading (and optionally writing) a file
 */
HDF5RawDataFile::HDF5RawDataFile(const std::string& file_name, bool allow_writing)
  : m_allow_writing(allow_writing)
{
  m_bare_file_name = file_name;
  size_t pos = m_bare_file_name.rfind(s_inprogress_suffix);
  if (pos != std::string::npos) {
    m_bare_file_name.erase(pos);
  }

  auto backend = std::make_unique<HDF5ReaderBackend>(allow_writing);
  auto* backend_ptr = backend.get();
  try {
    m_reader = std::make_unique<rawdatafilelibs::RawDataFileReader>(std::move(backend), file_name);
  } catch (rawdatafilelibs::FileOpenFailed const& excpt) {
    // pass on the hdf5libs issue that the backend raised (e.g. BadRecordType), if there was one
    if (auto const* cause = excpt.cause(); cause != nullptr) {
      if (std::string(cause->get_class_name()).rfind("hdf5libs::", 0) == 0) {
        cause->raise();
      }
    }
    throw FileOpenFailed(ERS_HERE, file_name, excpt.message(), excpt);
  }
  m_reader_backend = backend_ptr;

  auto const& metadata = m_reader->get_metadata();
  m_file_layout_ptr = std::make_unique<HDF5FileLayout>(m_reader_backend->get_file_layout());
  m_compression_level = metadata.compression_level;
  m_record_type = m_reader_backend->get_record_type_name();
  m_geo_id_map = m_reader->get_geo_id_map();
  m_final_stats = metadata.stats;
}

HDF5RawDataFile::~HDF5RawDataFile()
{
  try {
    close();
  } catch (std::exception const& excpt) {
    ers::error(FileCloseFailed(ERS_HERE, m_bare_file_name, excpt.what()));
  } catch (...) { // NOLINT(runtime/exceptions)
    ers::error(FileCloseFailed(ERS_HERE, m_bare_file_name, "unknown exception"));
  }
}

/**
 * @brief Finalize and close the file (see the header for the contract).
 */
void
HDF5RawDataFile::close()
{
  if (m_writer != nullptr) {
    auto writer = std::move(m_writer); // released even when closing fails
    m_writer_backend = nullptr;
    try {
      writer->close();
    } catch (rawdatafilelibs::FileCloseFailed const& excpt) {
      m_closed_file_name = writer->get_current_file_name();
      throw FileCloseFailed(ERS_HERE, m_closed_file_name, excpt.message(), excpt);
    }
    m_closed_file_name = writer->get_current_file_name();
    m_final_stats = writer->get_metadata().stats;
    return;
  }

  if (m_reader == nullptr) {
    return;
  }

  auto reader = std::move(m_reader); // released even when closing fails
  auto* backend = m_reader_backend;
  m_reader_backend = nullptr;
  const std::string open_file_name = reader->get_path();
  m_closed_file_name = open_file_name;

  if (m_allow_writing) {
    // the attribute failure is what gets reported, not a failure to release the file after it
    auto release_quietly = [&reader]() {
      try {
        reader->close();
      } catch (...) { // NOLINT(runtime/exceptions)
      }
    };
    // a file opened for modification (e.g. to recover it) gets any missing closing attributes
    try {
      auto& file = backend->get_file();
      if (!file.hasAttribute("recorded_size")) {
        file.createAttribute("recorded_size", m_final_stats.recorded_size);
      }
      if (!file.hasAttribute("uncompressed_raw_data_size")) {
        file.createAttribute("uncompressed_raw_data_size", m_final_stats.uncompressed_raw_data_size);
      }
      if (!file.hasAttribute("total_file_size")) {
        file.createAttribute("total_file_size", m_final_stats.total_file_size);
      }
      if (!file.hasAttribute("closing_timestamp")) {
        size_t file_closing_timestamp =
          std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();
        file.createAttribute("closing_timestamp", file_closing_timestamp);
      }
    } catch (std::exception const& excpt) {
      release_quietly();
      throw FileCloseFailed(ERS_HERE, open_file_name, std::string("could not finalize the file: ") + excpt.what());
    } catch (...) { // NOLINT(runtime/exceptions)
      release_quietly();
      throw FileCloseFailed(ERS_HERE, open_file_name, "could not finalize the file: unknown exception");
    }
  }

  try {
    reader->close();
  } catch (rawdatafilelibs::FileCloseFailed const& excpt) {
    throw FileCloseFailed(ERS_HERE, open_file_name, excpt.message(), excpt);
  }

  // rename the file to the bare name, if needed, never replacing an existing file
  if (m_allow_writing && open_file_name != m_bare_file_name) {
    try {
      rename_without_replacing(open_file_name, m_bare_file_name);
    } catch (std::system_error const& excpt) {
      std::string reason = (excpt.code() == std::errc::file_exists)
                             ? "the final name " + m_bare_file_name + " already exists"
                             : excpt.what();
      throw FileCloseFailed(ERS_HERE, open_file_name, reason + "; the file keeps its in-progress name");
    }
    m_closed_file_name = m_bare_file_name;
  }
}

std::string
HDF5RawDataFile::get_file_name() const
{
  if (m_writer != nullptr) {
    return m_writer->get_current_file_name();
  }
  if (m_reader != nullptr) {
    return m_reader->get_path();
  }
  return m_closed_file_name.empty() ? m_bare_file_name : m_closed_file_name;
}

rawdatafilelibs::FileStats
HDF5RawDataFile::get_stats() const noexcept
{
  if (m_writer != nullptr) {
    try {
      return m_writer->get_stats();
    } catch (...) { // NOLINT(runtime/exceptions)
      return m_final_stats;
    }
  }
  return m_final_stats;
}

HighFive::File&
HDF5RawDataFile::get_h5_file() const
{
  if (m_writer_backend != nullptr) {
    return m_writer_backend->get_file();
  }
  if (m_reader_backend != nullptr) {
    return m_reader_backend->get_file();
  }
  throw FileOpenFailed(ERS_HERE, m_bare_file_name, "the file has been closed");
}

rawdatafilelibs::RawDataFileReader&
HDF5RawDataFile::get_reader() const
{
  if (m_reader == nullptr) {
    throw FileOpenFailed(ERS_HERE,
                         get_file_name(),
                         m_writer != nullptr ? "records can't be read from a file that is open for writing"
                                             : "the file has been closed");
  }
  return *m_reader;
}

HDF5ReaderBackend&
HDF5RawDataFile::get_reader_backend() const
{
  get_reader();
  return *m_reader_backend;
}

/**
 * @brief Fetch the list of all file-level Attribute names.
 */
std::vector<std::string>
HDF5RawDataFile::get_attribute_names()
{
  return get_h5_file().listAttributeNames();
}

/**
 * @brief Write a TriggerRecord to the file.
 */
void
HDF5RawDataFile::write(const daqdataformats::TriggerRecord& tr)
{
  if (m_writer == nullptr) {
    throw FileOpenFailed(ERS_HERE, get_file_name(), "the file is not open for writing");
  }
  m_writer->write(tr);
}

/**
 * @brief Write a TimeSlice to the file.
 */
void
HDF5RawDataFile::write(const daqdataformats::TimeSlice& ts)
{
  if (m_writer == nullptr) {
    throw FileOpenFailed(ERS_HERE, get_file_name(), "the file is not open for writing");
  }
  m_writer->write(ts);
}

bool
HDF5RawDataFile::timeslice_already_exists(const daqdataformats::TimeSlice& timeslice) const
{
  if (m_writer != nullptr) {
    return m_writer->record_exists(daqdataformats::RecordID{ timeslice.get_header().timeslice_number });
  }
  return get_reader().contains(daqdataformats::RecordID{ timeslice.get_header().timeslice_number });
}

void
HDF5RawDataFile::check_record_type(std::string rt_name)
{
  if (get_version() < 2)
    return;

  if (m_file_layout_ptr->get_record_name_prefix().compare(rt_name) != 0)
    throw WrongRecordTypeRequested(ERS_HERE, rt_name, m_file_layout_ptr->get_record_name_prefix());
}

void
HDF5RawDataFile::throw_if_record_id_not_found(const record_id_t& rid)
{
  if (get_all_record_ids().count(rid) == 0) {
    throw RecordIDNotFound(ERS_HERE, rid.first, rid.second);
  }
}

const daqdataformats::RecordIndex&
HDF5RawDataFile::get_index(const record_id_t& rid)
{
  throw_if_record_id_not_found(rid);
  return get_reader().get_index(to_record_id(rid));
}

daqdataformats::SourceIDGeoIDMap
HDF5RawDataFile::get_record_geo_id_map(const record_id_t& rid)
{
  return m_geo_id_map.restricted_to(get_index(rid));
}

/**
 * @brief Return a vector of dataset names
 */
std::vector<std::string>
HDF5RawDataFile::get_dataset_paths(std::string top_level_group_name)
{
  if (top_level_group_name.empty())
    top_level_group_name = get_h5_file().getPath();

  HighFive::Group parent_group = get_h5_file().getGroup(top_level_group_name);
  if (!parent_group.isValid())
    throw InvalidHDF5Group(ERS_HERE, top_level_group_name);

  return collect_dataset_paths(parent_group, top_level_group_name);
}

/**
 * @brief Return all of the record numbers in the file.
 */
const HDF5RawDataFile::record_id_set& // NOLINT(build/unsigned)
HDF5RawDataFile::get_all_record_ids()
{
  if (!m_all_record_ids_in_file.empty())
    return m_all_record_ids_in_file;

  for (auto const& record_id : get_reader().get_record_ids()) {
    m_all_record_ids_in_file.insert(std::make_pair(record_id.number, record_id.sequence));
  }
  return m_all_record_ids_in_file;
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
      rec_paths.push_back(get_record_header_dataset_path(rec_id));
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
HDF5RawDataFile::get_record_header_dataset_path(const record_id_t& rid)
{
  throw_if_record_id_not_found(rid);

  if (get_version() <= 2) {
    return (get_h5_file().getPath() + m_file_layout_ptr->get_record_header_path(rid.first, rid.second));
  } else {
    auto const& index = get_index(rid);
    return get_reader_backend().get_dataset_path(index.get_record_id(), index.get_header_source_id());
  }
}

std::string
HDF5RawDataFile::get_record_header_dataset_path(const uint64_t rec_num, // NOLINT (build/unsigned)
                                                const daqdataformats::sequence_number_t seq_num)
{
  return get_record_header_dataset_path(std::make_pair(rec_num, seq_num));
}

std::string
HDF5RawDataFile::get_trigger_record_header_dataset_path(const record_id_t& rid)
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
HDF5RawDataFile::get_timeslice_header_dataset_path(const record_id_t& rid)
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
HDF5RawDataFile::get_fragment_dataset_paths(const record_id_t& rid)
{
  throw_if_record_id_not_found(rid);

  std::vector<std::string> frag_paths;
  if (get_version() <= 2) {
    std::string record_group_path =
      get_h5_file().getPath() + m_file_layout_ptr->get_record_number_string(rid.first, rid.second);

    for (auto const& path : get_dataset_paths(record_group_path)) {
      if (path.find(m_file_layout_ptr->get_record_header_dataset_name()) == std::string::npos)
        frag_paths.push_back(path);
    }
  } else {
    for (auto const& source_id : get_fragment_source_ids(rid)) {
      frag_paths.push_back(get_reader_backend().get_dataset_path(to_record_id(rid), source_id));
    }
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

// get all fragment dataset paths for a Subsystem
std::vector<std::string>
HDF5RawDataFile::get_fragment_dataset_paths(const daqdataformats::SourceID::Subsystem subsystem)
{
  std::vector<std::string> frag_paths;
  for (auto const& rid : get_all_record_ids()) {
    auto datasets = get_fragment_dataset_paths(rid, subsystem);
    frag_paths.insert(frag_paths.end(), datasets.begin(), datasets.end());
  }
  return frag_paths;
}

std::vector<std::string>
HDF5RawDataFile::get_fragment_dataset_paths(const std::string& subsystem_name)
{
  daqdataformats::SourceID::Subsystem subsystem = daqdataformats::SourceID::string_to_subsystem(subsystem_name);
  return get_fragment_dataset_paths(subsystem);
}

std::vector<std::string>
HDF5RawDataFile::get_fragment_dataset_paths(const record_id_t& rid, const daqdataformats::SourceID::Subsystem subsystem)
{
  throw_if_record_id_not_found(rid);

  if (get_version() <= 2) {
    return get_dataset_paths(get_h5_file().getPath() +
                             m_file_layout_ptr->get_fragment_type_path(rid.first, rid.second, subsystem));
  } else {
    std::vector<std::string> frag_paths;
    for (auto const& source_id : get_source_ids_for_subsystem(rid, subsystem)) {
      frag_paths.push_back(get_reader_backend().get_dataset_path(to_record_id(rid), source_id));
    }
    return frag_paths;
  }
}

std::vector<std::string>
HDF5RawDataFile::get_fragment_dataset_paths(const record_id_t& rid, const std::string& subsystem_name)
{
  daqdataformats::SourceID::Subsystem subsystem = daqdataformats::SourceID::string_to_subsystem(subsystem_name);
  return get_fragment_dataset_paths(rid, subsystem);
}

HDF5SourceIDHandler::source_id_geo_id_map_t
HDF5RawDataFile::get_srcid_geoid_map() const
{
  return m_geo_id_map.to_encoded();
}

std::set<uint64_t> // NOLINT(build/unsigned)
HDF5RawDataFile::get_all_geo_ids() const
{
  return encoded_geo_id_set(m_geo_id_map.get_all_geo_ids());
}

std::set<uint64_t> // NOLINT(build/unsigned)
HDF5RawDataFile::get_geo_ids(const record_id_t& rid)
{
  return encoded_geo_id_set(get_record_geo_id_map(rid).get_all_geo_ids());
}

std::set<uint64_t> // NOLINT(build/unsigned)
HDF5RawDataFile::get_geo_ids_for_subdetector(const record_id_t& rid, const detdataformats::DetID::Subdetector subdet)
{
  return encoded_geo_id_set(get_record_geo_id_map(rid).get_geo_ids(subdet));
}

// get all SourceIDs for given record ID: the fragments' and the record header's
std::set<daqdataformats::SourceID>
HDF5RawDataFile::get_source_ids(const record_id_t& rid)
{
  auto const& index = get_index(rid);
  std::set<daqdataformats::SourceID> source_ids = index.get_fragment_source_ids();
  source_ids.insert(index.get_header_source_id());
  return source_ids;
}

daqdataformats::SourceID
HDF5RawDataFile::get_record_header_source_id(const record_id_t& rid)
{
  return get_index(rid).get_header_source_id();
}

std::set<daqdataformats::SourceID>
HDF5RawDataFile::get_fragment_source_ids(const record_id_t& rid)
{
  return get_index(rid).get_fragment_source_ids();
}

// as before, this includes the record header's SourceID when it is of the requested subsystem
std::set<daqdataformats::SourceID>
HDF5RawDataFile::get_source_ids_for_subsystem(const record_id_t& rid,
                                              const daqdataformats::SourceID::Subsystem subsystem)
{
  auto const& index = get_index(rid);
  daqdataformats::SourceIDSelector selector;
  selector.subsystem = subsystem;
  std::set<daqdataformats::SourceID> source_ids = index.get_fragment_source_ids(selector);
  if (index.get_header_source_id().subsystem == subsystem) {
    source_ids.insert(index.get_header_source_id());
  }
  return source_ids;
}

std::set<daqdataformats::SourceID>
HDF5RawDataFile::get_source_ids_for_fragment_type(const record_id_t& rid, const daqdataformats::FragmentType frag_type)
{
  daqdataformats::SourceIDSelector selector;
  selector.fragment_type = frag_type;
  return get_index(rid).get_fragment_source_ids(selector);
}

std::set<daqdataformats::SourceID>
HDF5RawDataFile::get_source_ids_for_fragtype_and_subdetector(const record_id_t& rid,
                                                             const std::string& frag_type_name,
                                                             const std::string& subdet_name)
{
  daqdataformats::FragmentType frag_type = daqdataformats::string_to_fragment_type(frag_type_name);
  if (frag_type == daqdataformats::FragmentType::kUnknown)
    throw InvalidFragmentTypeString(ERS_HERE, frag_type_name);
  detdataformats::DetID::Subdetector subdet = detdataformats::DetID::string_to_subdetector(subdet_name);
  if (subdet == detdataformats::DetID::Subdetector::kUnknown)
    throw InvalidSubdetectorString(ERS_HERE, subdet_name);

  daqdataformats::SourceIDSelector selector;
  selector.fragment_type = frag_type;
  selector.subdetector = subdet;
  return get_index(rid).get_fragment_source_ids(selector);
}

std::set<daqdataformats::SourceID>
HDF5RawDataFile::get_source_ids_for_subdetector(const record_id_t& rid, const detdataformats::DetID::Subdetector subdet)
{
  daqdataformats::SourceIDSelector selector;
  selector.subdetector = subdet;
  return get_index(rid).get_fragment_source_ids(selector);
}

std::unique_ptr<char[]>
HDF5RawDataFile::get_dataset_raw_data(const std::string& dataset_path)
{
  auto buffer = get_reader_backend().read_dataset(dataset_path);
  auto membuffer = std::make_unique<char[]>(buffer.size);
  std::memcpy(membuffer.get(), buffer.data.get(), buffer.size);
  return membuffer;
}

std::unique_ptr<daqdataformats::Fragment>
HDF5RawDataFile::get_frag_ptr(const std::string& dataset_name)
{
  // the backend's buffers come from malloc, so the Fragment can take one over (it frees with free())
  auto buffer = get_reader_backend().read_dataset(dataset_name);
  return std::make_unique<daqdataformats::Fragment>(buffer.data.release(),
                                                    daqdataformats::Fragment::BufferAdoptionMode::kTakeOverBuffer);
}

std::unique_ptr<daqdataformats::Fragment>
HDF5RawDataFile::get_frag_ptr(const record_id_t& rid, const daqdataformats::SourceID& source_id)
{
  throw_if_record_id_not_found(rid);
  return get_frag_ptr(get_reader_backend().get_dataset_path(to_record_id(rid), source_id));
}

std::unique_ptr<daqdataformats::Fragment>
HDF5RawDataFile::get_frag_ptr(const uint64_t rec_num, // NOLINT(build/unsigned)
                              const daqdataformats::sequence_number_t seq_num,
                              const daqdataformats::SourceID& source_id)
{
  record_id_t rid = std::make_pair(rec_num, seq_num);
  return get_frag_ptr(rid, source_id);
}

std::unique_ptr<daqdataformats::Fragment>
HDF5RawDataFile::get_frag_ptr(const record_id_t& rid,
                              const daqdataformats::SourceID::Subsystem type,
                              const uint32_t id) // NOLINT(build/unsigned)
{
  daqdataformats::SourceID source_id(type, id);
  return get_frag_ptr(rid, source_id);
}

std::unique_ptr<daqdataformats::Fragment>
HDF5RawDataFile::get_frag_ptr(const uint64_t rec_num, // NOLINT(build/unsigned)
                              const daqdataformats::sequence_number_t seq_num,
                              const daqdataformats::SourceID::Subsystem type,
                              const uint32_t id) // NOLINT(build/unsigned)
{
  record_id_t rid = std::make_pair(rec_num, seq_num);
  daqdataformats::SourceID source_id(type, id);
  return get_frag_ptr(rid, source_id);
}

std::unique_ptr<daqdataformats::Fragment>
HDF5RawDataFile::get_frag_ptr(const record_id_t& rid,
                              const std::string& typestring,
                              const uint32_t id) // NOLINT(build/unsigned)
{
  daqdataformats::SourceID source_id(daqdataformats::SourceID::string_to_subsystem(typestring), id);
  return get_frag_ptr(rid, source_id);
}

std::unique_ptr<daqdataformats::Fragment>
HDF5RawDataFile::get_frag_ptr(const uint64_t rec_num, // NOLINT(build/unsigned)
                              const daqdataformats::sequence_number_t seq_num,
                              const std::string& typestring,
                              const uint32_t id) // NOLINT(build/unsigned)
{
  record_id_t rid = std::make_pair(rec_num, seq_num);
  daqdataformats::SourceID source_id(daqdataformats::SourceID::string_to_subsystem(typestring), id);
  return get_frag_ptr(rid, source_id);
}

std::unique_ptr<daqdataformats::Fragment>
HDF5RawDataFile::get_frag_ptr(const record_id_t& rid,
                              const uint64_t geo_id) // NOLINT(build/unsigned)
{
  daqdataformats::SourceID sid = get_source_id_for_geo_id(rid, geo_id);
  return get_frag_ptr(rid, sid);
}

std::unique_ptr<daqdataformats::Fragment>
HDF5RawDataFile::get_frag_ptr(const uint64_t rec_num, // NOLINT(build/unsigned)
                              const daqdataformats::sequence_number_t seq_num,
                              const uint64_t geo_id) // NOLINT(build/unsigned)
{
  record_id_t rid = std::make_pair(rec_num, seq_num);
  return get_frag_ptr(rid, geo_id);
}

std::unique_ptr<daqdataformats::TriggerRecordHeader>
HDF5RawDataFile::get_trh_ptr(const std::string& dataset_name)
{
  auto buffer = get_reader_backend().read_dataset(dataset_name);
  // the header copies the buffer, which is then freed
  return std::make_unique<daqdataformats::TriggerRecordHeader>(buffer.data.get(), true);
}

std::unique_ptr<daqdataformats::TriggerRecordHeader>
HDF5RawDataFile::get_trh_ptr(const record_id_t& rid)
{
  auto const& index = get_index(rid);
  return get_trh_ptr(get_reader_backend().get_dataset_path(index.get_record_id(), index.get_header_source_id()));
}

std::unique_ptr<daqdataformats::TimeSliceHeader>
HDF5RawDataFile::get_tsh_ptr(const std::string& dataset_name)
{
  auto buffer = get_reader_backend().read_dataset(dataset_name);
  auto tsh_ptr = std::make_unique<daqdataformats::TimeSliceHeader>();
  std::memcpy(tsh_ptr.get(), buffer.data.get(), std::min(buffer.size, sizeof(daqdataformats::TimeSliceHeader)));
  return tsh_ptr;
}

std::unique_ptr<daqdataformats::TimeSliceHeader>
HDF5RawDataFile::get_tsh_ptr(const record_id_t& rid)
{
  auto const& index = get_index(rid);
  return get_tsh_ptr(get_reader_backend().get_dataset_path(index.get_record_id(), index.get_header_source_id()));
}

daqdataformats::TriggerRecord
HDF5RawDataFile::get_trigger_record(const record_id_t& rid)
{
  daqdataformats::TriggerRecord trigger_record(*get_trh_ptr(rid));
  for (auto const& frag_path : get_fragment_dataset_paths(rid)) {
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

std::vector<uint64_t> // NOLINT(build/unsigned)
HDF5RawDataFile::get_geo_ids_for_source_id(const record_id_t& rid, const daqdataformats::SourceID& source_id)
{
  return encoded_geo_ids(get_record_geo_id_map(rid).get_geo_ids(source_id));
}

daqdataformats::SourceID
HDF5RawDataFile::get_source_id_for_geo_id(const record_id_t& rid,
                                          const uint64_t requested_geo_id) // NOLINT(build/unsigned)
{
  auto source_id = get_record_geo_id_map(rid).get_source_id(detdataformats::GeoID::decode(requested_geo_id));
  return source_id.value_or(daqdataformats::SourceID());
}

} // namespace hdf5libs
} // namespace dunedaq
