/**
 * @file Issues.hpp ERS issues of the hdf5libs package
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef HDF5LIBS_INCLUDE_HDF5LIBS_ISSUES_HPP_
#define HDF5LIBS_INCLUDE_HDF5LIBS_ISSUES_HPP_

#include "ers/Issue.hpp"

#include <cstdint>
#include <string>

namespace dunedaq {

// Disable coverage checking LCOV_EXCL_START

ERS_DECLARE_ISSUE(hdf5libs,
                  DeprecatedUsage,
                  func_name << " is deprecated. " << message,
                  ((std::string)func_name)((std::string)message))

ERS_DECLARE_ISSUE(hdf5libs,
                  FileOpenFailed,
                  "Issue when opening file " << file << ": " << message,
                  ((std::string)file)((std::string)message))

ERS_DECLARE_ISSUE(hdf5libs,
                  IncompatibleOpenFlags,
                  "Issue when opening file " << file << ": "
                                             << "bad open flags " << open_flags,
                  ((std::string)file)((unsigned)open_flags))

ERS_DECLARE_ISSUE(hdf5libs,
                  MissingFileLayout,
                  "No DUNEDAQ FileLayout information available." << " Assigning version " << version,
                  ((uint32_t)version)) // NOLINT(build/unsigned)

ERS_DECLARE_ISSUE(hdf5libs,
                  IncompatibleFileLayoutVersion,
                  "FileLayout version incompatibility. Found version " << version << " but min allowed version is "
                                                                       << min_allowed << " and max allowed version is "
                                                                       << max_allowed,
                  ((uint32_t)version)((uint32_t)min_allowed)((uint32_t)max_allowed)) // NOLINT(build/unsigned)

ERS_DECLARE_ISSUE(hdf5libs,
                  BadRecordType,
                  "Record type attribute " << rt_attr << " does not match file layout config record name prefix "
                                           << rt_fl,
                  ((std::string)rt_attr)((std::string)rt_fl))

ERS_DECLARE_ISSUE(hdf5libs,
                  WrongRecordTypeRequested,
                  "Record type requested " << rname << " does not match file layout config record name prefix "
                                           << rt_fl,
                  ((std::string)rname)((std::string)rt_fl))

ERS_DECLARE_ISSUE(hdf5libs,
                  RecordIDNotFound,
                  "Record ID with record number=" << rec_num << " and sequence number=" << seq_num << " not found.",
                  ((uint64_t)rec_num)((uint16_t)seq_num)) // NOLINT(build/unsigned)

ERS_DECLARE_ISSUE(hdf5libs,
                  SourceIDNotFound,
                  "SourceID " << source_id << " not found in record with record number=" << rec_num
                              << " and sequence number=" << seq_num << ".",
                  ((std::string)source_id)((uint64_t)rec_num)((uint16_t)seq_num)) // NOLINT(build/unsigned)

ERS_DECLARE_ISSUE(hdf5libs,
                  FileCloseFailed,
                  "Issue when closing file " << file << ": " << message,
                  ((std::string)file)((std::string)message))

ERS_DECLARE_ISSUE(hdf5libs, InvalidHDF5Group, "Group " << name << " is invalid.", ((std::string)name))

ERS_DECLARE_ISSUE(hdf5libs,
                  InvalidHDF5Dataset,
                  "The HDF5 Dataset associated with name \"" << data_set << "\" is invalid. (file = " << filename
                                                             << ")",
                  ((std::string)data_set)((std::string)filename))

ERS_DECLARE_ISSUE(hdf5libs, InvalidHDF5Attribute, "Attribute " << name << " not found.", ((std::string)name))

ERS_DECLARE_ISSUE(hdf5libs, HDF5AttributeExists, "Attribute " << name << " already exists.", ((std::string)name))

ERS_DECLARE_ISSUE(hdf5libs,
                  InvalidFragmentTypeString,
                  "Fragment type name \"" << name << "\" does not map to a valid type.",
                  ((std::string)name))

ERS_DECLARE_ISSUE(hdf5libs,
                  InvalidSubdetectorString,
                  "Subdetector name \"" << name << "\" does not map to a valid detector ID.",
                  ((std::string)name))

ERS_DECLARE_ISSUE(hdf5libs,
                  LayoutDoesNotMatchRecordType,
                  "The file layout's record name prefix \"" << prefix << "\" does not match the record type \""
                                                            << record_type << "\" of file " << file,
                  ((std::string)prefix)((std::string)record_type)((std::string)file))

// Re-enable coverage checking LCOV_EXCL_STOP

} // namespace dunedaq

#endif // HDF5LIBS_INCLUDE_HDF5LIBS_ISSUES_HPP_
