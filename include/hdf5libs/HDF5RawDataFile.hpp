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
#include "daqdataformats/TimeSlice.hpp"
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
#include <utility>

namespace dunedaq {

ERS_DECLARE_ISSUE(hdf5libs,
                  FileOpenFailed,
                  "Issue when opening file " << file << ": " << message,
                  ((std::string)file)((std::string)message))

ERS_DECLARE_ISSUE(hdf5libs,
                  IncompatibleOpenFlags,
                  "Issue when opening file " << file << ": " 
		  << "bad open flags " << open_flags,
                  ((std::string)file)((unsigned)open_flags))

ERS_DECLARE_ISSUE(hdf5libs,
		  MissingFileLayout,
		  "No DUNEDAQ FileLayout information available."
		  << " Assigning version " << version,
		  ((uint32_t)version)) // NOLINT(build/unsigned)

ERS_DECLARE_ISSUE(hdf5libs,
		  IncompatibleFileLayoutVersion,
		  "FileLayout version incompatibility. Found version " << version
		  << " but min allowed version is " << min_allowed
		  << " and max allowed version is " << max_allowed,
		  ((uint32_t)version)((uint32_t)min_allowed)((uint32_t)max_allowed)) // NOLINT(build/unsigned)

ERS_DECLARE_ISSUE(hdf5libs,
		  BadRecordType,
		  "Record type attribute " << rt_attr
		  << " does not match file layout config record name prefix " << rt_fl,
		  ((std::string)rt_attr)((std::string)rt_fl))

ERS_DECLARE_ISSUE(hdf5libs,
		  WrongRecordTypeRequested,
		  "Record type requested " << rname
		  << " does not match file layout config record name prefix " << rt_fl,
		  ((std::string)rname)((std::string)rt_fl))

ERS_DECLARE_ISSUE(hdf5libs,
		  InvalidHDF5Group,
		  "Group " << name << " is invalid.",
		  ((std::string)name))

ERS_DECLARE_ISSUE(hdf5libs,
                  InvalidHDF5Dataset,
		  "The HDF5 Dataset associated with name \"" << data_set << "\" is invalid. (file = " << filename
		  << ")",
		  ((std::string)data_set)((std::string)filename))

ERS_DECLARE_ISSUE(hdf5libs,
		  InvalidHDF5Attribute,
		  "Attribute " << name << " not found.",
		  ((std::string)name))

ERS_DECLARE_ISSUE(hdf5libs,
		  HDF5AttributeExists,
		  "Attribute " << name << " already exists.",
		  ((std::string)name))

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
		  std::string inprogress_filename_suffix=".writing",
                  unsigned open_flags = HighFive::File::Create);

  // constructor for reading
  explicit HDF5RawDataFile(const std::string& file_name);

  ~HDF5RawDataFile();

  std::string get_file_name() const { return m_file_ptr->getName(); }

  size_t get_recorded_size() const noexcept { return m_recorded_size; }

  HDF5FileLayout get_file_layout() const { return *(m_file_layout_ptr.get()); }

  uint32_t get_version() const { return m_file_layout_ptr->get_version(); } // NOLINT(build/unsigned)

  // basic data writing methods
  void write(const daqdataformats::TriggerRecord& tr);
  void write(const daqdataformats::TriggerRecordHeader& trh);
  void write(const daqdataformats::TimeSlice& ts);
  void write(const daqdataformats::TimeSliceHeader& tsh);
  void write(const daqdataformats::Fragment& frag);

  // attribute writers/getters
  template<typename T>
  void write_attribute(std::string name, T value);
  template<typename T>
  void write_attribute(HighFive::Group& grp, const std::string& name, T value);
  template<typename T>
  void write_attribute(HighFive::DataSet& dset, const std::string& name, T value);

  template<typename T>
  T get_attribute(const std::string& name);
  template<typename T>
  T get_attribute(const HighFive::Group& grp, const std::string& name);
  template<typename T>
  T get_attribute(const HighFive::DataSet& dset, std::string name);

  std::vector<std::string> get_dataset_paths(std::string top_level_group_name = "");

  std::set<uint64_t> get_all_record_numbers(); // NOLINT(build/unsigned)
  std::set<daqdataformats::trigger_number_t> get_all_trigger_record_numbers();
  std::set<daqdataformats::timeslice_number_t> get_all_timeslice_numbers();

  std::vector<std::string> get_record_header_dataset_paths();
  std::vector<std::string> get_trigger_record_header_dataset_paths();
  std::vector<std::string> get_timeslice_header_dataset_paths();

  std::vector<std::string> get_all_fragment_dataset_paths();

  std::unique_ptr<char[]> get_dataset_raw_data(const std::string& dataset_path);

  std::unique_ptr<daqdataformats::Fragment>            get_frag_ptr(const std::string& dataset_name);
  std::unique_ptr<daqdataformats::TriggerRecordHeader> get_trh_ptr(const std::string& dataset_name);
  std::unique_ptr<daqdataformats::TimeSliceHeader>     get_tsh_ptr(const std::string& dataset_name);
  
  std::unique_ptr<daqdataformats::Fragment> get_frag_ptr(const uint64_t rec_num, // NOLINT(build/unsigned)
                                                         const daqdataformats::sequence_number_t seq_num,
                                                         const daqdataformats::GeoID element_id);
  std::unique_ptr<daqdataformats::Fragment> get_frag_ptr(const uint64_t rec_num, // NOLINT(build/unsigned)
                                                         const daqdataformats::sequence_number_t seq_num,
							 const daqdataformats::GeoID::SystemType type,
                                                         const uint16_t region_id, // NOLINT(build/unsigned)
                                                         const uint32_t element_id); // NOLINT(build/unsigned)
  std::unique_ptr<daqdataformats::Fragment> get_frag_ptr(const uint64_t rec_num, // NOLINT(build/unsigned)
                                                         const daqdataformats::sequence_number_t seq_num,
                                                         const std::string typestring,
                                                         const uint16_t region_id, // NOLINT(build/unsigned)
                                                         const uint32_t element_id); // NOLINT(build/unsigned)

  std::unique_ptr<daqdataformats::TriggerRecordHeader> get_trh_ptr(const daqdataformats::trigger_number_t trig_num,
                                                                   const daqdataformats::sequence_number_t seq_num = 0);
  std::unique_ptr<daqdataformats::TimeSliceHeader>     get_tsh_ptr(const daqdataformats::timeslice_number_t ts_num);

private:
  HDF5RawDataFile(const HDF5RawDataFile&) = delete;
  HDF5RawDataFile& operator=(const HDF5RawDataFile&) = delete;
  HDF5RawDataFile(HDF5RawDataFile&&) = delete;
  HDF5RawDataFile& operator=(HDF5RawDataFile&&) = delete;

  std::unique_ptr<HighFive::File> m_file_ptr;
  std::unique_ptr<HDF5FileLayout> m_file_layout_ptr;
  const std::string m_bare_file_name;
  const unsigned m_open_flags;

  // Total size of data being written
  size_t m_recorded_size;

  // file layout writing/reading
  void write_file_layout();
  void read_file_layout();
  void check_file_layout();

  //checking function
  void check_record_type(std::string);
  
  // writing to datasets
  size_t do_write(std::vector<std::string> const&, const char*, size_t);

  //unpacking groups when reading
  void explore_subgroup(const HighFive::Group& parent_group, std::string relative_path, std::vector<std::string>& path_list);

};


// HDF5RawDataFile attribute writers/getters definitions
template<typename T>
void HDF5RawDataFile::write_attribute(std::string name, T value)
{
  if (!m_file_ptr->hasAttribute(name))
    m_file_ptr->createAttribute(name, value);
  else
    ers::warning(HDF5AttributeExists(ERS_HERE,name));
}

template<typename T>
void HDF5RawDataFile::write_attribute(HighFive::Group& grp, const std::string& name, T value)
{
  if (!(grp.hasAttribute(name)))
    grp.createAttribute<T>(name, value);
  else
    ers::warning(HDF5AttributeExists(ERS_HERE,name));
}
  
template<typename T>
void HDF5RawDataFile::write_attribute(HighFive::DataSet& dset, const std::string& name, T value)
{
  if (!dset.hasAttribute(name))
    dset.createAttribute<T>(name, value);
  else
    ers::warning(HDF5AttributeExists(ERS_HERE,name));

}
template<typename T>
T HDF5RawDataFile::get_attribute(const std::string& name)
{
  if (!m_file_ptr->hasAttribute(name)) {
    throw InvalidHDF5Attribute(ERS_HERE,name);
  }
  auto attr = m_file_ptr->getAttribute(name);
  T value;
  attr.read(value);
  return value;
}

template<typename T>
T HDF5RawDataFile::get_attribute(const HighFive::Group& grp, const std::string& name)
{
  if (!(grp.hasAttribute(name))) {
    throw InvalidHDF5Attribute(ERS_HERE,name);
  }
  auto attr = grp.getAttribute(name);
  T value;
  attr.read(value);
  return value;
}

template<typename T>
T HDF5RawDataFile::get_attribute(const HighFive::DataSet& dset, std::string name)
{
  if (!dset.hasAttribute(name)) {
    throw InvalidHDF5Attribute(ERS_HERE,name);
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
