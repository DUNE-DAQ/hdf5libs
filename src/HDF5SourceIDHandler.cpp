/**
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 *
 */

#include "hdf5libs/HDF5SourceIDHandler.hpp"
#include "hdf5libs/hdf5sourceidmaps/Nljs.hpp"

//#include <stdexcept>

namespace dunedaq {
namespace hdf5libs {

void
HDF5SourceIDHandler::write_version_info(HighFive::File& h5_file)
{
  write_attribute(h5_file, "source_id_metadata_version", s_source_id_param_version);
}

void
HDF5SourceIDHandler::populate_source_id_geo_id_map(std::shared_ptr<detchannelmaps::HardwareMapService> /*hw_map_svc*/,
                                                   source_id_geo_id_map_t& /*the_map*/)
{}

void
HDF5SourceIDHandler::write_file_level_geo_id_info(HighFive::File& /*h5_file*/,
                                                  const source_id_geo_id_map_t& /*the_map*/)
{
  //write_attribute(record_level_group, "source_id_path_map", get_json_string(sid_maps.get_source_id_path_map()));
  //HDF5RawDataFileSid.cpp_20220816145709:  write_attribute(record_level_group, "source_id_geo_id_map", get_json_string(sid_maps.get_source_id_geo_id_map()));
}

void
HDF5SourceIDHandler::write_record_level_path_info(HighFive::Group& record_group,
                                                  const source_id_path_map_t& the_map)
{
  write_attribute(record_group, "source_id_path_map", get_json_string(the_map));
}

uint32_t
HDF5SourceIDHandler::determine_version_from_file(const HighFive::File& /*h5_file*/) // NOLINT(build/unsigned)
{
  uint32_t version = 1;

  // fetch the version attribute from the file

  return version;
}

HDF5SourceIDHandler::HDF5SourceIDHandler(const uint32_t version) // NOLINT(build/unsigned)
  : m_version(version)
{}

void
HDF5SourceIDHandler::fetch_file_level_geo_id_info(const HighFive::File& /*h5_file*/,
                                                  source_id_geo_id_map_t& /*the_map*/)
{}

void
HDF5SourceIDHandler::fetch_record_level_geo_id_info(const HighFive::Group& /*record_group*/,
                                                    source_id_geo_id_map_t& /*the_map*/)
{}

void
HDF5SourceIDHandler::fetch_source_id_path_info(const HighFive::Group& /*record_group*/,
                                               source_id_path_map_t& /*the_map*/)
{}

void
HDF5SourceIDHandler::add_source_id_path_to_map(source_id_path_map_t& source_id_path_map,
                                               const daqdataformats::SourceID& source_id,
                                               const std::string& hdf5_path)
{
  source_id_path_map[source_id] = hdf5_path;
}

void
HDF5SourceIDHandler::add_source_id_geo_id_to_map(source_id_geo_id_map_t& source_id_geo_id_map,
                                                 const daqdataformats::SourceID& source_id,
                                                 uint64_t geo_id) // NOLINT(build/unsigned)
{
  if (source_id_geo_id_map.count(source_id) == 0) {
    std::vector<uint64_t> tmp_vec; // NOLINT(build/unsigned)
    tmp_vec.push_back(geo_id);
    source_id_geo_id_map[source_id] = tmp_vec;
  } else {
    source_id_geo_id_map[source_id].push_back(geo_id);
  }
}

std::string
HDF5SourceIDHandler::get_json_string(const HDF5SourceIDHandler::source_id_path_map_t& the_map)
{
  hdf5sourceidmaps::SourceIDPathMap json_struct;
  for (auto map_element : the_map) {
    json_struct.source_id_version = map_element.first.version;
    hdf5sourceidmaps::SourceIDPathPair json_element;
    json_element.subsys = static_cast<uint32_t>(map_element.first.subsystem); // NOLINT(build/unsigned)
    json_element.id = map_element.first.id;
    json_element.path = map_element.second;
    json_struct.map_entries.push_back(json_element);
  }
  hdf5sourceidmaps::data_t json_obj;
  hdf5sourceidmaps::to_json(json_obj, json_struct);
  return json_obj.dump();
}

std::string
HDF5SourceIDHandler::get_json_string(const HDF5SourceIDHandler::source_id_geo_id_map_t& the_map)
{
  hdf5sourceidmaps::SourceIDGeoIDMap json_struct;
  for (auto map_element : the_map) {
    json_struct.source_id_version = map_element.first.version;
    hdf5sourceidmaps::GeoIDList json_geo_id_list;
    for (auto const& geo_id_from_map : map_element.second) {
      json_geo_id_list.push_back(geo_id_from_map);
    }
    hdf5sourceidmaps::SourceIDGeoIDPair json_element;
    json_element.subsys = static_cast<uint32_t>(map_element.first.subsystem); // NOLINT(build/unsigned)
    json_element.id = map_element.first.id;
    json_element.geoids = json_geo_id_list;
    json_struct.map_entries.push_back(json_element);
  }
  hdf5sourceidmaps::data_t json_obj;
  hdf5sourceidmaps::to_json(json_obj, json_struct);
  return json_obj.dump();
}

} // namespace hdf5libs
} // namespace dunedaq
