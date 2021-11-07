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
#include "detchannelmaps/TPCChannelMap.hpp"

using namespace dunedaq::detchannelmaps;

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




void ReadWibFrag(std::unique_ptr<dunedaq::dataformats::Fragment> frag, std::shared_ptr<TPCChannelMap> cm, 
		std::map<size_t, std::pair<float,float> > *offline_map, std::vector<uint32_t> *adc_sums) { 

     if(frag->get_fragment_type() == dunedaq::dataformats::FragmentType::kTPCData) {
//       std::cout << "Fragment with Run number: " << frag->get_run_number()
//                 << " Trigger number: " << frag->get_trigger_number()
//                 << " Sequence number: " << frag->get_sequence_number()
//                 << " GeoID: " << frag->get_element_id() << std::endl;
       size_t n_blocks = 4;
       size_t n_channels = 64;
//       if(frag->get_element_id().element_id == 6) {
//          n_blocks = 2;
//       }
       size_t raw_data_packets = (frag->get_size() - sizeof(dunedaq::dataformats::FragmentHeader)) / sizeof(dunedaq::dataformats::WIBFrame);
       std::map <size_t, std::vector<uint16_t> > ch_adcs_map;
       auto whdr = reinterpret_cast<dunedaq::dataformats::WIBFrame*>(frag->get_data());
       uint crate = 1; // hardcoded for decoders.... should be:  whdr->get_wib_header()->crate_no;
       uint slot = whdr->get_wib_header()->slot_no;
       uint fiber = whdr->get_wib_header()->fiber_no;

       //if(slot != 3 || fiber !=1) return;
/*      
       if(slot > 1 && fiber == 1) {
         fiber = 2;
       }
       if(slot > 1 && fiber == 2) {
         fiber = 1;
       }
*/

        for (size_t i=0; i < raw_data_packets; ++i) {
           auto wfptr = reinterpret_cast<dunedaq::dataformats::WIBFrame*>(frag->get_data()+i*sizeof(dunedaq::dataformats::WIBFrame));
           for (size_t k=0 ; k < n_blocks; ++k) {
              for (size_t j=0; j < n_channels; ++j) {
                 ch_adcs_map[k*64+j].push_back(wfptr->get_channel(k,j));
                 adc_sums->at(i) += wfptr->get_channel(k,j);
              }
           }
       }
       std::cout << "Arrays filled for link " << frag->get_element_id().element_id << std::endl;
       std::stringstream filename;
       filename << "./Link_" << frag->get_element_id().element_id << ".txt";
       std::ofstream output_file(filename.str());
       float mean, rms, stddev;
       size_t oc;
       for (size_t k=0 ; k < n_blocks*n_channels; ++k) {
             rmsValue(ch_adcs_map[k], mean, rms, stddev); 
             output_file << k << " " << mean << " " << rms << " " << stddev << std::endl;
             oc = cm->get_offline_channel_from_crate_slot_fiber_chan(crate, slot, fiber, k);
             std::cout << k << " " << oc << " " << mean << " " << rms << " " << stddev << std::endl;
             offline_map->emplace(oc, std::make_pair(mean, stddev)); 
       }

      

/*
       if (slot == 0 && fiber == 1) {
       for(size_t k=0 ; k < n_blocks*n_channels; ++k) { 
          std::stringstream filename_raw;
          filename_raw << "./Link_" << frag->get_element_id().element_id << "_RawCh_" << k<< ".txt";
          std::ofstream raw_datafile(filename_raw.str());
          for (auto & val : ch_adcs_map[k]) {
             raw_datafile << val << std::endl;
          }
        } 
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

  // Hack to initialise vector...
  std::vector<std::string> datasets_path = decoder.get_fragments(num_trs);
  size_t raw_data_packets = (decoder.get_frag_ptr(datasets_path[0])->get_size() - sizeof(dunedaq::dataformats::FragmentHeader)) / sizeof(dunedaq::dataformats::WIBFrame);

  //std::vector<std::string> datasets_path = decoder.get_trh(num_trs);
  std::map<size_t, std::pair<float,float> > offline_map;
  std::vector<uint32_t> adc_channels_sums(raw_data_packets,0);
  std::shared_ptr<TPCChannelMap> vdcb_map = make_map("VDColdboxChannelMap");


  for (auto& element : datasets_path) {
    std::cout << "*** " << element << " ***" << std::endl;
    ReadWibFrag(decoder.get_frag_ptr(element), vdcb_map, &offline_map, &adc_channels_sums);
  }

  std::ofstream output_file_plane_0("offline_map_mean_stddev_0.txt");
  std::ofstream output_file_plane_1("offline_map_mean_stddev_1.txt");
  std::ofstream output_file_plane_2("offline_map_mean_stddev_2.txt");
  int plane = 0;
  for (auto p : offline_map) {
    try {
      plane = vdcb_map->get_plane_from_offline_channel(p.first);
      if(plane == 0) {
         output_file_plane_0 << p.first << " " << p.second.first << " " << p.second.second << std::endl;
      } else if (plane == 1) {
         output_file_plane_1 << p.first << " " << p.second.first << " " << p.second.second << std::endl;
      } else {
         output_file_plane_2 << p.first << " " << p.second.first << " " << p.second.second << std::endl;
      }
    }
    catch (std::exception & e) {
      std::cout << "Offline channel=" << p.first << " " << e.what() << std::endl;
    }
  }

  std::ofstream output_file_2("summed_adcs.txt");
  uint64_t ts = 0;
  for (size_t i = 0; i < 8192 ; ++i) {
    output_file_2 << ts << " " << adc_channels_sums[i] <<std::endl;
    ts += 500; 
  }
  return 0;
}
