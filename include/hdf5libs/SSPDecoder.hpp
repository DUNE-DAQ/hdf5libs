/**
 * @file SSPDecoder.hpp
 *
 * Class for reading out HDF5 files as 
 * 
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef HDF5LIBS_INCLUDE_SSPDECODER_HPP_
#define HDF5LIBS_INCLUDE_SSPDECODER_HPP_

// System
#include <iostream>
#include <string>

#include <vector>

// HighFive
//#include <highfive/H5File.hpp>
//#include <highfive/H5Object.hpp>

//#include "logging/Logging.hpp"

//#include "daqdataformats/TriggerRecord.hpp"

namespace dunedaq {
namespace hdf5libs {

class SSPDecoder
{

public:

  //enum 
  //{
  //  TLVL_BASIC = 2,
  //  TLVL_FILE_SIZE = 5
  //};

  SSPDecoder(const std::string& file_name, const unsigned& num_events);

  std::vector<int> get_frag_size();
  std::vector<int> get_frag_header_size();
  std::vector<int> get_module_channel_id();
  std::vector<unsigned long> get_frag_timestamp();
  std::vector<unsigned int> get_nADC();
  std::vector<std::vector<int>> get_ssp_frames();

private: 
  SSPDecoder(const SSPDecoder&) = delete;
  SSPDecoder& operator=(const SSPDecoder&) = delete;
  SSPDecoder(SSPDecoder&&) = delete;
  SSPDecoder& operator=(SSPDecoder&&) = delete;

  //std::unique_ptr<HighFive::File> m_file_ptr;
  std::string m_file_name;
  unsigned m_number_events; 
  //std::string m_top_level_group_name;

  std::vector<int> m_frag_size;
  std::vector<int> m_frag_header_size;
  std::vector<int> m_module_channel_id;
  std::vector<unsigned long> m_frag_timestamp;
  std::vector<unsigned int> m_nADC;
  std::vector<std::vector<int>> m_ssp_frames;


};

} // hdf5libs
} // dunedaq


#endif // HDF5LIBS_INCLUDE_SSPDECODER_HPP_

// Local Variables:
// c-basic-offset: 2
// End:
