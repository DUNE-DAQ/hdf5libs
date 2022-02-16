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

#ifndef HDF5LIBS_INCLUDE_HDF5RAWDATAFILE_HPP_
#define HDF5LIBS_INCLUDE_HDF5RAWDATAFILE_HPP_

// System
#include <iostream>
#include <string>
#include <variant>
#include <filesystem>
#include <sys/statvfs.h>

// HighFive
#include <highfive/H5File.hpp>
#include <highfive/H5Object.hpp>
#include <highfive/H5Group.hpp>
#include <highfive/H5DataSet.hpp>


// DUNE-DAQ
#include "logging/Logging.hpp"
#include "daqdataformats/Fragment.hpp"
#include "daqdataformats/TriggerRecord.hpp"
#include "nlohmann/json.hpp"
#include "hdf5libs/StorageKey.hpp"
#include "hdf5libs/HDF5FileHandle.hpp"
#include "hdf5libs/HDF5FileLayout.hpp"
//#include "hdf5libs/HDF5KeyTranslator.hpp"


namespace dunedaq {
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

  //constructor for writing
  HDF5RawDataFile(const nlohmann::json& conf);

  //constructor for reading
  //HDF5RawDataFile(const string& file_name) {};

  //basic data writing methods
  void write(const daqdataformats::TriggerRecord& tr);
  void write(const daqdataformats::TriggerRecordHeader& trh);
  void write(const daqdataformats::Fragment& frag);
  size_t do_write(std::vector<std::string> const&,
		  const char*, size_t);

  //attribute writers/getters
  template<typename T>
  void write_attribute(std::string name, T value);
  template<typename T>
  void write_attribute(HighFive::Group* grp_ptr,std::string name, T value);
  template<typename T>
  void write_attribute(HighFive::DataSet* d_ptr,std::string name, T value);

  template<typename T>
  T get_attribute(std::string name);
  template<typename T>
  T get_attribute(HighFive::Group* grp_ptr,std::string name);
  template<typename T>
  T get_attribute(HighFive::DataSet* d_ptr,std::string name);

  //file layour writing/reading
  void write_file_layout();
  void read_file_layout();


  //file handling stuff that may go away ...
  void open_file_if_needed(const std::string& file_name, unsigned open_flags);  
  void increment_file_index_if_needed(size_t size_of_next_write);


  //writing stuff that may be outdated soon ...
  std::vector<std::string> get_path_elements(const StorageKey& data_key); 
  void do_write(const KeyedDataBlock& data_block);
  void write(const KeyedDataBlock& data_block);
  void write(const std::vector<KeyedDataBlock>& data_block_list);


  std::vector<std::string> get_datasets();
  std::vector<std::string> get_fragments(const unsigned& start_tr, const unsigned& num_trs);
  std::vector<std::string> get_trh(const unsigned& start_tr, const unsigned& num_trs);
  std::map<std::string, std::variant<std::string, int>> get_attributes();

  //void read_fragment(std::string dataset_path);
  std::unique_ptr<daqdataformats::Fragment> get_frag_ptr(const std::string& dataset_name);
  std::unique_ptr<daqdataformats::TriggerRecordHeader> get_trh_ptr (const std::string& dataset_name);

private: 
  HDF5RawDataFile(const HDF5RawDataFile&) = delete;
  HDF5RawDataFile& operator=(const HDF5RawDataFile&) = delete;
  HDF5RawDataFile(HDF5RawDataFile&&) = delete;
  HDF5RawDataFile& operator=(HDF5RawDataFile&&) = delete;

  std::unique_ptr<HDF5FileHandle> m_file_handle;
  std::unique_ptr<HighFive::File> m_file_ptr;
  std::string m_file_name;
  std::string m_top_level_group_name;

  std::string m_basic_name_of_open_file;
  unsigned m_open_flags_of_open_file;
  daqdataformats::run_number_t m_run_number;
  std::string m_application_name;

  std::unique_ptr<HDF5FileLayout> m_file_layout_ptr;

  // Total number of generated files
  size_t m_file_index;

  // Total size of data being written
  size_t m_recorded_size;

  // Configuration
  std::string m_operation_mode;
  std::string m_path;
  size_t m_max_file_size;
  bool m_disable_unique_suffix;
  float m_free_space_safety_factor_for_write;

  //std::unique_ptr<HDF5KeyTranslator> m_key_translator_ptr;


  std::vector<std::string> get_path_elements(const daqdataformats::TriggerRecordHeader& trh);
  std::vector<std::string> get_path_elements(const daqdataformats::FragmentHeader& fh);
  std::string get_trigger_number_string(daqdataformats::trigger_number_t,
					daqdataformats::sequence_number_t);


};













} // hdf5libs
} // dunedaq


#endif // HDF5LIBS_INCLUDE_DAQDECODER_HPP_

// Local Variables:
// c-basic-offset: 2
// End:
