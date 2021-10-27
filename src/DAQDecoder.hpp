/**
 * @file DAQDecoder.hpp
 *
 * Class for reading out HDF5 files as 
 * 
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef DAQDECODER_HPP_
#define DAQDECODER_HPP_

// System
#include <iostream>
#include <string>

// HighFive
#include <highfive/H5File.hpp>
#include <highfive/H5Object.hpp>

#include "logging/Logging.hpp"

#include "hdf5libs/StorageKey.hpp"
#include "hdf5libs/StorageKeyList.hpp"


class DAQDecoder
{

public:

  enum 
  {
    TLVL_BASIC = 2,
    TLVL_FILE_SIZE = 5
  };

  DAQDecoder(const std::string& file_name, const unsigned& num_events);

  std::vector<std::string> get_datasets();
  std::vector<std::string> get_fragments(const unsigned& num_trs);
  std::vector<std::string> get_trh(const unsigned& num_trs);
  void read_fragment(std::string dataset_path);

  dunedaq::hdf5libs::StorageKeyList get_all_storage_keys()
  { 
    if(m_storage_keys.empty()) fill_storage_keys();
    return m_storage_keys;
  }

private: 
  DAQDecoder(const DAQDecoder&) = delete;
  DAQDecoder& operator=(const DAQDecoder&) = delete;
  DAQDecoder(DAQDecoder&&) = delete;
  DAQDecoder& operator=(DAQDecoder&&) = delete;

  std::unique_ptr<HighFive::File> m_file_ptr;
  std::string m_file_name;
  unsigned m_number_events; 
  std::string m_top_level_group_name;

  dunedaq::hdf5libs::StorageKeyList m_storage_keys;

  //fill the internal list of storage keys
  void fill_storage_keys();

  dunedaq::hdf5libs::StorageKey make_key_from_path(std::string const&);
};



#endif // DAQDECODER_HPP_

// Local Variables:
// c-basic-offset: 2
// End:
