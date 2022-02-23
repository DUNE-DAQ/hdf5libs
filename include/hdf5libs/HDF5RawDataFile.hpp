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
#include "hdf5libs/HDF5FileLayout.hpp"
//#include "hdf5libs/HDF5KeyTranslator.hpp"

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
                  const nlohmann::json& fl_params_conf,
                  unsigned open_flags = HighFive::File::Create);

  // constructor for reading
  explicit HDF5RawDataFile(const std::string& file_name);

  // destructor
  ~HDF5RawDataFile();

  std::string get_file_name() const { return m_file_ptr->getName(); }

  size_t get_recorded_size() const { return m_recorded_size; }

  // basic data writing methods
  void write(const daqdataformats::TriggerRecord& tr);
  void write(const daqdataformats::TriggerRecordHeader& trh);
  void write(const daqdataformats::Fragment& frag);

  // attribute writers/getters
  template<typename T>
  void write_attribute(std::string name, T value);
  template<typename T>
  void write_attribute(HighFive::Group* grp_ptr, std::string name, T value);
  template<typename T>
  void write_attribute(HighFive::DataSet* d_ptr, std::string name, T value);

  template<typename T>
  T get_attribute(std::string name);
  template<typename T>
  T get_attribute(HighFive::Group* grp_ptr, std::string name);
  template<typename T>
  T get_attribute(HighFive::DataSet* d_ptr, std::string name);

  std::vector<std::string> get_datasets();
  std::vector<std::string> get_fragments(const unsigned& start_tr, const unsigned& num_trs);
  std::vector<std::string> get_trh(const unsigned& start_tr, const unsigned& num_trs);
  std::map<std::string, std::variant<std::string, int>> get_attributes();

  // void read_fragment(std::string dataset_path);
  std::unique_ptr<daqdataformats::Fragment> get_frag_ptr(const std::string& dataset_name);
  std::unique_ptr<daqdataformats::TriggerRecordHeader> get_trh_ptr(const std::string& dataset_name);

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

  // helper functions for the path elements
  std::vector<std::string> get_path_elements(const daqdataformats::TriggerRecordHeader& trh);
  std::vector<std::string> get_path_elements(const daqdataformats::FragmentHeader& fh);
  std::string get_trigger_number_string(daqdataformats::trigger_number_t, daqdataformats::sequence_number_t);
};

} // namespace hdf5libs
} // namespace dunedaq

#endif // HDF5LIBS_INCLUDE_HDF5LIBS_HDF5RAWDATAFILE_HPP_

// Local Variables:
// c-basic-offset: 2
// End:
