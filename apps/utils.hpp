
#include <iostream>

#include "dataformats/wib/WIBFrame.hpp"
#include "dataformats/daphne/DAPHNEFrame.hpp"


void ReadWibFrag(std::unique_ptr<dunedaq::dataformats::Fragment> frag) {
  size_t raw_data_packets = (frag->get_size() - sizeof(dunedaq::dataformats::FragmentHeader)) / sizeof(dunedaq::dataformats::WIBFrame);
  std::cout << "Fragment contains " << raw_data_packets << " WIB frames" << std::endl;
  for (size_t i=0; i < raw_data_packets; ++i) {
    auto wfptr = reinterpret_cast<dunedaq::dataformats::WIBFrame*>(frag->get_data()+i*sizeof(dunedaq::dataformats::WIBFrame));
    if (i==0) {
      std::cout << "First WIB header:"<< *(wfptr->get_wib_header());
      std::cout << "Printout sampled timestamps in WIB headers: " ;
    }
    if(i%1000 == 0) std::cout << "Timestamp " << i << ": " << wfptr->get_timestamp() << " ";
  }
  std::cout << std::endl;
  return;
}


void ReadSSPFrag(std::unique_ptr<dunedaq::dataformats::Fragment> frag) {
    size_t raw_data_packets = (frag->get_size() - sizeof(dunedaq::dataformats::FragmentHeader)) / sizeof(dunedaq::dataformats::DAPHNEFrame);
    std::cout << "Fragment contains " << raw_data_packets << " DAPHNE frames" << std::endl;
    for (size_t i=0; i < raw_data_packets; ++i) {
      auto dfptr = reinterpret_cast<dunedaq::dataformats::DAPHNEFrame*>(frag->get_data()+i*sizeof(dunedaq::dataformats::DAPHNEFrame));
      if (i==0) {
        std::cout << "Printout sampled timestamps in DAPHNE headers: " ;
      }
      if (i%1000 == 0) std::cout << "Timestamp " << i << ": " << dfptr->get_timestamp() << " ";
    }
    std::cout << std::endl;
  return;
}


void read_fragment(std::unique_ptr<dunedaq::dataformats::Fragment> frag){
  std::cout << "Fragment with Run number: " << frag->get_run_number()
                 << " Trigger number: " << frag->get_trigger_number()
                 << " GeoID: " << frag->get_element_id() << std::endl;

  if (frag->get_fragment_type() == dunedaq::dataformats::FragmentType::kPDSData) {
    ReadSSPFrag(std::move(frag));
  } else if (frag->get_fragment_type() == dunedaq::dataformats::FragmentType::kTPCData) {
    ReadWibFrag(std::move(frag));
  } else {
    std::cout << "Skipping unknown fragment type" << std::endl;
  }
}

