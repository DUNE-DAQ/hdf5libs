/**
 * @file HDF5SourceIDMaps.hpp
 *
 * Maps that provide efficient look up of DataSets in records (TriggerRecords or
 * TimeSlices) written in HDF5.
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef HDF5LIBS_INCLUDE_HDF5LIBS_HDF5SOURCEIDMAPS_HPP_
#define HDF5LIBS_INCLUDE_HDF5LIBS_HDF5SOURCEIDMAPS_HPP_

//#include "hdf5libs/hdf5filelayout/Structs.hpp"

//#include "daqdataformats/Fragment.hpp"
#include "daqdataformats/SourceID.hpp"
//#include "daqdataformats/TimeSliceHeader.hpp"
//#include "daqdataformats/TriggerRecordHeader.hpp"
//#include "logging/Logging.hpp"

//#include "nlohmann/json.hpp"

#include <map>
#include <string>
#include <vector>

namespace dunedaq {
namespace hdf5libs {

class HDF5SourceIDMaps
{
public:
  typedef std::map<daqdataformats::SourceID, std::string> source_id_path_map_t;

  /**
   * @brief Constructor
   */
  explicit HDF5SourceIDMaps(uint32_t version = 1); // NOLINT(build/unsigned)

  uint32_t get_version() const noexcept // NOLINT(build/unsigned)
  {
    return m_version;
  }

  void add_source_id_path(const daqdataformats::SourceID& source_id, const std::string hdf5_path);

  source_id_path_map_t get_source_id_path_map()
  {
    return m_source_id_path_map;
  }

private:
  /**
   * @brief version number
   */
  uint32_t m_version; // NOLINT(build/unsigned)

  /**
   * @brief map from SourceID to DataSet path string
   */
  source_id_path_map_t m_source_id_path_map;
};

/**
 * @brief free function to produce the JSON string from the specified source_id_path map
 */
std::string get_json_string(const HDF5SourceIDMaps::source_id_path_map_t& source_id_path_map);

} // namespace hdf5libs
} // namespace dunedaq

#endif // HDF5LIBS_INCLUDE_HDF5LIBS_HDF5SOURCEIDMAPS_HPP_
