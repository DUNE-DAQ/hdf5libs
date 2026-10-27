/**
 * @file HDF5Backend.hpp The HDF5 format for rawdatafilelibs: reader and writer backends
 *
 * These implement the rawdatafilelibs backend interfaces for DUNE DAQ HDF5 files, and are
 * available as the "hdf5" plugin, so rawdatafilelibs::open_raw_data_file("x.hdf5") uses them.
 * The on-disk layout is the one written by HDF5RawDataFile (currently file layout version 7).
 *
 * Besides the backend interface, both classes offer some HDF5-only "expert" functions (the
 * HighFive file, the file layout, dataset paths). Code that only reads or writes records
 * shouldn't need them; it should use RawDataFileReader/Writer and iterate over SourceIDs.
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef HDF5LIBS_INCLUDE_HDF5LIBS_HDF5BACKEND_HPP_
#define HDF5LIBS_INCLUDE_HDF5LIBS_HDF5BACKEND_HPP_

#include "hdf5libs/HDF5FileLayout.hpp"
#include "hdf5libs/HDF5FileLayoutParameters.hpp"

#include "rawdatafilelibs/Backend.hpp"
#include "rawdatafilelibs/FileMetadata.hpp"

#include "daqdataformats/RecordID.hpp"
#include "daqdataformats/RecordIndex.hpp"
#include "daqdataformats/SourceID.hpp"
#include "daqdataformats/SourceIDGeoIDMap.hpp"

#include <highfive/H5File.hpp>
#include <highfive/H5Group.hpp>

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace dunedaq::hdf5libs {

// The newest file layout version, which is the one written
constexpr uint32_t s_current_file_layout_version = 7; // NOLINT(build/unsigned)

// Files with a layout version at or below this have no stored per-record index, so their
// records are found by scanning dataset paths (read-only support)
constexpr uint32_t s_last_legacy_file_layout_version = 2; // NOLINT(build/unsigned)

// The standard layout for a record type, used when a writer isn't given one: "TriggerRecord"
// records with 4 sequence-number digits, or "TimeSlice" records with none.
HDF5FileLayoutParameters
default_file_layout_parameters(rawdatafilelibs::RecordType record_type);

// The paths of all datasets below a group, depth first in name order; group_path is the group's path
std::vector<std::string>
collect_dataset_paths(const HighFive::Group& group, const std::string& group_path);

/**
 * @brief Reads DUNE DAQ HDF5 files of any layout version.
 *
 * For layout versions 3 and later, each record's index comes from the SourceID maps stored
 * with the record. For versions 2 and earlier it is rebuilt by scanning the record's dataset
 * paths; those files give no fragment types or subdetectors, and their record headers get
 * the default (invalid) SourceID.
 */
class HDF5ReaderBackend : public rawdatafilelibs::ReaderBackend
{
public:
  using RecordID = daqdataformats::RecordID;
  using SourceID = daqdataformats::SourceID;

  // allow_writing opens the file read-write, for repairing files (see HDF5RawDataFile)
  explicit HDF5ReaderBackend(bool allow_writing = false);

  void open(const std::string& path) override;
  rawdatafilelibs::FileMetadata read_metadata() override;
  daqdataformats::SourceIDGeoIDMap read_geo_id_map() override;
  std::set<RecordID> read_record_ids() override;
  daqdataformats::RecordIndex read_index(const RecordID& record_id) override;
  rawdatafilelibs::Buffer read_blob(const RecordID& record_id, const SourceID& source_id) override;
  void close() override;

  // ---- HDF5-only (expert) functions ----

  HighFive::File& get_file();
  const HDF5FileLayout& get_file_layout() const;
  uint32_t get_file_layout_version() const { return get_file_layout().get_version(); } // NOLINT(build/unsigned)
  bool is_legacy_layout() const { return get_file_layout_version() <= s_last_legacy_file_layout_version; }

  // The file's record type name, from its "record_type" attribute or else its layout
  const std::string& get_record_type_name() const noexcept { return m_record_type_name; }

  // The path of the dataset that holds a record's header (header_source_id) or a fragment.
  // Throws SourceIDNotFound if the record has no such dataset.
  const std::string& get_dataset_path(const RecordID& record_id, const SourceID& source_id);

  // All dataset paths below a group (default: the whole file)
  std::vector<std::string> get_dataset_paths(const std::string& group_path = "");

  // The bytes of any dataset, by path; throws InvalidHDF5Dataset
  rawdatafilelibs::Buffer read_dataset(const std::string& dataset_path);

private:
  struct RecordContents
  {
    daqdataformats::RecordIndex index;
    std::map<SourceID, std::string> dataset_paths;
  };

  const RecordContents& load_record(const RecordID& record_id);
  RecordContents scan_legacy_record(const RecordID& record_id, const HighFive::Group& record_group);
  void check_open() const;

  bool m_allow_writing;
  std::unique_ptr<HighFive::File> m_file;
  std::unique_ptr<HDF5FileLayout> m_file_layout;
  std::string m_record_type_name;
  std::map<RecordID, RecordContents> m_records;
  bool m_warned_duplicate_source_ids{ false }; // DuplicateSourceIDInRecord is reported once per file
};

/**
 * @brief Options for HDF5WriterBackend
 */
struct HDF5WriterOptions
{
  // Group and dataset naming; default_file_layout_parameters() of the record type if unset
  std::optional<HDF5FileLayoutParameters> file_layout_parameters;

  // Write the operational_environment, offline_data_stream and run_was_for_test_purposes
  // attributes from the FileMetadata. HDF5RawDataFile's original constructor turns this off,
  // because its callers have always written (or not written) those attributes themselves.
  bool write_run_context_attributes{ true };
};

/**
 * @brief Writes DUNE DAQ HDF5 files with the current file layout version.
 *
 * The file is always newly created (HighFive::File::Excl): an existing file is never
 * reopened, appended to or overwritten.
 */
class HDF5WriterBackend : public rawdatafilelibs::WriterBackend
{
public:
  explicit HDF5WriterBackend(HDF5WriterOptions options = {});

  void open(const std::string& path,
            const rawdatafilelibs::FileMetadata& metadata,
            const daqdataformats::SourceIDGeoIDMap& geo_id_map) override;
  bool record_exists(const daqdataformats::RecordID& record_id) override;
  void write_record(const daqdataformats::RecordIndex& index,
                    const rawdatafilelibs::BlobView& header,
                    const std::vector<rawdatafilelibs::BlobView>& fragments) override;
  rawdatafilelibs::FileStats get_stats() const override { return m_stats; }
  void close(const rawdatafilelibs::FileMetadata& final_metadata) override;

  // Recovery of unfinished files (rawdatafilelibs::recover_raw_data_file()). The recalculated
  // recorded_size is the storage size of each readable dataset (after compression), as close()
  // counts it; uncompressed_raw_data_size their byte counts; total_file_size the file's size.
  // Missing values are stored with the attribute names and types of the file's layout version
  // (closing_timestamp as a string before version 6), and the recovery time as
  // "file_recovery_timestamp".
  rawdatafilelibs::RecoveryScan scan_for_recovery(const std::string& path) override;
  void store_recovery_metadata(const std::string& path,
                               const rawdatafilelibs::RecoveryScan& scan,
                               const rawdatafilelibs::FileMetadata& final_metadata) override;

  // ---- HDF5-only (expert) functions ----

  HighFive::File& get_file();
  const HDF5FileLayout& get_file_layout() const;

private:
  // Writes one dataset, creating its groups as needed; returns the dataset path and the top-level (record) group
  std::pair<std::string, HighFive::Group> write_dataset(const std::vector<std::string>& path_elements,
                                                        const void* data,
                                                        size_t size);

  HDF5WriterOptions m_options;
  unsigned m_compression_level{ 0 }; // NOLINT(build/unsigned)
  std::unique_ptr<HighFive::File> m_file;
  std::unique_ptr<HDF5FileLayout> m_file_layout;
  rawdatafilelibs::FileStats m_stats;
};

} // namespace dunedaq::hdf5libs

#endif // HDF5LIBS_INCLUDE_HDF5LIBS_HDF5BACKEND_HPP_
