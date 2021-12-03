/**
 * @file raw_tpc_decoder.cpp
 *
 * Decode a file of WIB1 TPC data, saving all of the ADC values
 *
 * Output format is a text file. Each column contains the ADCs from a
 * single channel. The first row is the channel number and subsequent
 * rows are the ADCs in time order
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include <fstream>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <string>

#include "boost/program_options.hpp"

#include "hdf5libs/DAQDecoder.hpp"
#include "logging/Logging.hpp"
#include "utils.hpp"

#include "detchannelmaps/TPCChannelMap.hpp"

using namespace dunedaq::hdf5libs;
using namespace dunedaq::detchannelmaps;

int
main(int argc, char** argv)
{
  namespace po = boost::program_options;
  po::options_description desc("Allowed options");
  desc.add_options()
    ("help,h",                                 "produce help message")
    ("input,i",      po::value<std::string>(), "Input file name")
    ("output,o",     po::value<std::string>(), "Output file name")
    ("channelmap,c", po::value<std::string>(), "Channel map to use");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  if (vm.count("help") || vm.empty()) {
    std::cout << desc << "\n";
    return 1;
  }

  if (!vm.count("input")) {
    std::cout << "No input file specified" << std::endl;
    std::cout << desc << std::endl;
    return 1;
  }

  if (!vm.count("output")) {
    std::cout << "No output file specified" << std::endl;
    std::cout << desc << std::endl;
    return 1;
  }

  std::string channel_map_name = vm["channelmap"].as<std::string>();
  if (channel_map_name != "VDColdboxChannelMap" && channel_map_name != "ProtoDUNESP1ChannelMap") {
    std::cout << "Valid values for -c/--channelmap are ProtoDUNESP1ChannelMap and VDColdboxChannelMap" << std::endl;
    return 1;
  }

  std::ofstream fout(vm["output"].as<std::string>());

  DAQDecoder decoder = DAQDecoder(vm["input"].as<std::string>(), 1);

  std::vector<std::string> datasets_path = decoder.get_fragments(1);

  std::cout << "Total number of fragments: " << datasets_path.size() << std::endl;

  std::cout << "Building channel map..." << std::flush;
  std::shared_ptr<TPCChannelMap> channel_map = make_map(channel_map_name);
  std::cout << "done" << std::endl;

  std::vector<std::unique_ptr<dunedaq::daqdataformats::Fragment>> frags;
  for (auto const& path : datasets_path) {
    auto frag = decoder.get_frag_ptr(path);
    // We only decode TPC data in this application
    if (frag->get_fragment_type() == dunedaq::daqdataformats::FragmentType::kTPCData) {
      frags.push_back(std::move(frag));
    }
  }

  std::cout << "Number of TPC fragments to be decoded: " << frags.size() << std::endl;
  
  size_t n_samples = (frags[0]->get_size() - sizeof(dunedaq::daqdataformats::FragmentHeader)) /
                     sizeof(dunedaq::detdataformats::wib::WIBFrame);
  std::cout << "There are " << n_samples << " samples" << std::endl;

  const size_t n_channels = 256;

  // First output line is the offline channel numbers for all channels
  for (auto const& frag : frags) {
    auto frame = reinterpret_cast<dunedaq::detdataformats::wib::WIBFrame*>(frag->get_data());
    auto crate = 1; // hardcoded for decoders.... should be:  frame->get_wib_header()->crate_no;
    auto slot = frame->get_wib_header()->slot_no;
    auto fiber = frame->get_wib_header()->fiber_no;
    std::cout << "frame has slot/fiber = " << slot << "/" << fiber << std::endl;
    for (size_t i = 0; i < n_channels; ++i) {
      fout << std::setw(5) << channel_map->get_offline_channel_from_crate_slot_fiber_chan(crate, slot, fiber, i) << " ";
    }
  }
  fout << std::endl;

  // Now output the actual ADCs
  for (size_t i = 0; i < n_samples; ++i) {
    for (auto const& frag : frags) {
      auto frame = reinterpret_cast<dunedaq::detdataformats::wib::WIBFrame*>(frag->get_data()) + i;
      for (size_t j = 0; j < n_channels; ++j) {
        fout << std::setw(5) << frame->get_channel(j) << " ";
      } // loop over channels
    }   // loop over links
    fout << std::endl;
  } // loop over samples

  return 0;
}
