/**
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 *
 */

#include "hdf5libs/HDF5SourceIDHandler.hpp"
#include "hdf5libs/hdf5sourceidmaps/Nljs.hpp"

#include "logging/Logging.hpp"

namespace dunedaq {
namespace hdf5libs {

void
HDF5SourceIDHandler::write_version_info(HighFive::File& h5_file)
{
  write_attribute(h5_file, "source_id_metadata_version", s_source_id_param_version);
}

void
HDF5SourceIDHandler::populate_source_id_geo_id_map(std::shared_ptr<detchannelmaps::HardwareMapService> hw_map_svc,
                                                   source_id_geo_id_map_t& source_id_geo_id_map)
{
  std::vector<detchannelmaps::HardwareMapService::HWInfo> hw_info_list = hw_map_svc->get_all_hw_info();
  for (auto const& hw_info : hw_info_list) {
    daqdataformats::SourceID source_id(daqdataformats::SourceID::Subsystem::kDetectorReadout, hw_info.dro_source_id);
    add_source_id_geo_id_to_map(source_id_geo_id_map, source_id, hw_info.geo_id);
  }
}

void
HDF5SourceIDHandler::write_file_level_geo_id_info(HighFive::File& h5_file, const source_id_geo_id_map_t& the_map)
{
  write_attribute(h5_file, "source_id_geo_id_map", get_json_string(the_map));
}

void
HDF5SourceIDHandler::write_record_level_path_info(HighFive::Group& record_group, const source_id_path_map_t& the_map)
{
  write_attribute(record_group, "source_id_path_map", get_json_string(the_map));
}

uint32_t
HDF5SourceIDHandler::determine_version_from_file(const HighFive::File& h5_file) // NOLINT(build/unsigned)
{
  uint32_t version = 1;

  try {
    version = get_attribute<HighFive::File, uint32_t>(h5_file, "source_id_metadata_version");
  } catch (...) {
  }

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
HDF5SourceIDHandler::fetch_source_id_path_info(const HighFive::Group& record_group,
                                               source_id_path_map_t& source_id_path_map)
{
  if (m_version == 3) {
    try {
      std::string map_string = get_attribute<HighFive::Group, std::string>(record_group, "source_id_path_map");
      parse_json_string(map_string, source_id_path_map);
    } catch (...) {
    }
  }
}

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
  for (auto const& map_element : the_map) {
    json_struct.source_id_version = map_element.first.version;
    hdf5sourceidmaps::SourceIDPathPair json_element;
    json_element.subsys = static_cast<uint32_t>(map_element.first.subsystem); // NOLINT(build/unsigned)
    json_element.id = map_element.first.id;
    json_element.path = map_element.second;
    json_struct.map_entries.push_back(json_element);
  }
  hdf5sourceidmaps::data_t json_tmp_data;
  hdf5sourceidmaps::to_json(json_tmp_data, json_struct);
  return json_tmp_data.dump();
}

std::string
HDF5SourceIDHandler::get_json_string(const HDF5SourceIDHandler::source_id_geo_id_map_t& the_map)
{
  hdf5sourceidmaps::SourceIDGeoIDMap json_struct;
  for (auto const& map_element : the_map) {
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
  hdf5sourceidmaps::data_t json_tmp_data;
  hdf5sourceidmaps::to_json(json_tmp_data, json_struct);
  return json_tmp_data.dump();
}

void
HDF5SourceIDHandler::parse_json_string(const std::string& json_string, source_id_path_map_t& source_id_path_map)
{
  hdf5sourceidmaps::SourceIDPathMap json_struct;
  hdf5sourceidmaps::data_t json_tmp_data = nlohmann::json::parse(json_string);
  hdf5sourceidmaps::from_json(json_tmp_data, json_struct);
  for (auto const& json_element : json_struct.map_entries) {
    daqdataformats::SourceID::Subsystem subsys = static_cast<daqdataformats::SourceID::Subsystem>(json_element.subsys);
    daqdataformats::SourceID::ID_t id = static_cast<daqdataformats::SourceID::ID_t>(json_element.id);
    daqdataformats::SourceID source_id(subsys, id);
    source_id_path_map[source_id] = json_element.path;
  }
}

void
HDF5SourceIDHandler::parse_json_string(const std::string& /*json_string*/,
                                       source_id_geo_id_map_t& /*source_id_geo_id_map*/)
{}

} // namespace hdf5libs
} // namespace dunedaq
