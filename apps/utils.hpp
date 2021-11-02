
#include <iostream>

#include "dataformats/wib/WIBFrame.hpp"
#include "dataformats/ssp/SSPTypes.hpp"





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




void ReadWibFrag(std::unique_ptr<dunedaq::dataformats::Fragment> frag) {
  if (frag->get_fragment_type() == dunedaq::dataformats::FragmentType::kTPCData) {
    size_t raw_data_packets = (frag->get_size() - sizeof(dunedaq::dataformats::FragmentHeader)) / sizeof(dunedaq::dataformats::WIBFrame);
    std::cout << "Fragment contains " << raw_data_packets << " WIB frames" << std::endl;

    size_t n_blocks = 4;
    size_t n_channels = 64;

    std::map <size_t, std::vector<uint16_t> > ch_adcs_map;

    for (size_t i=0; i < raw_data_packets; ++i) {
      auto wfptr = reinterpret_cast<dunedaq::dataformats::WIBFrame*>(frag->get_data()+i*sizeof(dunedaq::dataformats::WIBFrame));
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




void ReadSSPFrag(std::unique_ptr<dunedaq::dataformats::Fragment> frag) {
  if (frag->get_fragment_type() == dunedaq::dataformats::FragmentType::kPDSData) {
    auto ssp_event_header_ptr = reinterpret_cast<dunedaq::dataformats::EventHeader*>(frag->get_data()); 
    // std::cout << ssp_event_header_ptr->group2 << std::endl; // Module ID, Channel ID
   
    // Get the timestamp 
    unsigned long ts = 0;
    for (unsigned int iword = 0 ; iword <= 3; ++iword) {
      ts += ((unsigned long)(ssp_event_header_ptr->timestamp[iword])) << 16 * iword;
    }
    std::cout << "Timestamp: " << ts << std::endl;

    // Start parsing the waveforms
    size_t raw_data_packets = (frag->get_size() - sizeof(dunedaq::dataformats::FragmentHeader) );
    std::cout << "Raw data packets: " << raw_data_packets << std::endl; 


    /*
    std::cout << "SSP event length: "  << ssp_event_header_ptr->length << std::endl; 
    std::cout << "Size of event channels: " <<  ssp_event_header_ptr->length - sizeof(dunedaq::dataformats::EventHeader) << std::endl ;
    std::cout << "Raw data packates without event header: " <<  raw_data_packets - sizeof(dunedaq::dataformats::EventHeader) << std::endl ;
    std::cout << "Number of SSP events: " <<  raw_data_packets /ssp_event_header_ptr->length  << std::endl ;
    */
    unsigned int nADC=((ssp_event_header_ptr->length-sizeof(dunedaq::dataformats::EventHeader))/sizeof(unsigned int))*2;
    std::cout << "Number of ADCs: " << nADC << std::endl;
    
    unsigned short* adcPointer=reinterpret_cast<unsigned short*>(frag->get_data());
    adcPointer += sizeof(dunedaq::dataformats::EventHeader)/sizeof(unsigned short);
    unsigned short* adc; 
    std::vector<int> ssp_frames; 
    for (size_t idata=0; idata< nADC; ++idata) {
      adc = adcPointer + idata;
      //std::cout << "Value of ADC: " << *adc << std::endl;
      ssp_frames.push_back(*adc);  
    }

    std::stringstream filename;
    filename << "./SSP_data_" << std::to_string(ts) << ".txt";
    std::ofstream output_file(filename.str());
    for (auto entry : ssp_frames) {
      output_file << entry << std::endl;
    } 
  } else {
   std::cout << "Skipping: not PD fragment type" << std::endl;
  }
    
}





