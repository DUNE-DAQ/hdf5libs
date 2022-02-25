/**
 * @file HDF5RawDataFile.hpp
 *
 * Class for reading out HDF5 files as
 *
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef HDF5LIBS_INCLUDE_HDF5LIBS_HDF5RAWDATAFILE_HPP_
#define HDF5LIBS_INCLUDE_HDF5LIBS_HDF5RAWDATAFILE_HPP_

// DUNE-DAQ
#include "hdf5libs/hdf5filelayout/Structs.hpp"
#include "hdf5libs/HDF5FileLayout.hpp"

#include "daqdataformats/Fragment.hpp"
#include "daqdataformats/TriggerRecord.hpp"
#include "logging/Logging.hpp"

// External Packages
#include <highfive/H5DataSet.hpp>
#include <highfive/H5File.hpp>
#include <highfive/H5Group.hpp>
#include <highfive/H5Object.hpp>
#include <nlohmann/json.hpp>

// System
#include <filesystem>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <sys/statvfs.h>
#include <variant>
#include <vector>

namespace dunedaq {

ERS_DECLARE_ISSUE(hdf5libs,
                  FileOpenFailed,
                  "Issue when opening file " << file << ": " << message,
                  ((std::string)file)((std::string)message))

namespace hdf5libs {

/**
 * @brief HDF5RawDataFile is the class responsible
 * for interfacing the DAQ format with the HDF5 file format.
 */
class HDF5RawDataFile
{

public:
  enum
  {
    TLVL_BASIC = 2,
    TLVL_FILE_SIZE = 5
  };

  // constructor for writing
  HDF5RawDataFile(std::string file_name,
                  daqdataformats::run_number_t run_number,
                  size_t file_index,
                  std::string application_name,
                  const hdf5filelayout::FileLayoutParams& fl_params,
                  unsigned open_flags = HighFive::File::Create);
  HDF5RawDataFile(std::string file_name,
                  daqdataformats::run_number_t run_number,
                  size_t file_index,
                  std::string application_name,
                  const nlohmann::json& fl_params_conf,
                  unsigned open_flags = HighFive::File::Create);

  // constructor for reading
  explicit HDF5RawDataFile(const std::string& file_name);

  ~HDF5RawDataFile();

  std::string get_file_name() const { return m_file_ptr->getName(); }

  size_t get_recorded_size() const noexcept { return m_recorded_size; }

  HDF5FileLayout get_file_layout() const { return *(m_file_layout_ptr.get()); }

  // basic data writing methods
  void write(const daqdataformats::TriggerRecord& tr);
  void write(const daqdataformats::TriggerRecordHeader& trh);
  void write(const daqdataformats::Fragment& frag);

  // attribute writers/getters
  template<typename T>
  void write_attribute(std::string name, T value)
  {
    if (!m_file_ptr->hasAttribute(name))
      m_file_ptr->createAttribute(name, value);
  }
  template<typename T>
  void write_attribute(HighFive::Group& grp, const std::string& name, T value)
  {
    if (!(grp.hasAttribute(name))) {
      grp.createAttribute<T>(name, value);
    }
  }
  template<typename T>
  void write_attribute(HighFive::DataSet& dset, const std::string& name, T value)
  {
    if (!dset.hasAttribute(name)) {
      dset.createAttribute<T>(name, value);
    }
  }

  template<typename T>
  T get_attribute(const std::string& name)
  {
    if (!m_file_ptr->hasAttribute(name)) {
      // throw that we don't have that attribute
      throw "Placeholder for yet-to-be-implemented exception";
    }
    auto attr = m_file_ptr->getAttribute(name);
    T value;
    attr.read(value);
    return value;
  }
  template<typename T>
  T get_attribute(const HighFive::Group& grp, const std::string& name)
  {
    if (!(grp.hasAttribute(name))) {
      // throw that we don't have that attribute
      throw "Placeholder for yet-to-be-implemented exception";
    }
    auto attr = grp.getAttribute(name);
    T value;
    attr.read(value);
    return value;
  }

  template<typename T>
  T get_attribute(const HighFive::DataSet& dset, std::string name)
  {
    if (!dset.hasAttribute(name)) {
      throw "Placeholder for yet-to-be-implemented exception";
    }
    auto attr = dset.getAttribute(name);
    T value;
    attr.read(value);
    return value;
  }

  void explore_subgroup(const HighFive::Group& parent_group, std::string relative_path, std::vector<std::string>& path_list);

  std::vector<std::string> get_dataset_paths(std::string top_level_group_name = "");
  std::set<daqdataformats::trigger_number_t> get_all_record_numbers();
  std::set<daqdataformats::trigger_number_t> get_all_trigger_record_numbers();

  std::vector<std::string> get_trigger_record_header_dataset_paths(int max_trigger_records = -1);
  std::vector<std::string> get_all_fragment_dataset_paths(int max_trigger_records = -1);

  std::unique_ptr<char[]> get_dataset_raw_data(const std::string& dataset_path);

  std::unique_ptr<daqdataformats::Fragment> get_frag_ptr(const std::string& dataset_name);
  std::unique_ptr<daqdataformats::Fragment> get_frag_ptr(const daqdataformats::trigger_number_t trig_num,
                                                         const daqdataformats::GeoID element_id,
                                                         const daqdataformats::sequence_number_t seq_num = 0);
  std::unique_ptr<daqdataformats::Fragment> get_frag_ptr(const daqdataformats::trigger_number_t trig_num,
                                                         const daqdataformats::GeoID::SystemType type,
                                                         const uint16_t region_id, // NOLINT(build/unsigned)
                                                         const uint32_t element_id, // NOLINT(build/unsigned)
                                                         const daqdataformats::sequence_number_t seq_num = 0);
  std::unique_ptr<daqdataformats::Fragment> get_frag_ptr(const daqdataformats::trigger_number_t trig_num,
                                                         const std::string typestring,
                                                         const uint16_t region_id, // NOLINT(build/unsigned)
                                                         const uint32_t element_id, // NOLINT(build/unsigned)
                                                         const daqdataformats::sequence_number_t seq_num = 0);

  std::unique_ptr<daqdataformats::TriggerRecordHeader> get_trh_ptr(const std::string& dataset_name);
  std::unique_ptr<daqdataformats::TriggerRecordHeader> get_trh_ptr(const daqdataformats::trigger_number_t trig_num,
                                                                   const daqdataformats::sequence_number_t seq_num = 0);

private:
  HDF5RawDataFile(const HDF5RawDataFile&) = delete;
  HDF5RawDataFile& operator=(const HDF5RawDataFile&) = delete;
  HDF5RawDataFile(HDF5RawDataFile&&) = delete;
  HDF5RawDataFile& operator=(HDF5RawDataFile&&) = delete;

  std::unique_ptr<HighFive::File> m_file_ptr;
  std::unique_ptr<HDF5FileLayout> m_file_layout_ptr;
  const unsigned m_open_flags;

  // Total size of data being written
  size_t m_recorded_size;

  // file layout writing/reading
  void write_file_layout();
  void read_file_layout();

  // writing to datasets
  size_t do_write(std::vector<std::string> const&, const char*, size_t);
};

} // namespace hdf5libs
} // namespace dunedaq

#endif // HDF5LIBS_INCLUDE_HDF5LIBS_HDF5RAWDATAFILE_HPP_

// Local Variables:
// c-basic-offset: 2
// End:
