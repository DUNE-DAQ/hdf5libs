/**
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 *
 */

#include "hdf5libs/SSPDecoder.hpp"
#include "hdf5libs/DAQDecoder.hpp"
#include "hdf5libs/utils.hpp"

namespace dunedaq {
namespace hdf5libs {

//using namespace dunedaq::detchannelmaps;
//using namespace dunedaq::hdf5libs;


//HDF5 Utility function to recursively traverse a file
// void exploreSubGroup(HighFive::Group parent_group, std::string relative_path, std::vector<std::string>& path_list){
//    std::vector<std::string> childNames = parent_group.listObjectNames();
//    for (auto& child_name : childNames) {
//      std::string full_path = relative_path + "/" + child_name;
//      HighFive::ObjectType child_type = parent_group.getObjectType(child_name);
//      if (child_type == HighFive::ObjectType::Dataset) {
//        //std::cout << "Dataset: " << child_name << std::endl;       
//        path_list.push_back(full_path);
//      } else if (child_type == HighFive::ObjectType::Group) {
//        //std::cout << "Group: " << child_name << std::endl;
//        HighFive::Group child_group = parent_group.getGroup(child_name);
//        // start the recusion
//        std::string new_path = relative_path + "/" + child_name;
//        exploreSubGroup(child_group, new_path, path_list);
//      }
//    }
// }
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
      // Module and Channel ID
      size_t module_channel_id = ssp_event_header_ptr->group2 ;
      TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << "Module and channel ID: " << module_channel_id;  
     
      // Convert module and channel id to the right SiPM element
      //unsigned int channel = ((trunc(module_channel_id/10) -1 )*4 + module_channel_id%10 -1 )*12 ;//+ trig.channel_id;
    
      // Get the timestamp 
      unsigned long ts = 0;
      for (unsigned int iword = 0 ; iword <= 3; ++iword) {
        ts += ((unsigned long)(ssp_event_header_ptr->timestamp[iword])) << 16 * iword;
      }
      TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << "Fragment timestamp: " << ts << std::endl;

      // Start parsing the waveforms included in the fragment

      unsigned int nADC=(ssp_event_header_ptr->length-sizeof(dunedaq::detdataformats::ssp::EventHeader)/sizeof(unsigned int))*2;
      TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << "Number of ADC values: " << nADC;

      // Decoding SSP data 
      unsigned short* adcPointer=reinterpret_cast<unsigned short*>(frag->get_data());
      adcPointer += sizeof(dunedaq::detdataformats::ssp::EventHeader)/sizeof(unsigned short);
      unsigned short* adc; 

      std::vector<int> ssp_frames; 
      for (size_t idata=0; idata < nADC; idata++) { 
        adc = adcPointer + idata;
        //std::cout << "Value of ADC: " << *adc << std::endl;
        ssp_frames.push_back(*adc);  
      }    
      adcPointer += nADC;    
    
      // AAA: hack to save the output ADC values
      //std::stringstream filename;
      //filename << "./SSP_data_ts_" << std::to_string(ts) << "_module_channel_"  << std::to_string(module_channel_id) << ".txt";
      //std::ofstream output_file(filename.str());
      //for (auto entry : ssp_frames) {
      //  output_file << entry << std::endl;
      //}

      // Store SSP data
      m_frag_size.push_back( frag->get_size() );
      m_frag_header_size.push_back( sizeof(dunedaq::daqdataformats::FragmentHeader) );
      m_module_channel_id.push_back( module_channel_id );
      m_frag_timestamp.push_back( ts );
      m_nADC.push_back( nADC );
      m_ssp_frames.push_back( ssp_frames );
    
    } else { // payload is empty, dropping fragment 
      dropped_fragments += 1;

      //debugging
      std::vector<int> a = {1,2,3};
      m_frag_size.push_back( frag->get_size() );
      m_frag_header_size.push_back( sizeof(dunedaq::daqdataformats::FragmentHeader) );
      m_ssp_frames.push_back( a ); 
    }  
  } else {
   std::cout << "Skipping: not PD fragment type" << std::endl;
  }

  }

}


std::vector<int> SSPDecoder::get_frag_size() {
  return m_frag_size;
}


std::vector<int> SSPDecoder::get_frag_header_size() {
  return m_frag_header_size;
}


std::vector<int> SSPDecoder::get_module_channel_id() {
  return m_module_channel_id;
}


std::vector<unsigned long> SSPDecoder::get_frag_timestamp() {
  return m_frag_timestamp;
}


std::vector<unsigned int> SSPDecoder::get_nADC() {
  return m_nADC;
}


std::vector<std::vector<int>> SSPDecoder::get_ssp_frames() {
  return m_ssp_frames;
}



} // hdf5libs
} // dunedaq
