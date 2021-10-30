/**
 * @file demo.cpp
 *
 * Demo of HDF5 file reader: this example shows how to extract fragments from a file and decode WIB frames
 * A user who wishes to decode different data or make different operations should just change this file.
 * 
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */


#include <iostream>
#include <string>
#include <fstream>
#include <math.h>  

#include "DAQDecoder.hpp" 

#include "dataformats/wib/WIBFrame.hpp"

void rmsValue(std::vector<uint16_t> adcs, float &mean, float &rms, float &stddev)
{
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
     if(frag->get_fragment_type() == dunedaq::dataformats::FragmentType::kTPCData) {
//       std::cout << "Fragment with Run number: " << frag->get_run_number()
//                 << " Trigger number: " << frag->get_trigger_number()
//                 << " Sequence number: " << frag->get_sequence_number()
//                 << " GeoID: " << frag->get_element_id() << std::endl;
       size_t n_blocks = 4;
       size_t n_channels = 64;
       //if(frag->get_element_id().element_id == 6) {
       //   n_blocks = 2;
       //}
       size_t raw_data_packets = (frag->get_size() - sizeof(dunedaq::dataformats::FragmentHeader)) / sizeof(dunedaq::dataformats::WIBFrame);
        std::map <size_t, std::vector<uint16_t> > ch_adcs_map;

        for (size_t i=0; i < raw_data_packets; ++i) {
           auto wfptr = reinterpret_cast<dunedaq::dataformats::WIBFrame*>(frag->get_data()+i*sizeof(dunedaq::dataformats::WIBFrame));
           for (size_t k=0 ; k < n_blocks; ++k) {
              for (size_t j=0; j < n_channels; ++j) {
                 ch_adcs_map[k*64+j].push_back(wfptr->get_channel(k,j));
                 //if (wfptr->get_channel(k,j) > 3000 || wfptr->get_channel(k,j) < 500) {
                 //   std::cout << "Bad ADC value for link " << frag->get_element_id().element_id 
                 //             << " channel=" << k*64+j << " value=" << wfptr->get_channel(k,j) << std::endl;
                 //} 
              }
           }
       }
       std::cout << "Arrays filled for link " << frag->get_element_id().element_id << std::endl;
       std::stringstream filename;
       filename << "./Link_" << frag->get_element_id().element_id << ".txt";
       std::ofstream output_file(filename.str());
       float mean, rms, stddev;
       for (size_t k=0 ; k < n_blocks*n_channels; ++k) {
             rmsValue(ch_adcs_map[k], mean, rms, stddev); 
             output_file << k << " " << mean << " " << rms << " " << stddev << std::endl;
             std::cout << k << " " << mean << " " << rms << " " << stddev << std::endl;
       }

       /*
       for(size_t k=0 ; k < n_blocks*n_channels; ++k) { 
          std::stringstream filename_raw;
          filename_raw << "./Link_" << frag->get_element_id().element_id << "_RawCh_" << k<< ".txt";
          std::ofstream raw_datafile(filename_raw.str());
          for (auto & val : ch_adcs_map[k]) {
             std::cout << val << std::endl;
          }
         */


//           if (i==0) {
//               std::cout << "First WIB header:"<< *(wfptr->get_wib_header());
//               std::cout << "Printout sampled timestamps in WIB headers: " ;
//           }
 //          if(i%1000 == 0) std::cout << "Timestamp " << i << ": " << wfptr->get_timestamp() << " ";
//       }
       //std::cout << std::endl;
     }
     else {
       std::cout << "Skipping unknown fragment type" << std::endl;
     }
     return;
}

int main(int argc, char** argv){
  int num_trs = 1000000;
  if(argc <2) {
    std::cerr << "Usage: demo <fully qualified file name> [number of events to read]" << std::endl;
    return -1;
  }

  if(argc == 3) {
    num_trs = std::stoi(argv[2]);
    std::cout << "Number of events to read: " << num_trs << std::endl;
  }   

  DAQDecoder decoder = DAQDecoder(argv[1], num_trs);

  std::vector<std::string> tr_paths = decoder.get_trh(num_trs);
  for (auto& tr : tr_paths) {
    std::cout << "=== " << tr << " ===" << std::endl;
    std::unique_ptr<dunedaq::dataformats::TriggerRecordHeader> trh_ptr(decoder.get_trh_ptr(tr));
    std::cout << "Trigger record with run number: " << trh_ptr->get_run_number()
              << " Trigger number: " << trh_ptr->get_trigger_number()
              << " Sequence number: " << trh_ptr->get_sequence_number() << std::endl;
  }


  std::vector<std::string> datasets_path = decoder.get_fragments(num_trs);
  //std::vector<std::string> datasets_path = decoder.get_trh(num_trs);
  for (auto& element : datasets_path) {
    std::cout << "*** " << element << " ***" << std::endl;
    ReadWibFrag(decoder.get_frag_ptr(element));
  }
  return 0;
}
