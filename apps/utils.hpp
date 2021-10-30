
#include <iostream>

#include "dataformats/wib/WIBFrame.hpp"
#include "dataformats/ssp/SSPTypes.hpp"

/*
void ReadWibFrag(std::unique_ptr<dunedaq::dataformats::Fragment> frag) {
  if (frag->get_fragment_type() == dunedaq::dataformats::FragmentType::kTPCData) {
    size_t raw_data_packets = (frag->get_size() - sizeof(dunedaq::dataformats::FragmentHeader)) / sizeof(dunedaq::dataformats::WIBFrame);
    std::cout << "Fragment contains " << raw_data_packets << " WIB frames" << std::endl;
    for (size_t i=0; i < raw_data_packets; ++i) {
      auto wfptr = reinterpret_cast<dunedaq::dataformats::WIBFrame*>(frag->get_data()+i*sizeof(dunedaq::dataformats::WIBFrame));
      if (i==0) {
        std::cout << "First WIB header:"<< *(wfptr->get_wib_header());
        std::cout << "Printout sampled timestamps in WIB headers: " ;
      }
      if(i%1000 == 0) std::cout << "Timestamp " << i << ": " << wfptr->get_timestamp() << " ";
      for (int j=0; j<256; ++j) {
        //std::cout << std::to_string(wfptr->get_channel(j)) + "\n";
      }
    }
  std::cout << std::endl;
  } else {
    std::cout << "Skipping: not TPC fragment type " << std::endl;
  }
  return;
}
*/

std::vector<int> ReadWibFrag(std::unique_ptr<dunedaq::dataformats::Fragment> frag) {
  std::vector<int> wib_frames_vec;
  if (frag->get_fragment_type() == dunedaq::dataformats::FragmentType::kTPCData) {
    size_t raw_data_packets = (frag->get_size() - sizeof(dunedaq::dataformats::FragmentHeader)) / sizeof(dunedaq::dataformats::WIBFrame);
    std::cout << "Fragment contains " << raw_data_packets << " WIB frames" << std::endl;
    for (size_t i=0; i < raw_data_packets; ++i) {
      auto wfptr = reinterpret_cast<dunedaq::dataformats::WIBFrame*>(frag->get_data()+i*sizeof(dunedaq::dataformats::WIBFrame));
      if (i==0) {
        std::cout << "First WIB header:"<< *(wfptr->get_wib_header());
        std::cout << "Printout sampled timestamps in WIB headers: " ;
      }
      if(i%1000 == 0) std::cout << "Timestamp " << i << ": " << wfptr->get_timestamp() << " ";
      for (int j=0; j<256; ++j) {
        wib_frames_vec.push_back(wfptr->get_channel(j));
        //std::cout << std::to_string(wfptr->get_channel(j)) + " ";
      }
      //std::cout << std::endl;
    }
  std::cout << std::endl;
  } else {
    std::cout << "Skipping: not TPC fragment type " << std::endl;
  }
  return wib_frames_vec;
}

/*
std::vector<int> ReadSSPFrag(std::unique_ptr<dunedaq::dataformats::Fragment> frag) {
    size_t raw_data_packets = (frag->get_size() - sizeof(dunedaq::dataformats::FragmentHeader)) / sizeof(dunedaq::dataformats::DAPHNEFrame);
    std::cout << "Fragment contains " << raw_data_packets << " DAPHNE frames" << std::endl;

    // Create the vector to dump the DAPHNE frames: for each frame we have 320 channels
    std::vector<int> ssp_frames; 
    ssp_frames.reserve(320*raw_data_packets);

    // Loop through all the packets
    for (size_t i=0; i < raw_data_packets; ++i) {
      auto dfptr = reinterpret_cast<dunedaq::dataformats::DAPHNEFrame*>(frag->get_data()+i*sizeof(dunedaq::dataformats::DAPHNEFrame));
      // Print timestamp of frame
      std::cout << "Timestamp: " << dfptr->get_timestamp() << "  ";
      std::cout << std::endl;

      // Dump the DAPHNE frames inside the vector
      for (int i=0; i<320; ++i) { ssp_frames.push_back(dfptr->get_t(i)); }
      if (i == 0) std::cout << "Timestamp " << i << ": " << dfptr->get_timestamp() << " ";
    }
    std::cout << std::endl;
  return ssp_frames;
}
*/

std::vector<int> ReadSSPFrag(std::unique_ptr<dunedaq::dataformats::Fragment> frag) {
  std::vector<int> ssp_frames; 
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





    std::cout << "SSP event length: "  << ssp_event_header_ptr->length << std::endl; 
    std::cout << "Size of event channels: " <<  ssp_event_header_ptr->length - sizeof(dunedaq::dataformats::EventHeader) << std::endl ;
    std::cout << "Raw data packates without event header: " <<  raw_data_packets - sizeof(dunedaq::dataformats::EventHeader) << std::endl ;
    std::cout << "Number of SSP events: " <<  raw_data_packets /ssp_event_header_ptr->length  << std::endl ;

    unsigned int nADC=((ssp_event_header_ptr->length-sizeof(dunedaq::dataformats::EventHeader))/sizeof(unsigned int))*2;
    std::cout << "Number of ADCs: " << nADC << std::endl;
    
    unsigned short* adcPointer=reinterpret_cast<unsigned short*>(frag->get_data());
    adcPointer += sizeof(dunedaq::dataformats::EventHeader)/sizeof(unsigned short);
    unsigned short* adc; 
    for (size_t idata=0; idata< nADC; ++idata) {
      adc = adcPointer + idata;
      //std::cout << "Value of ADC: " << *adc << std::endl;
      ssp_frames.push_back(*adc);  
    }
    
   } else {
     std::cout << "Skipping: not PD fragment type" << std::endl;
   }
    
    return ssp_frames;
}





