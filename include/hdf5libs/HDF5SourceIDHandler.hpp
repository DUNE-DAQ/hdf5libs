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

#include "daqdataformats/SourceID.hpp"
#include "detchannelmaps/HardwareMapService.hpp"

//#include "hdf5libs/hdf5filelayout/Structs.hpp"
//#include "daqdataformats/Fragment.hpp"
//#include "daqdataformats/TimeSliceHeader.hpp"
//#include "daqdataformats/TriggerRecordHeader.hpp"
//#include "logging/Logging.hpp"

#include <highfive/H5File.hpp>
#include <highfive/H5Group.hpp>

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace dunedaq {
namespace hdf5libs {

/**
 * At the moment, this class is designed to handle different versions of
 * translation (to/from strings) when *reading* data.  For writing data,
 * the interface is currently designed to only support the current version
 * of the translation.  If we ever decide to add support for writing of
 * different versions, then the 'write' methods would become non-static.
 */

class HDF5SourceIDHandler
{
public:
  typedef std::map<daqdataformats::SourceID, std::string> source_id_path_map_t;
  typedef std::map<daqdataformats::SourceID, std::vector<uint64_t>> source_id_geo_id_map_t; // NOLINT(build/unsigned)

  /**
   * The current version of the SourceID parameter translation logic.
   */
  static constexpr uint32_t s_source_id_param_version = 3; // NOLINT(build/unsigned)

  /**
   * Writes the current version of this class into the specified file.
   */
  static void write_version_info(HighFive::File& h5_file);

  /**
   * Populates the specified source_id_geo_id map with information contained in the
   * specified Hardware Map.
   */
  static void populate_source_id_geo_id_map(std::shared_ptr<detchannelmaps::HardwareMapService> hw_map_svc,
                                            source_id_geo_id_map_t& the_map);

  /**
   * Writes the map from SourceID to GeoID into the specified HighFive::File.
   */
  static void write_file_level_geo_id_info(HighFive::File& h5_file, const source_id_geo_id_map_t& the_map);

  /**
   * Writes the map from SourceID to HDF5 Path into the specified HighFive::Group.
   */
  static void write_record_level_path_info(HighFive::Group& record_group, const source_id_path_map_t& the_map);

  /**
   * Determines the version of this class that was used when the
   * specified file was written.
   */
  static uint32_t determine_version_from_file(const HighFive::File& h5_file);

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
   * Adds entries to the specified SourceID-to-HDF5-Path map using information
   * stored at the record level in the specified HighFive::Group.
   */
  void fetch_source_id_path_info(const HighFive::Group& record_group, source_id_path_map_t& the_map);

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

private:
  /**
   * @brief version number
   */
  uint32_t m_version; // NOLINT(build/unsigned)

  /**
   * Produces the JSON string that corresponds to the specified source_id_path map
   */
  static std::string get_json_string(const source_id_path_map_t& source_id_path_map);

  /**
   * Produces the JSON string that corresponds to the specified source_id_geo_id map
   */
  static std::string get_json_string(const source_id_geo_id_map_t& source_id_geo_id_map);

  /**
   * Writes the specified attribute name and value to the specified HightFive File or Group.
   */
  template<typename C, typename T>
  static void write_attribute(HighFive::AnnotateTraits<C>& h5annt, const std::string& name, T value);
};

template<typename C, typename T>
void
HDF5SourceIDHandler::write_attribute(HighFive::AnnotateTraits<C>& h5annt, const std::string& name, T value)
{
  if (!(h5annt.hasAttribute(name)))
    h5annt.createAttribute(name, value);
  //else
  //  ers::warning(HDF5AttributeExists(ERS_HERE, name));
}


} // namespace hdf5libs
} // namespace dunedaq

#endif // HDF5LIBS_INCLUDE_HDF5LIBS_HDF5SOURCEIDHANDLER_HPP_
