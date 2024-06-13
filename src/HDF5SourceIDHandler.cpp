/**
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 *
 */

#include "hdf5libs/HDF5SourceIDHandler.hpp"

#include "confmodel/ReadoutGroup.hpp"
#include "confmodel/ReadoutInterface.hpp"
#include "confmodel/DROStreamConf.hpp"
#include "confmodel/GeoId.hpp"

#include "logging/Logging.hpp"
#include <nlohmann/json.hpp>

namespace dunedaq {
namespace hdf5libs {

uint64_t
encode_geoid(int det_id, int crate_id, int slot_id, int stream_id)
{
  return (static_cast<uint64_t>(stream_id) << 48) | (static_cast<uint64_t>(slot_id) << 32) |
         (static_cast<uint64_t>(crate_id) << 16) | det_id;
}

HDF5SourceIDHandler::source_id_geo_id_map_t
HDF5SourceIDHandler::make_source_id_geo_id_map(const confmodel::ReadoutMap* rmap)
{
  HDF5SourceIDHandler::source_id_geo_id_map_t output_map;

  for (auto& rog : rmap->get_groups()) {
    auto group_rset = rog->cast<confmodel::ReadoutGroup>();
    for (auto& roi : group_rset->get_contains()) {
      auto iface_rset = roi->cast<confmodel::ReadoutInterface>();
      for (auto& dros : iface_rset->get_contains()) {
        auto stream = dros->cast<confmodel::DROStreamConf>();
        auto geoid = stream->get_geo_id();
        auto geoid_int =
          encode_geoid(geoid->get_detector_id(), geoid->get_crate_id(), geoid->get_slot_id(), geoid->get_stream_id());
        daqdataformats::SourceID sid;
        sid.subsystem = daqdataformats::SourceID::Subsystem::kDetectorReadout;
        sid.id = stream->get_source_id();

        output_map[sid].push_back(geoid_int);
      }
    }
  }
  return output_map;
}

void
HDF5SourceIDHandler::store_file_level_geo_id_info(HighFive::File& h5_file, const source_id_geo_id_map_t& the_map)
{
  write_attribute(h5_file, "source_id_geo_id_map", get_json_string(the_map));
}

void
HDF5SourceIDHandler::store_record_header_source_id(HighFive::Group& record_group,
                                                   const daqdataformats::SourceID& source_id)
{
  write_attribute(record_group, "record_header_source_id", get_json_string(source_id));
}

void
HDF5SourceIDHandler::store_record_level_path_info(HighFive::Group& record_group, const source_id_path_map_t& the_map)
{
  write_attribute(record_group, "source_id_path_map", get_json_string(the_map));
}

void
HDF5SourceIDHandler::store_record_level_fragment_type_map(HighFive::Group& record_group,
                                                          const fragment_type_source_id_map_t& the_map)
{
  write_attribute(record_group, "fragment_type_source_id_map", get_json_string(the_map));
}

void
HDF5SourceIDHandler::store_record_level_subdetector_map(HighFive::Group& record_group,
                                                        const subdetector_source_id_map_t& the_map)
{
  write_attribute(record_group, "subdetector_source_id_map", get_json_string(the_map));
}

HDF5SourceIDHandler::HDF5SourceIDHandler(const uint32_t version) // NOLINT(build/unsigned)
  : m_version(version)
{
}

void
HDF5SourceIDHandler::fetch_file_level_geo_id_info(const HighFive::File& h5_file,
                                                  source_id_geo_id_map_t& source_id_geo_id_map)
{
  if (m_version >= 3) {
    try {
      std::string map_string = get_attribute<HighFive::File, std::string>(h5_file, "source_id_geo_id_map");
      parse_json_string(map_string, source_id_geo_id_map);
    } catch (...) {
    }
  }
}

void
HDF5SourceIDHandler::fetch_record_level_geo_id_info(const HighFive::Group& /*record_group*/,
                                                    source_id_geo_id_map_t& /*source_id_geo_id_map*/)
{
  // In versions 3 and 4, there is no record-level geo_id information stored in the file
  if (m_version >= 3) {
    return;
  }
}

daqdataformats::SourceID
HDF5SourceIDHandler::fetch_record_header_source_id(const HighFive::Group& record_group)
{
  daqdataformats::SourceID source_id;
  if (m_version >= 3) {
    try {
      std::string sid_string = get_attribute<HighFive::Group, std::string>(record_group, "record_header_source_id");
      parse_json_string(sid_string, source_id);
    } catch (...) {
    }
  }
  return source_id;
}

void
HDF5SourceIDHandler::fetch_source_id_path_info(const HighFive::Group& record_group,
                                               source_id_path_map_t& source_id_path_map)
{
  if (m_version >= 3) {
    try {
      std::string map_string = get_attribute<HighFive::Group, std::string>(record_group, "source_id_path_map");
      parse_json_string(map_string, source_id_path_map);
    } catch (...) {
    }
  }
}

void
HDF5SourceIDHandler::fetch_fragment_type_source_id_info(const HighFive::Group& record_group,
                                                        fragment_type_source_id_map_t& fragment_type_source_id_map)
{
  if (m_version >= 3) {
    try {
      std::string map_string = get_attribute<HighFive::Group, std::string>(record_group, "fragment_type_source_id_map");
      parse_json_string(map_string, fragment_type_source_id_map);
    } catch (...) {
    }
  }
}

void
HDF5SourceIDHandler::fetch_subdetector_source_id_info(const HighFive::Group& record_group,
                                                      subdetector_source_id_map_t& subdetector_source_id_map)
{
  if (m_version >= 3) {
    try {
      std::string map_string = get_attribute<HighFive::Group, std::string>(record_group, "subdetector_source_id_map");
      parse_json_string(map_string, subdetector_source_id_map);
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

void
HDF5SourceIDHandler::add_fragment_type_source_id_to_map(fragment_type_source_id_map_t& fragment_type_source_id_map,
                                                        const daqdataformats::FragmentType fragment_type,
                                                        const daqdataformats::SourceID& source_id)
{
  if (fragment_type_source_id_map.count(fragment_type) == 0) {
    std::set<daqdataformats::SourceID> tmp_set;
    tmp_set.insert(source_id);
    fragment_type_source_id_map[fragment_type] = tmp_set;
  } else {
    fragment_type_source_id_map[fragment_type].insert(source_id);
  }
}

void
HDF5SourceIDHandler::add_subdetector_source_id_to_map(subdetector_source_id_map_t& subdetector_source_id_map,
                                                      const detdataformats::DetID::Subdetector subdetector,
                                                      const daqdataformats::SourceID& source_id)
{
  if (subdetector_source_id_map.count(subdetector) == 0) {
    std::set<daqdataformats::SourceID> tmp_set;
    tmp_set.insert(source_id);
    subdetector_source_id_map[subdetector] = tmp_set;
  } else {
    subdetector_source_id_map[subdetector].insert(source_id);
  }
}

void
HDF5SourceIDHandler::add_subsystem_source_id_to_map(subsystem_source_id_map_t& subsystem_source_id_map,
                                                    const daqdataformats::SourceID::Subsystem subsystem,
                                                    const daqdataformats::SourceID& source_id)
{
  if (subsystem_source_id_map.count(subsystem) == 0) {
    std::set<daqdataformats::SourceID> tmp_set;
    tmp_set.insert(source_id);
    subsystem_source_id_map[subsystem] = tmp_set;
  } else {
    subsystem_source_id_map[subsystem].insert(source_id);
  }
}

std::string
HDF5SourceIDHandler::get_json_string(const daqdataformats::SourceID& source_id)
{
  nlohmann::json json_struct;
  json_struct["subsys"] = static_cast<uint32_t>(source_id.subsystem); // NOLINT(build/unsigned)
  json_struct["id"] = source_id.id;
  return json_struct.dump();
}

std::string
HDF5SourceIDHandler::get_json_string(const HDF5SourceIDHandler::source_id_path_map_t& the_map)
{
  nlohmann::json json_struct;
  for (auto const& map_element : the_map) {
    nlohmann::json json_element;
    json_element["subsys"] = static_cast<uint32_t>(map_element.first.subsystem); // NOLINT(build/unsigned)
    json_element["id"] = map_element.first.id;
    json_element["path"] = map_element.second;
    json_struct["map_entries"].push_back(json_element);
  }
  return json_struct.dump();
}

std::string
HDF5SourceIDHandler::get_json_string(const HDF5SourceIDHandler::source_id_geo_id_map_t& the_map)
{
  nlohmann::json json_struct;
  for (auto const& map_element : the_map) {
    nlohmann::json json_geo_id_list;
    for (auto const& geo_id_from_map : map_element.second) {
      json_geo_id_list.push_back(geo_id_from_map);
    }
    nlohmann::json json_element;
    json_element["subsys"] = static_cast<uint32_t>(map_element.first.subsystem); // NOLINT(build/unsigned)
    json_element["id"] = map_element.first.id;
    json_element["geoids"] = json_geo_id_list;
    json_struct["map_entries"].push_back(json_element);
  }
  return json_struct.dump();
}

std::string
HDF5SourceIDHandler::get_json_string(const HDF5SourceIDHandler::fragment_type_source_id_map_t& the_map)
{
  nlohmann::json json_struct;
  for (auto const& map_element : the_map) {
    nlohmann::json json_source_id_list;
    for (auto const& source_id_from_map : map_element.second) {
      nlohmann::json json_source_id;
      json_source_id["subsys"] = static_cast<uint32_t>(source_id_from_map.subsystem); // NOLINT(build/unsigned)
      json_source_id["id"] = source_id_from_map.id;
      json_source_id_list.push_back(json_source_id);
    }
    nlohmann::json json_element;
    json_element["fragment_type"] = static_cast<uint32_t>(map_element.first);
    json_element["sourceids"] = json_source_id_list;
    json_struct["map_entries"].push_back(json_element);
  }
  return json_struct.dump();
}

std::string
HDF5SourceIDHandler::get_json_string(const HDF5SourceIDHandler::subdetector_source_id_map_t& the_map)
{
  nlohmann::json json_struct;
  for (auto const& map_element : the_map) {
    nlohmann::json json_source_id_list;
    for (auto const& source_id_from_map : map_element.second) {
      nlohmann::json json_source_id;
      json_source_id["subsys"] = static_cast<uint32_t>(source_id_from_map.subsystem); // NOLINT(build/unsigned)
      json_source_id["id"] = source_id_from_map.id;
      json_source_id_list.push_back(json_source_id);
    }
    nlohmann::json json_element;
    json_element["subdetector"] = static_cast<uint32_t>(map_element.first);
    json_element["sourceids"] = json_source_id_list;
    json_struct["map_entries"].push_back(json_element);
  }
  return json_struct.dump();
}

void
HDF5SourceIDHandler::parse_json_string(const std::string& json_string, daqdataformats::SourceID& source_id)
{
  nlohmann::json json_struct = nlohmann::json::parse(json_string);
  daqdataformats::SourceID::Subsystem subsys = static_cast<daqdataformats::SourceID::Subsystem>(json_struct["subsys"]);
  daqdataformats::SourceID::ID_t id = static_cast<daqdataformats::SourceID::ID_t>(json_struct["id"]);
  source_id.subsystem = subsys;
  source_id.id = id;
}

void
HDF5SourceIDHandler::parse_json_string(const std::string& json_string, source_id_path_map_t& source_id_path_map)
{
  nlohmann::json json_struct = nlohmann::json::parse(json_string);
  for (auto const& json_element : json_struct["map_entries"]) {
    daqdataformats::SourceID::Subsystem subsys =
      static_cast<daqdataformats::SourceID::Subsystem>(json_element["subsys"]);
    daqdataformats::SourceID::ID_t id = static_cast<daqdataformats::SourceID::ID_t>(json_element["id"]);
    daqdataformats::SourceID source_id(subsys, id);
    source_id_path_map[source_id] = json_element["path"];
  }
}

void
HDF5SourceIDHandler::parse_json_string(const std::string& json_string, source_id_geo_id_map_t& source_id_geo_id_map)
{
  nlohmann::json json_struct = nlohmann::json::parse(json_string);
  for (auto const& json_element : json_struct["map_entries"]) {
    daqdataformats::SourceID::Subsystem subsys =
      static_cast<daqdataformats::SourceID::Subsystem>(json_element["subsys"]);
    daqdataformats::SourceID::ID_t id = static_cast<daqdataformats::SourceID::ID_t>(json_element["id"]);
    daqdataformats::SourceID source_id(subsys, id);
    std::vector<uint64_t> local_geo_id_list; // NOLINT(build/unsigned)
    nlohmann::json json_geo_id_list = json_element["geoids"];
    for (nlohmann::json json_geo_id_value : json_geo_id_list) {
      local_geo_id_list.push_back(json_geo_id_value);
    }
    source_id_geo_id_map[source_id] = local_geo_id_list;
  }
}

void
HDF5SourceIDHandler::parse_json_string(const std::string& json_string,
                                       fragment_type_source_id_map_t& fragment_type_source_id_map)
{
  nlohmann::json json_struct = nlohmann::json::parse(json_string);
  for (auto const& json_element : json_struct["map_entries"]) {
    daqdataformats::FragmentType fragment_type =
      static_cast<daqdataformats::FragmentType>(json_element["fragment_type"]);
    std::set<daqdataformats::SourceID> local_source_id_list;
    nlohmann::json json_source_id_list = json_element["sourceids"];
    for (nlohmann::json json_source_id : json_source_id_list) {
      daqdataformats::SourceID::Subsystem subsys =
        static_cast<daqdataformats::SourceID::Subsystem>(json_source_id["subsys"]);
      daqdataformats::SourceID::ID_t id = static_cast<daqdataformats::SourceID::ID_t>(json_source_id["id"]);
      daqdataformats::SourceID source_id(subsys, id);
      local_source_id_list.insert(source_id);
    }
    fragment_type_source_id_map[fragment_type] = local_source_id_list;
  }
}

void
HDF5SourceIDHandler::parse_json_string(const std::string& json_string,
                                       subdetector_source_id_map_t& subdetector_source_id_map)
{
  nlohmann::json json_struct = nlohmann::json::parse(json_string);
  for (auto const& json_element : json_struct["map_entries"]) {
    detdataformats::DetID::Subdetector subdetector =
      static_cast<detdataformats::DetID::Subdetector>(json_element["subdetector"]);
    std::set<daqdataformats::SourceID> local_source_id_list;
    nlohmann::json json_source_id_list = json_element["sourceids"];
    for (nlohmann::json json_source_id : json_source_id_list) {
      daqdataformats::SourceID::Subsystem subsys =
        static_cast<daqdataformats::SourceID::Subsystem>(json_source_id["subsys"]);
      daqdataformats::SourceID::ID_t id = static_cast<daqdataformats::SourceID::ID_t>(json_source_id["id"]);
      daqdataformats::SourceID source_id(subsys, id);
      local_source_id_list.insert(source_id);
    }
    subdetector_source_id_map[subdetector] = local_source_id_list;
  }
}

} // namespace hdf5libs
} // namespace dunedaq
