/**
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 *
 */

#include "hdf5libs/SSPDecoder.hpp"
#include "hdf5libs/DAQDecoder.hpp"
#include "detdataformats/ssp/SSPTypes.hpp"

namespace dunedaq {
namespace hdf5libs {

enum
{
  TLVL_ENTER_EXIT_METHODS = 5,
  TLVL_CONFIG = 7,
  TLVL_WORK_STEPS = 10,
  TLVL_SEQNO_MAP_CONTENTS = 13,
  TLVL_FRAGMENT_HEADER_DUMP = 17
};

SSPDecoder::SSPDecoder(const std::string& file_name, const unsigned& num_events) {

  m_file_name = file_name; 
  m_number_events = num_events;
  
  DAQDecoder decoder = DAQDecoder(file_name, num_events);
  std::vector<std::string> datasets_path = decoder.get_fragments(num_events);

  // Read all the fragments
  int dropped_fragments = 0;
  int fragment_counter = 0; 
  for (auto& element : datasets_path) {
    fragment_counter += 1;
    std::unique_ptr<dunedaq::daqdataformats::Fragment> frag = decoder.get_frag_ptr(element);
    
    if (frag->get_fragment_type() == dunedaq::daqdataformats::FragmentType::kPDSData) {
 
    TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << "Fragment size: " << frag->get_size();  
    TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << "Fragment header size: " << sizeof(dunedaq::daqdataformats::FragmentHeader);  

    // If the fragment is not empty (i.e. greater than the header size)
    if (frag->get_size() > sizeof(dunedaq::daqdataformats::FragmentHeader) ) {
      // Ptr to the SSP data
      auto ssp_event_header_ptr = reinterpret_cast<dunedaq::detdataformats::ssp::EventHeader*>(frag->get_data()); 
      
      // Parse header info
      // Module and Channel ID
      size_t module_channel_id = ssp_event_header_ptr->group2 ;
      TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << "Module and channel ID: " << module_channel_id;  
      unsigned short module_id = ((module_channel_id & 0xFFF0) >> 4);
      unsigned short channel_id = ((module_channel_id & 0x000F) >> 0);
 
      // Get the timestamp 
      unsigned long ts = 0;
      for (unsigned int iword = 0 ; iword <= 3; ++iword) {
        ts += ((unsigned long)(ssp_event_header_ptr->timestamp[iword])) << 16 * iword;
      }
      TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << "Fragment timestamp: " << ts << std::endl;

      uint32_t peaksum = ((ssp_event_header_ptr->group3 & 0x00FF) >> 16) + ssp_event_header_ptr->peakSumLow;  // peak sum
      if(peaksum & 0x00800000) {
        peaksum |= 0xFF000000;
      }
      unsigned short peaktime = ((ssp_event_header_ptr->group3 & 0xFF00) >> 8);                                  // peak time
      unsigned int prerise = ((ssp_event_header_ptr->group4 & 0x00FF) << 16) + ssp_event_header_ptr->preriseLow;          // prerise
      unsigned int intsum = ((unsigned int)(ssp_event_header_ptr->intSumHigh) << 8) + (((unsigned int)(ssp_event_header_ptr->group4) & 0xFF00) >> 8);  // integrated sum
      unsigned long baseline = ssp_event_header_ptr->baseline;                                                  // baseline
      unsigned long baselinesum = ((ssp_event_header_ptr->group4 & 0x00FF) << 16) + ssp_event_header_ptr->preriseLow;      // baselinesum
      //std::vector<unsigned long> cfd_interpol;
      //for(unsigned int i_cfdi = 0; i_cfdi < 4; i_cfdi++)                                   // CFD timestamp interpolation points
      //  cfd_interpol.push_back(ssp_event_header_ptr->cfdPoint[i_cfdi]);
      unsigned long internal_interpol = ssp_event_header_ptr->intTimestamp[0];                                  // internal interpolation point
      uint64_t internal_ts = ((uint64_t)((uint64_t)ssp_event_header_ptr->intTimestamp[3] << 32)) + ((uint64_t)((uint64_t)ssp_event_header_ptr->intTimestamp[2]) << 16) + ((uint64_t)((uint64_t)ssp_event_header_ptr->intTimestamp[1]));  // internal timestamp


      // Start parsing the waveforms included in the fragment

      unsigned int nADC=(ssp_event_header_ptr->length)/2-sizeof(dunedaq::detdataformats::ssp::EventHeader)/sizeof(unsigned short);
      TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << "Number of ADC values: " << nADC;

      // Decoding SSP data 
      unsigned short* adcPointer=reinterpret_cast<unsigned short*>(frag->get_data());
      adcPointer += sizeof(dunedaq::detdataformats::ssp::EventHeader)/sizeof(unsigned short);
      unsigned short* adc; 

      std::vector<int> ssp_frames; 
      for (size_t idata=0; idata < nADC; idata++) { 
        adc = adcPointer + idata;
        ssp_frames.push_back(*adc);  
      }    
      adcPointer += nADC;    
    
      // Store SSP data
      m_module_id.push_back( module_id );
      m_channel_id.push_back( channel_id );      
      m_frag_timestamp.push_back( ts );
      m_ssp_frames.push_back( ssp_frames );
      m_peaksum.push_back( peaksum );
      m_peaktime.push_back( peaktime );
      m_prerise.push_back( prerise );
      m_intsum.push_back( intsum );
      m_baseline.push_back( baseline );
      m_baselinesum.push_back( baselinesum );
      //m_cfd_interpol.push_back( cfd_interpol );
      m_internal_interpol.push_back( internal_interpol );
      m_internal_ts.push_back( internal_ts );

    } else { // payload is empty, dropping fragment 
      dropped_fragments += 1;

    }  
  } else {
   std::cout << "Skipping: not PD fragment type" << std::endl;
  }

  }

}

// Property getter functions
// 

std::vector<int> SSPDecoder::get_module_id() {
  return m_module_id;
}


std::vector<int> SSPDecoder::get_channel_id() {
  return m_channel_id;
}


std::vector<unsigned long> SSPDecoder::get_frag_timestamp() {
  return m_frag_timestamp;
}


std::vector<std::vector<int>> SSPDecoder::get_ssp_frames() {
  return m_ssp_frames;
}


std::vector<uint32_t> SSPDecoder::get_peaksum() {
  return m_peaksum;
}


std::vector<short> SSPDecoder::get_peaktime() {
  return m_peaktime;
}

std::vector<int> SSPDecoder::get_prerise() {
  return m_prerise;
}

std::vector<int> SSPDecoder::get_intsum() {
  return m_intsum;
}

std::vector<long> SSPDecoder::get_baseline() {
  return m_baseline;
}

std::vector<long> SSPDecoder::get_baselinesum() {
  return m_baselinesum;
}

//std::vector<std::vector<long>> SSPDecoder::get_cfd_interpol() {
//  return m_cfd_interpol;
//}

std::vector<long> SSPDecoder::get_internal_interpol() {
  return m_internal_interpol;
}

std::vector<uint64_t> SSPDecoder::get_internal_ts() {
  return m_internal_ts;
}

} // hdf5libs
} // dunedaq
