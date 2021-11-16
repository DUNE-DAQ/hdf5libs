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

#ifndef HDF5LIBS_INCLUDE_DAQDECODER_HPP_
#define HDF5LIBS_INCLUDE_DAQDECODER_HPP_

// System
#include <iostream>
#include <string>
#include <variant>

// HighFive
#include <highfive/H5File.hpp>
#include <highfive/H5Object.hpp>

#include "logging/Logging.hpp"

#include "daqdataformats/TriggerRecord.hpp"

namespace dunedaq {
namespace hdf5libs {

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
  std::vector<std::string> get_fragments(const unsigned& start_tr, const unsigned& num_trs);
  std::vector<std::string> get_trh(const unsigned& start_tr, const unsigned& num_trs);
  std::map<std::string, std::variant<std::string, int>> get_attributes();

  //void read_fragment(std::string dataset_path);
  std::unique_ptr<dunedaq::daqdataformats::Fragment> get_frag_ptr(const std::string& dataset_name);
  std::unique_ptr<dunedaq::daqdataformats::TriggerRecordHeader> get_trh_ptr (const std::string& dataset_name);

private: 
  DAQDecoder(const DAQDecoder&) = delete;
  DAQDecoder& operator=(const DAQDecoder&) = delete;
  DAQDecoder(DAQDecoder&&) = delete;
  DAQDecoder& operator=(DAQDecoder&&) = delete;

  std::unique_ptr<HighFive::File> m_file_ptr;
  std::string m_file_name;
  unsigned m_number_events; 
  std::string m_top_level_group_name;


};

} // hdf5libs
} // dunedaq


#endif // HDF5LIBS_INCLUDE_DAQDECODER_HPP_

// Local Variables:
// c-basic-offset: 2
// End:
