/**
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 *
 */

#include "hdf5libs/HDF5SourceIDMaps.hpp"
#include "hdf5libs/hdf5sourceidmaps/Nljs.hpp"

//#include <stdexcept>

namespace dunedaq {
namespace hdf5libs {

HDF5SourceIDMaps::HDF5SourceIDMaps(uint32_t version) // NOLINT(build/unsigned)
  : m_version(version)
{}

void
HDF5SourceIDMaps::add_source_id_path(const daqdataformats::SourceID& source_id, const std::string hdf5_path)
{
  m_source_id_path_map[source_id] = hdf5_path;
}

std::string get_json_string(const HDF5SourceIDMaps::source_id_path_map_t& the_map)
{
  hdf5sourceidmaps::SourceIDPathMap json_struct;
  for (auto map_element : the_map) {
    json_struct.source_id_version = map_element.first.version;
    hdf5sourceidmaps::SourceIDPathPair json_element;
    json_element.subsys = static_cast<uint32_t>(map_element.first.subsystem);
    json_element.id = map_element.first.id;
    json_element.path = map_element.second;
    json_struct.map_entries.push_back(json_element);
  }
  hdf5sourceidmaps::data_t json_obj;
  hdf5sourceidmaps::to_json(json_obj, json_struct);
  return json_obj.dump();
}

} // namespace hdf5libs
} // namespace dunedaq
