
#include <iostream>

#include "detdataformats/wib/WIBFrame.hpp"
#include "detdataformats/ssp/SSPTypes.hpp"



using namespace dunedaq::hdf5libs;

enum
{
  TLVL_ENTER_EXIT_METHODS = 5,
  TLVL_CONFIG = 7,
  TLVL_WORK_STEPS = 10,
  TLVL_SEQNO_MAP_CONTENTS = 13,
  TLVL_FRAGMENT_HEADER_DUMP = 17
};



void rmsValue(std::vector<uint16_t> adcs, float &mean, float &rms, float &stddev) {
  uint64_t square = 0;
  uint64_t devsquare = 0;
  uint64_t sum = 0;
  mean = 0.0;
  rms = 0.0; 
  stddev = 0.0;

  // Calculate square.
  for (size_t i = 0; i < adcs.size(); i++) {
    square += pow(adcs[i], 2);
    sum += adcs[i]; 
  }
  mean = sum/float(adcs.size());
  rms = sqrt(square/(float)(adcs.size()));
  for (size_t i = 0; i < adcs.size(); i++) {
    devsquare += pow((adcs[i]-mean), 2);
  }
                  
  stddev = sqrt(devsquare / (float)(adcs.size()-1));
                                
  return;
}




void ReadWibFrag(std::unique_ptr<dunedaq::daqdataformats::Fragment> frag) {
  if (frag->get_fragment_type() == dunedaq::daqdataformats::FragmentType::kTPCData) {
    size_t raw_data_packets = (frag->get_size() - sizeof(dunedaq::daqdataformats::FragmentHeader)) / sizeof(dunedaq::detdataformats::WIBFrame);
    std::cout << "Fragment contains " << raw_data_packets << " WIB frames" << std::endl;

    size_t n_blocks = 4;
    size_t n_channels = 64;

    std::map <size_t, std::vector<uint16_t> > ch_adcs_map;

    for (size_t i=0; i < raw_data_packets; ++i) {
      auto wfptr = reinterpret_cast<dunedaq::detdataformats::WIBFrame*>(frag->get_data()+i*sizeof(dunedaq::detdataformats::WIBFrame));
      for (size_t k=0 ; k < n_blocks; ++k) {
        for (size_t j=0; j < n_channels; ++j) {
          ch_adcs_map[k*64+j].push_back(wfptr->get_channel(k,j));
        }
      }
    }
 
    std::cout << "Arrays filled for link " << frag->get_element_id().element_id << std::endl;
    std::stringstream filename;
    filename << "./Link_" << frag->get_element_id().element_id << ".txt";
    std::ofstream output_file(filename.str());
    float mean,rms, stddev;
    
    std::cout << "Channel Mean RMS STDEV" << std::endl;
    for (size_t k=0 ; k < n_blocks*n_channels; ++k) {
      rmsValue(ch_adcs_map[k], mean, rms, stddev);
      output_file << k << " " << mean << " " << rms << " " << stddev << std::endl;
      std::cout << k << " " << mean << " " << rms << " " << stddev << std::endl;
    }

  } else {
   std::cout << "Skipping: not TPC fragment type " << std::endl;
  }

}



void ReadSSPFrag(std::unique_ptr<dunedaq::daqdataformats::Fragment> frag, int& dropped_fragments_per_event) {
  if (frag->get_fragment_type() == dunedaq::daqdataformats::FragmentType::kPDSData) {
 
    TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << "Fragment size: " << frag->get_size();  
    TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << "Fragment header size: " << sizeof(dunedaq::daqdataformats::FragmentHeader);  

    // If the fragment is not empty (i.e. greater than the header size)
    if (frag->get_size() > sizeof(dunedaq::daqdataformats::FragmentHeader) ) {
      // Ptr to the SSP data
      auto ssp_event_header_ptr = reinterpret_cast<dunedaq::detdataformats::EventHeader*>(frag->get_data()); 
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

      unsigned int nADC=(ssp_event_header_ptr->length-sizeof(dunedaq::detdataformats::EventHeader)/sizeof(unsigned int))*2;
      TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << "Number of ADC values: " << nADC;

      // Decoding SSP data 
      unsigned short* adcPointer=reinterpret_cast<unsigned short*>(frag->get_data());
      adcPointer += sizeof(dunedaq::detdataformats::EventHeader)/sizeof(unsigned short);
      unsigned short* adc; 

      std::vector<int> ssp_frames; 
      for (size_t idata=0; idata < nADC; idata++) { 
        adc = adcPointer + idata;
        //std::cout << "Value of ADC: " << *adc << std::endl;
        ssp_frames.push_back(*adc);  
      }    
      adcPointer += nADC;    
    
      // AAA: hack to save the output ADC values
      std::stringstream filename;
      filename << "./SSP_data_ts_" << std::to_string(ts) << "_module_channel_"  << std::to_string(module_channel_id) << ".txt";
      std::ofstream output_file(filename.str());
      for (auto entry : ssp_frames) {
        output_file << entry << std::endl;
      }
    
    } else { // payload is empty, dropped fragment 
      dropped_fragments_per_event += 1;
    }  
  } else {
   std::cout << "Skipping: not PD fragment type" << std::endl;
  }
    
}





