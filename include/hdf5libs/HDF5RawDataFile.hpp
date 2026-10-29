/**
 * @file HDF5RawDataFile.hpp
 *
 * DEPRECATED interface for reading and writing DUNE DAQ HDF5 files. New code should use
 * rawdatafilelibs::open_raw_data_file() / create_raw_data_file() (RawDataFileReader and
 * RawDataFileWriter), which work for any file format. HDF5RawDataFile is kept, unchanged in
 * behaviour, for one release cycle; it is now implemented on top of those classes and the
 * HDF5 backend (HDF5Backend.hpp).
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef HDF5LIBS_INCLUDE_HDF5LIBS_HDF5RAWDATAFILE_HPP_
#define HDF5LIBS_INCLUDE_HDF5LIBS_HDF5RAWDATAFILE_HPP_

// DUNE-DAQ
#include "hdf5libs/HDF5Backend.hpp"
#include "hdf5libs/HDF5FileLayout.hpp"
#include "hdf5libs/HDF5SourceIDHandler.hpp"

#include "rawdatafilelibs/FileMetadata.hpp"
#include "rawdatafilelibs/RawDataFileReader.hpp"
#include "rawdatafilelibs/RawDataFileWriter.hpp"

#include "daqdataformats/Fragment.hpp"
#include "daqdataformats/RecordID.hpp"
#include "daqdataformats/SourceIDGeoIDMap.hpp"
#include "daqdataformats/TimeSlice.hpp"
#include "daqdataformats/TriggerRecord.hpp"
#include "logging/Logging.hpp" // NOTE: if ISSUES ARE DECLARED BEFORE include logging/Logging.hpp, TLOG_DEBUG<<issue wont work.

#include "hdf5libs/Issues.hpp"

// External Packages
#include <highfive/H5DataSet.hpp>
#include <highfive/H5File.hpp>
#include <highfive/H5Group.hpp>
#include <highfive/H5Object.hpp>
#include <nlohmann/json.hpp>

// System
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <sys/statvfs.h>
#include <utility>
#include <variant>
#include <vector>

namespace dunedaq {

namespace hdf5libs {

// Renames a file without ever replacing an existing file at the new name, atomically (so a file
// that appears at the new name concurrently is not replaced either). The same as
// rawdatafilelibs::rename_without_replacing: throws std::system_error (code EEXIST if the new
// name exists); on failure the file keeps its old name.
void
rename_without_replacing(const std::string& from, const std::string& to);

/**
 * @brief HDF5RawDataFile is the class responsible
 * for interfacing the DAQ format with the HDF5 file format.
 *
 * DEPRECATED: see the file comment. Differences from earlier releases:
 *  - files are always created exclusively (open_flags other than Excl are ignored, with a warning);
 *  - per-record GeoID functions (get_geo_ids(rid), get_geo_ids_for_subdetector(),
 *    get_geo_ids_for_source_id(), get_source_id_for_geo_id()) only report GeoIDs of fragments
 *    that are actually in the record;
 *  - records of layout version <= 2 files are indexed by scanning their dataset paths, so the
 *    SourceID-based functions work for them too;
 *  - a file opened for writing can't use the record-reading functions (they throw FileOpenFailed);
 *    attributes and dataset paths can still be read.
 * get_source_id_for_geo_id() still returns the first (lowest) SourceID for a GeoID that the file
 * lists under more than one SourceID, unlike rawdatafilelibs, which gives no single answer for one.
 */
class HDF5RawDataFile
{

public:
  enum
  {
    TLVL_BASIC = 2,
    TLVL_FILE_SIZE = 5
  };

  // define a record number type
  // that is a pair of the trigger record or timeslice number and sequence number
  typedef std::pair<uint64_t, daqdataformats::sequence_number_t> record_id_t; // NOLINT(build/unsigned)
  typedef std::set<record_id_t, std::less<>> record_id_set;

  inline static const std::string s_inprogress_suffix = ".writing";

  // Constructor for writing. Throws FileOpenFailed if the in-progress or the final name already
  // exists; an existing file is never appended to, overwritten or replaced.
  HDF5RawDataFile(std::string file_name,
                  daqdataformats::run_number_t run_number,
                  size_t file_index,
                  std::string application_name,
                  HDF5FileLayoutParameters fl_params,
                  HDF5SourceIDHandler::source_id_geo_id_map_t srcid_geoid_map,
                  unsigned compression_level = 0,
                  std::string inprogress_filename_suffix = s_inprogress_suffix,
                  unsigned open_flags = HighFive::File::Excl);

  // constructor for writing, with the complete file metadata: besides the attributes written by the
  // constructor above, this also writes operational_environment, offline_data_stream and
  // run_was_for_test_purposes, and uses metadata.creation_timestamp_ms (0 means now)
  HDF5RawDataFile(const std::string& file_name,
                  const rawdatafilelibs::FileMetadata& metadata,
                  const HDF5FileLayoutParameters& fl_params,
                  const daqdataformats::SourceIDGeoIDMap& geo_id_map,
                  const std::string& inprogress_filename_suffix = s_inprogress_suffix);

  // constructor for reading and optional writing
  explicit HDF5RawDataFile(const std::string& file_name, bool allow_writing = false);

  // Calls close(); any failure is reported with ers::error, since a destructor must not throw.
  ~HDF5RawDataFile();

  // Finalizes a writable file and renames it to drop the in-progress suffix, never replacing an
  // existing file: if the final name exists, throws FileCloseFailed and the file keeps its
  // in-progress name. The file is always released, even on failure; calling close() again is a
  // no-op. Only get_file_name() may be used after close().
  void close();

  // The name of the file on disk: the in-progress name while open; after close(), the final name,
  // or the in-progress name if close() could not rename the file
  std::string get_file_name() const;
  static std::string get_file_name_extension() { return "hdf5"; }

  size_t get_recorded_size() const noexcept { return get_stats().recorded_size; }
  size_t get_uncompressed_raw_data_size() const noexcept { return get_stats().uncompressed_raw_data_size; }
  size_t get_total_file_size() const noexcept { return get_stats().total_file_size; }
  unsigned get_compression_level() const noexcept { return m_compression_level; }

  std::string get_record_type() const noexcept { return m_record_type; }

  bool is_trigger_record_type() const noexcept { return m_record_type.compare("TriggerRecord") == 0; }
  bool is_timeslice_type() const noexcept { return m_record_type.compare("TimeSlice") == 0; }

  const HDF5FileLayout& get_file_layout() const { return *(m_file_layout_ptr.get()); }

  uint32_t get_version() const // NOLINT(build/unsigned)
  {
    return m_file_layout_ptr->get_version();
  }

  // basic data writing methods
public:
  void write(const daqdataformats::TriggerRecord& tr);
  void write(const daqdataformats::TimeSlice& ts);

  // attribute writers/getters
  template<typename T>
  void write_attribute(std::string name, T value);
  template<typename T>
  void write_attribute(HighFive::Group& grp, const std::string& name, T value);
  template<typename T>
  void write_attribute(HighFive::DataSet& dset, const std::string& name, T value);

  std::vector<std::string> get_attribute_names();
  template<typename T>
  T get_attribute(const std::string& name);
  template<typename T>
  T get_attribute(const HighFive::Group& grp, const std::string& name);
  template<typename T>
  T get_attribute(const HighFive::DataSet& dset, std::string name);
  // For backwards compatibility with files created before compression_level existed
  template<typename T>
  T get_attribute_if_exists(const std::string& name, const T& default_value);

  std::vector<std::string> get_dataset_paths(std::string top_level_group_name = "");

  // The returned set is owned by this object and cached on first use.
  const record_id_set& get_all_record_ids();
  record_id_set get_all_trigger_record_ids();
  record_id_set get_all_timeslice_ids();

  std::set<uint64_t> get_all_record_numbers(); // NOLINT(build/unsigned)
  std::set<daqdataformats::trigger_number_t> get_all_trigger_record_numbers();
  std::set<daqdataformats::timeslice_number_t> get_all_timeslice_numbers();

  std::vector<std::string> get_record_header_dataset_paths();
  std::vector<std::string> get_trigger_record_header_dataset_paths();
  std::vector<std::string> get_timeslice_header_dataset_paths();

  std::string get_record_header_dataset_path(const record_id_t& rid);
  std::string get_record_header_dataset_path(const uint64_t rec_num, // NOLINT (build/unsigned)
                                             const daqdataformats::sequence_number_t seq_num = 0);
  std::string get_trigger_record_header_dataset_path(const record_id_t& rid);
  std::string get_trigger_record_header_dataset_path(const daqdataformats::trigger_number_t trig_num,
                                                     const daqdataformats::sequence_number_t seq_num = 0);
  std::string get_timeslice_header_dataset_path(const record_id_t& rid);
  std::string get_timeslice_header_dataset_path(const daqdataformats::timeslice_number_t trig_num);

  // get all fragment dataset paths
  std::vector<std::string> get_all_fragment_dataset_paths();

  // get all fragment dataset paths for given record ID
  std::vector<std::string> get_fragment_dataset_paths(const record_id_t& rid);
  std::vector<std::string> get_fragment_dataset_paths(const uint64_t rec_num, // NOLINT (build/unsigned)
                                                      const daqdataformats::sequence_number_t seq_num = 0);

  // get all fragment dataset paths for a Subsystem
  std::vector<std::string> get_fragment_dataset_paths(const daqdataformats::SourceID::Subsystem subsystem);
  std::vector<std::string> get_fragment_dataset_paths(const std::string& subsystem_name);

  // get all fragment dataset paths for a record ID and Subsystem
  std::vector<std::string> get_fragment_dataset_paths(const record_id_t& rid,
                                                      const daqdataformats::SourceID::Subsystem subsystem);
  std::vector<std::string> get_fragment_dataset_paths(const record_id_t& rid, const std::string& subsystem_name);

#if 0
  // get all fragment dataset paths for a SourceID
  std::vector<std::string> get_fragment_dataset_paths(const daqdataformats::SourceID& source_id);
  std::vector<std::string> get_fragment_dataset_paths(const daqdataformats::SourceID::Subsystem type,
  const uint32_t id); // NOLINT(build/unsigned)
  std::vector<std::string> get_fragment_dataset_paths(const std::string& typestring,
  const uint32_t id); // NOLINT(build/unsigned)

  // get a list of all the source ids from a list of fragment dataset paths
  std::set<daqdataformats::SourceID> get_source_ids(std::vector<std::string> const& frag_dataset_paths);
#endif

  HDF5SourceIDHandler::source_id_geo_id_map_t get_srcid_geoid_map() const;

  // get a list of all the geo ids anywhere in the file
  std::set<uint64_t> get_all_geo_ids() const; // NOLINT(build/unsigned)

  // get GeoIDs in a record
  std::set<uint64_t> get_geo_ids(const record_id_t& rid); // NOLINT(build/unsigned)
  std::set<uint64_t> get_geo_ids(const uint64_t rec_num,  // NOLINT(build/unsigned)
                                 const daqdataformats::sequence_number_t seq_num = 0)
  {
    return get_geo_ids(std::make_pair(rec_num, seq_num));
  }
  std::set<uint64_t> get_geo_ids_for_subdetector(const record_id_t& rid, // NOLINT(build/unsigned)
                                                 const detdataformats::DetID::Subdetector subdet);
  std::set<uint64_t> get_geo_ids_for_subdetector(const uint64_t rec_num, // NOLINT(build/unsigned)
                                                 const daqdataformats::sequence_number_t seq_num,
                                                 const detdataformats::DetID::Subdetector subdet)
  {
    return get_geo_ids_for_subdetector(std::make_pair(rec_num, seq_num), subdet);
  }
  std::set<uint64_t> get_geo_ids_for_subdetector(const record_id_t& rid, // NOLINT(build/unsigned)
                                                 const std::string& subdet_name)
  {
    detdataformats::DetID::Subdetector subdet = detdataformats::DetID::string_to_subdetector(subdet_name);
    return get_geo_ids_for_subdetector(rid, subdet);
  }
  std::set<uint64_t> get_geo_ids_for_subdetector(const uint64_t rec_num, // NOLINT(build/unsigned)
                                                 const daqdataformats::sequence_number_t seq_num,
                                                 const std::string& subdet_name)
  {
    return get_geo_ids_for_subdetector(std::make_pair(rec_num, seq_num), subdet_name);
  }

  // get SourceIDs in a record
  std::set<daqdataformats::SourceID> get_source_ids(const record_id_t& rid);
  std::set<daqdataformats::SourceID> get_source_ids(const uint64_t rec_num, // NOLINT(build/unsigned)
                                                    const daqdataformats::sequence_number_t seq_num = 0)
  {
    return get_source_ids(std::make_pair(rec_num, seq_num));
  }

  daqdataformats::SourceID get_record_header_source_id(const record_id_t& rid);
  daqdataformats::SourceID get_record_header_source_id(const uint64_t rec_num, // NOLINT(build/unsigned)
                                                       const daqdataformats::sequence_number_t seq_num = 0)
  {
    return get_record_header_source_id(std::make_pair(rec_num, seq_num));
  }

  std::set<daqdataformats::SourceID> get_fragment_source_ids(const record_id_t& rid);
  std::set<daqdataformats::SourceID> get_fragment_source_ids(const uint64_t rec_num, // NOLINT(build/unsigned)
                                                             const daqdataformats::sequence_number_t seq_num = 0)
  {
    return get_fragment_source_ids(std::make_pair(rec_num, seq_num));
  }

  // get SourceIDs for given subsystem in a record
  std::set<daqdataformats::SourceID> get_source_ids_for_subsystem(const record_id_t& rid,
                                                                  const daqdataformats::SourceID::Subsystem subsystem);
  std::set<daqdataformats::SourceID> get_source_ids_for_subsystem(const record_id_t& rid,
                                                                  const std::string& subsystem_name)
  {
    daqdataformats::SourceID::Subsystem subsys = daqdataformats::SourceID::string_to_subsystem(subsystem_name);
    return get_source_ids_for_subsystem(rid, subsys);
  }
  std::set<daqdataformats::SourceID> get_source_ids_for_subsystem(const uint64_t rec_num, // NOLINT(build/unsigned)
                                                                  const daqdataformats::sequence_number_t seq_num,
                                                                  const daqdataformats::SourceID::Subsystem subsystem)
  {
    return get_source_ids_for_subsystem(std::make_pair(rec_num, seq_num), subsystem);
  }
  std::set<daqdataformats::SourceID> get_source_ids_for_subsystem(const uint64_t rec_num, // NOLINT(build/unsigned)
                                                                  const daqdataformats::sequence_number_t seq_num,
                                                                  const std::string& subsystem_name)
  {
    return get_source_ids_for_subsystem(std::make_pair(rec_num, seq_num), subsystem_name);
  }

  // get SourceIDs for given fragment type in a record
  std::set<daqdataformats::SourceID> get_source_ids_for_fragment_type(const record_id_t& rid,
                                                                      const daqdataformats::FragmentType frag_type);
  std::set<daqdataformats::SourceID> get_source_ids_for_fragment_type(const record_id_t& rid,
                                                                      const std::string& frag_type_name)
  {
    daqdataformats::FragmentType frag_type = daqdataformats::string_to_fragment_type(frag_type_name);
    if (frag_type == daqdataformats::FragmentType::kUnknown)
      throw InvalidFragmentTypeString(ERS_HERE, frag_type_name);
    return get_source_ids_for_fragment_type(rid, frag_type);
  }
  std::set<daqdataformats::SourceID> get_source_ids_for_fragment_type(const uint64_t rec_num, // NOLINT(build/unsigned)
                                                                      const daqdataformats::sequence_number_t seq_num,
                                                                      const daqdataformats::FragmentType frag_type)
  {
    return get_source_ids_for_fragment_type(std::make_pair(rec_num, seq_num), frag_type);
  }
  std::set<daqdataformats::SourceID> get_source_ids_for_fragment_type(const uint64_t rec_num, // NOLINT(build/unsigned)
                                                                      const daqdataformats::sequence_number_t seq_num,
                                                                      const std::string& frag_type_name)
  {
    return get_source_ids_for_fragment_type(std::make_pair(rec_num, seq_num), frag_type_name);
  }

  // get SourceIDs for given fragment type and subdetector in a record
  std::set<daqdataformats::SourceID> get_source_ids_for_fragtype_and_subdetector(const record_id_t& rid,
                                                                                 const std::string& frag_type_name,
                                                                                 const std::string& subdet_name);

  // get SourceIDs for given subdetector in a record
  std::set<daqdataformats::SourceID> get_source_ids_for_subdetector(const record_id_t& rid,
                                                                    const detdataformats::DetID::Subdetector subdet);
  std::set<daqdataformats::SourceID> get_source_ids_for_subdetector(const record_id_t& rid,
                                                                    const std::string& subdet_name)
  {
    detdataformats::DetID::Subdetector subdet = detdataformats::DetID::string_to_subdetector(subdet_name);
    return get_source_ids_for_subdetector(rid, subdet);
  }
  std::set<daqdataformats::SourceID> get_source_ids_for_subdetector(const uint64_t rec_num, // NOLINT(build/unsigned)
                                                                    const daqdataformats::sequence_number_t seq_num,
                                                                    const detdataformats::DetID::Subdetector subdet)
  {
    return get_source_ids_for_subdetector(std::make_pair(rec_num, seq_num), subdet);
  }
  std::set<daqdataformats::SourceID> get_source_ids_for_subdetector(const uint64_t rec_num, // NOLINT(build/unsigned)
                                                                    const daqdataformats::sequence_number_t seq_num,
                                                                    const std::string& subdet_name)
  {
    return get_source_ids_for_subdetector(std::make_pair(rec_num, seq_num), subdet_name);
  }

#if 0
  // get SourceIDs for a system type
  std::set<daqdataformats::SourceID> get_source_ids(const daqdataformats::SourceID::Subsystem type)
  {
    return get_source_ids(get_fragment_dataset_paths(type));
  }
  std::set<daqdataformats::SourceID> get_source_ids(const std::string& typestring)
  {
    return get_source_ids(get_fragment_dataset_paths(typestring));
  }
#endif

  std::unique_ptr<char[]> get_dataset_raw_data(const std::string& dataset_path);

  std::unique_ptr<daqdataformats::Fragment> get_frag_ptr(const std::string& dataset_name);
  std::unique_ptr<daqdataformats::TriggerRecordHeader> get_trh_ptr(const std::string& dataset_name);
  std::unique_ptr<daqdataformats::TimeSliceHeader> get_tsh_ptr(const std::string& dataset_name);

  std::unique_ptr<daqdataformats::Fragment> get_frag_ptr(const record_id_t& rid,
                                                         const daqdataformats::SourceID& source_id);
  std::unique_ptr<daqdataformats::Fragment> get_frag_ptr(const uint64_t rec_num, // NOLINT(build/unsigned)
                                                         const daqdataformats::sequence_number_t seq_num,
                                                         const daqdataformats::SourceID& source_id);
  std::unique_ptr<daqdataformats::Fragment> get_frag_ptr(const record_id_t& rid,
                                                         const daqdataformats::SourceID::Subsystem type,
                                                         const uint32_t id);     // NOLINT(build/unsigned)
  std::unique_ptr<daqdataformats::Fragment> get_frag_ptr(const uint64_t rec_num, // NOLINT(build/unsigned)
                                                         const daqdataformats::sequence_number_t seq_num,
                                                         const daqdataformats::SourceID::Subsystem type,
                                                         const uint32_t id); // NOLINT(build/unsigned)
  std::unique_ptr<daqdataformats::Fragment> get_frag_ptr(const record_id_t& rid,
                                                         const std::string& typestring,
                                                         const uint32_t id);     // NOLINT(build/unsigned)
  std::unique_ptr<daqdataformats::Fragment> get_frag_ptr(const uint64_t rec_num, // NOLINT(build/unsigned)
                                                         const daqdataformats::sequence_number_t seq_num,
                                                         const std::string& typestring,
                                                         const uint32_t id); // NOLINT(build/unsigned)

  std::unique_ptr<daqdataformats::Fragment> get_frag_ptr(const record_id_t& rid,
                                                         const uint64_t geo_id); // NOLINT(build/unsigned)
  std::unique_ptr<daqdataformats::Fragment> get_frag_ptr(const uint64_t rec_num, // NOLINT(build/unsigned)
                                                         const daqdataformats::sequence_number_t seq_num,
                                                         const uint64_t geo_id); // NOLINT(build/unsigned)

  std::unique_ptr<daqdataformats::TriggerRecordHeader> get_trh_ptr(const record_id_t& rid);
  std::unique_ptr<daqdataformats::TriggerRecordHeader> get_trh_ptr(const daqdataformats::trigger_number_t trig_num,
                                                                   const daqdataformats::sequence_number_t seq_num = 0)
  {
    return get_trh_ptr(std::make_pair(trig_num, seq_num));
  }

  std::unique_ptr<daqdataformats::TimeSliceHeader> get_tsh_ptr(const record_id_t& rid);
  std::unique_ptr<daqdataformats::TimeSliceHeader> get_tsh_ptr(const daqdataformats::timeslice_number_t ts_num)
  {
    return get_tsh_ptr(std::make_pair(ts_num, 0));
  }

  daqdataformats::TriggerRecord get_trigger_record(const record_id_t& rid);
  daqdataformats::TriggerRecord get_trigger_record(const daqdataformats::trigger_number_t trig_num,
                                                   const daqdataformats::sequence_number_t seq_num = 0)
  {
    return get_trigger_record(std::make_pair(trig_num, seq_num));
  }

  daqdataformats::TimeSlice get_timeslice(const daqdataformats::timeslice_number_t ts_num);
  daqdataformats::TimeSlice get_timeslice(const record_id_t& rid) { return get_timeslice(rid.first); }

  std::vector<uint64_t> get_geo_ids_for_source_id(const record_id_t& rid, // NOLINT(build/unsigned)
                                                  const daqdataformats::SourceID& source_id);

  daqdataformats::SourceID get_source_id_for_geo_id(const record_id_t& rid,
                                                    const uint64_t geo_id); // NOLINT(build/unsigned)

  bool timeslice_already_exists(const daqdataformats::TimeSlice& timeslice) const;

private:
  HDF5RawDataFile(const HDF5RawDataFile&) = delete;
  HDF5RawDataFile& operator=(const HDF5RawDataFile&) = delete;
  HDF5RawDataFile(HDF5RawDataFile&&) = delete;
  HDF5RawDataFile& operator=(HDF5RawDataFile&&) = delete;

  // the open reader or writer (at most one of them), and its HDF5 backend
  std::unique_ptr<rawdatafilelibs::RawDataFileWriter> m_writer;
  HDF5WriterBackend* m_writer_backend{ nullptr };
  std::unique_ptr<rawdatafilelibs::RawDataFileReader> m_reader;
  HDF5ReaderBackend* m_reader_backend{ nullptr };
  bool m_allow_writing{ false }; // a file opened for reading that may be modified (and is finalized by close())

  std::unique_ptr<HDF5FileLayout> m_file_layout_ptr;
  std::string m_bare_file_name;
  std::string m_closed_file_name; // the file's name after close(); empty while open
  unsigned m_compression_level{ 0 };
  std::string m_record_type;
  daqdataformats::SourceIDGeoIDMap m_geo_id_map;
  rawdatafilelibs::FileStats m_final_stats; // the stats once the file is closed, or as read from the file

  void open_for_writing(const std::string& file_name,
                        rawdatafilelibs::FileMetadata metadata,
                        const HDF5FileLayoutParameters& fl_params,
                        const std::string& inprogress_filename_suffix,
                        bool write_run_context_attributes);

  rawdatafilelibs::FileStats get_stats() const noexcept;

  // the open HighFive file; throws FileOpenFailed after close()
  HighFive::File& get_h5_file() const;

  // the reader; throws FileOpenFailed if the file was opened for writing, or after close()
  rawdatafilelibs::RawDataFileReader& get_reader() const;
  HDF5ReaderBackend& get_reader_backend() const;

  // checking function
  void check_record_type(std::string);

  // throws RecordIDNotFound if the record is not in the file
  void throw_if_record_id_not_found(const record_id_t& rid);

  // the record's index (as stored in the file); throws RecordIDNotFound
  const daqdataformats::RecordIndex& get_index(const record_id_t& rid);

  // the GeoID map restricted to the fragments in the record; throws RecordIDNotFound
  daqdataformats::SourceIDGeoIDMap get_record_geo_id_map(const record_id_t& rid);

  // caches of full-file information
  record_id_set m_all_record_ids_in_file;
};

// HDF5RawDataFile attribute writers/getters definitions
template<typename T>
void
HDF5RawDataFile::write_attribute(std::string name, T value)
{
  if (!get_h5_file().hasAttribute(name))
    get_h5_file().createAttribute(name, value);
  else
    ers::warning(HDF5AttributeExists(ERS_HERE, name));
}

template<typename T>
void
HDF5RawDataFile::write_attribute(HighFive::Group& grp, const std::string& name, T value)
{
  if (!(grp.hasAttribute(name)))
    grp.createAttribute<T>(name, value);
  else
    ers::warning(HDF5AttributeExists(ERS_HERE, name));
}

template<typename T>
void
HDF5RawDataFile::write_attribute(HighFive::DataSet& dset, const std::string& name, T value)
{
  if (!dset.hasAttribute(name))
    dset.createAttribute<T>(name, value);
  else
    ers::warning(HDF5AttributeExists(ERS_HERE, name));
}

template<typename T>
T
HDF5RawDataFile::get_attribute(const std::string& name)
{
  if (!get_h5_file().hasAttribute(name)) {
    throw InvalidHDF5Attribute(ERS_HERE, name);
  }
  auto attr = get_h5_file().getAttribute(name);
  T value;
  attr.read(value);
  return value;
}

template<typename T>
T
HDF5RawDataFile::get_attribute_if_exists(const std::string& name, const T& default_value)
{
  if (!get_h5_file().hasAttribute(name)) {
    TLOG_DEBUG(7) << "Debug: Attribute \"" << name << "\" not found. Defaulting to " << default_value;
    return default_value;
  }
  auto attr = get_h5_file().getAttribute(name);
  T value;
  attr.read(value);
  TLOG_DEBUG(7) << "Debug: Attribute \"" << name << "\" found. Value: " << value;
  return value;
}

template<typename T>
T
HDF5RawDataFile::get_attribute(const HighFive::Group& grp, const std::string& name)
{
  if (!(grp.hasAttribute(name))) {
    throw InvalidHDF5Attribute(ERS_HERE, name);
  }
  auto attr = grp.getAttribute(name);
  T value;
  attr.read(value);
  return value;
}

template<typename T>
T
HDF5RawDataFile::get_attribute(const HighFive::DataSet& dset, std::string name)
{
  if (!dset.hasAttribute(name)) {
    throw InvalidHDF5Attribute(ERS_HERE, name);
  }
  auto attr = dset.getAttribute(name);
  T value;
  attr.read(value);
  return value;
}

} // namespace hdf5libs

} // namespace dunedaq

#endif // HDF5LIBS_INCLUDE_HDF5LIBS_HDF5RAWDATAFILE_HPP_

// Local Variables:
// c-basic-offset: 2
// End:
