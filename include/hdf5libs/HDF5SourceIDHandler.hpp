/**
 * @file HDF5SourceIDHandler.hpp
 *
 * Collection of routines for translating SourceID-related quantities
 * from string formats to in-memory representations and back.
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef HDF5LIBS_INCLUDE_HDF5LIBS_HDF5SOURCEIDHANDLER_HPP_
#define HDF5LIBS_INCLUDE_HDF5LIBS_HDF5SOURCEIDHANDLER_HPP_

#include "daqdataformats/Fragment.hpp"
#include "daqdataformats/SourceID.hpp"
#include "detchannelmaps/HardwareMapService.hpp"
#include "hdf5libs/hdf5rawdatafile/Structs.hpp"
#include "detdataformats/DetID.hpp"

//#include "hdf5libs/hdf5filelayout/Structs.hpp"
//#include "daqdataformats/TimeSliceHeader.hpp"
//#include "daqdataformats/TriggerRecordHeader.hpp"
//#include "logging/Logging.hpp"

#include <highfive/H5File.hpp>
#include <highfive/H5Group.hpp>

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace dunedaq {
namespace hdf5libs {

/**
 * At the moment, this class is designed to handle different versions of
 * translation (to/from strings) when *reading* data.  For writing data,
 * the interface is currently designed to only support the current version
 * of the translation.  If we ever decide to add support for writing of
 * different versions, then the 'store' methods would become non-static.
 */

class HDF5SourceIDHandler
{
public:
  typedef std::map<daqdataformats::SourceID, std::string> source_id_path_map_t;
  typedef std::map<daqdataformats::SourceID, std::vector<uint64_t>> source_id_geo_id_map_t; // NOLINT(build/unsigned)
  typedef std::map<daqdataformats::SourceID::Subsystem, std::set<daqdataformats::SourceID>> subsystem_source_id_map_t;
  typedef std::map<daqdataformats::FragmentType, std::set<daqdataformats::SourceID>> fragment_type_source_id_map_t;
  typedef std::map<detdataformats::DetID::Subdetector, std::set<daqdataformats::SourceID>> subdetector_source_id_map_t;

  /**
   * Populates the specified source_id_geo_id map with information contained in the
   * specified Hardware Map.
   */
  static void populate_source_id_geo_id_map(std::shared_ptr<detchannelmaps::HardwareMapService> hw_map_svc,
                                            source_id_geo_id_map_t& the_map);

  static void populate_source_id_geo_id_map(dunedaq::hdf5libs::hdf5rawdatafile::SourceGeoIDMap  src_id_geo_id_mp_struct,
                                            source_id_geo_id_map_t& the_map);
  /**
   * Stores the map from SourceID to GeoID in the specified HighFive::File.
   */
  static void store_file_level_geo_id_info(HighFive::File& h5_file, const source_id_geo_id_map_t& the_map);

  /**
   * Stores the SourceID of the record header DataSet in the specified HighFive::Group.
   */
  static void store_record_header_source_id(HighFive::Group& record_group, const daqdataformats::SourceID& source_id);

  /**
   * Stores the map from SourceID to HDF5 Path in the specified HighFive::Group.
   */
  static void store_record_level_path_info(HighFive::Group& record_group, const source_id_path_map_t& the_map);

  /**
   * Stores the map from FragmentType to SourceID in the specified HighFive::Group.
   */
  static void store_record_level_fragment_type_map(HighFive::Group& record_group,
                                                   const fragment_type_source_id_map_t& the_map);

  /**
   * Stores the map from DetID::Subdetector to SourceID in the specified HighFive::Group.
   */
  static void store_record_level_subdetector_map(HighFive::Group& record_group,
                                                 const subdetector_source_id_map_t& the_map);

  /**
   * @brief Constructor.
   */
  explicit HDF5SourceIDHandler(const uint32_t version); // NOLINT(build/unsigned)

  /**
   * Adds entries to the specified SourceID-to-GeoID map using information
   * stored at the file level in the specified HighFive::File.
   */
  void fetch_file_level_geo_id_info(const HighFive::File& h5_file, source_id_geo_id_map_t& the_map);

  /**
   * Adds entries to the specified SourceID-to-GeoID map using information
   * stored at the record level in the specified HighFive::Group.
   */
  void fetch_record_level_geo_id_info(const HighFive::Group& record_group, source_id_geo_id_map_t& the_map);

  /**
   * Fetches the record header SourceID using information
   * stored at the record level in the specified HighFive::Group.
   */
  daqdataformats::SourceID fetch_record_header_source_id(const HighFive::Group& record_group);

  /**
   * Adds entries to the specified SourceID-to-HDF5-Path map using information
   * stored at the record level in the specified HighFive::Group.
   */
  void fetch_source_id_path_info(const HighFive::Group& record_group, source_id_path_map_t& the_map);

  /**
   * Adds entries to the specified FragmentType-to-SourceID map using information
   * stored at the record level in the specified HighFive::Group.
   */
  void fetch_fragment_type_source_id_info(const HighFive::Group& record_group, fragment_type_source_id_map_t& the_map);

  /**
   * Adds entries to the specified Subdetector-to-SourceID map using information
   * stored at the record level in the specified HighFive::Group.
   */
  void fetch_subdetector_source_id_info(const HighFive::Group& record_group, subdetector_source_id_map_t& the_map);

  /**
   * Adds the specified SourceID and HDF5 Path to the specified source_id_path map.
   */
  static void add_source_id_path_to_map(source_id_path_map_t& source_id_path_map,
                                        const daqdataformats::SourceID& source_id,
                                        const std::string& hdf5_path);

  /**
   * Adds the specified SourceID and GeoID list to the specified source_id_geo_id map.
   */
  static void add_source_id_geo_id_to_map(source_id_geo_id_map_t& source_id_geo_id_map,
                                          const daqdataformats::SourceID& source_id,
                                          uint64_t geo_id); // NOLINT(build/unsigned)

  /**
   * Adds the specified FragmentType and SourceId to the specified fragment_type_source_id map.
   */
  static void add_fragment_type_source_id_to_map(fragment_type_source_id_map_t& fragment_type_source_id_map,
                                                 const daqdataformats::FragmentType fragment_type,
                                                 const daqdataformats::SourceID& source_id);

  /**
   * Adds the specified Subdetector and SourceId to the specified subdetector_source_id map.
   */
  static void add_subdetector_source_id_to_map(subdetector_source_id_map_t& subdetector_source_id_map,
                                               const detdataformats::DetID::Subdetector subdetector,
                                               const daqdataformats::SourceID& source_id);

  /**
   * Adds the specified Subsystem and SourceId to the specified subsystem_source_id map.
   */
  static void add_subsystem_source_id_to_map(subsystem_source_id_map_t& subsystem_source_id_map,
                                             const daqdataformats::SourceID::Subsystem subsystem,
                                             const daqdataformats::SourceID& source_id);

private:
  /**
   * @brief version number
   */
  uint32_t m_version; // NOLINT(build/unsigned)

  /**
   * Produces the JSON string that corresponds to the specified source_id
   */
  static std::string get_json_string(const daqdataformats::SourceID& source_id);

  /**
   * Produces the JSON string that corresponds to the specified source_id_path map
   */
  static std::string get_json_string(const source_id_path_map_t& source_id_path_map);

  /**
   * Produces the JSON string that corresponds to the specified source_id_geo_id map
   */
  static std::string get_json_string(const source_id_geo_id_map_t& source_id_geo_id_map);

  /**
   * Produces the JSON string that corresponds to the specified fragment_type_source_id map
   */
  static std::string get_json_string(const fragment_type_source_id_map_t& fragment_type_source_id_map);

  /**
   * Produces the JSON string that corresponds to the specified subdetector_source_id map
   */
  static std::string get_json_string(const subdetector_source_id_map_t& subdetector_source_id_map);

  /**
   * Parses the specified JSON string into the specified source_id
   */
  static void parse_json_string(const std::string& json_string, daqdataformats::SourceID& source_id);

  /**
   * Parses the specified JSON string into the specified source_id_path map
   */
  static void parse_json_string(const std::string& json_string, source_id_path_map_t& source_id_path_map);

  /**
   * Parses the specified JSON string into the specified source_id_geo_id map
   */
  static void parse_json_string(const std::string& json_string, source_id_geo_id_map_t& source_id_geo_id_map);

  /**
   * Parses the specified JSON string into the specified fragment_type_source_id_map
   */
  static void parse_json_string(const std::string& json_string, fragment_type_source_id_map_t& fragment_type_source_id_map);

  /**
   * Parses the specified JSON string into the specified subdetector_source_id_map
   */
  static void parse_json_string(const std::string& json_string, subdetector_source_id_map_t& subdetector_source_id_map);

  /**
   * Writes the specified attribute name and value to the specified HightFive File or Group.
   */
  template<typename C, typename T>
  static void write_attribute(HighFive::AnnotateTraits<C>& h5annt, const std::string& name, T value);

  /**
   * Fetches the attribute with the specified name from the specified HightFive File or Group.
   */
  template<typename C, typename T>
  static T get_attribute(const HighFive::AnnotateTraits<C>& h5annt, const std::string& name);
};

template<typename C, typename T>
void
HDF5SourceIDHandler::write_attribute(HighFive::AnnotateTraits<C>& h5annt, const std::string& name, T value)
{
  if (!(h5annt.hasAttribute(name)))
    h5annt.createAttribute(name, value);
  // else
  //   ers::warning(HDF5AttributeExists(ERS_HERE, name));
}

template<typename C, typename T>
T
HDF5SourceIDHandler::get_attribute(const HighFive::AnnotateTraits<C>& h5annt, const std::string& name)
{
  // if (!h5annt.hasAttribute(name)) {
  //   throw InvalidHDF5Attribute(ERS_HERE, name);
  // }
  auto attr = h5annt.getAttribute(name);
  T value;
  attr.read(value);
  return value;
}

} // namespace hdf5libs
} // namespace dunedaq

#endif // HDF5LIBS_INCLUDE_HDF5LIBS_HDF5SOURCEIDHANDLER_HPP_
