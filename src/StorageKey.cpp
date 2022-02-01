/**
 * @file StorageKey.hpp
 *
 * StorageKey class used to identify a given block of data
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "hdf5libs/StorageKey.hpp"

namespace dunedaq {
namespace hdf5libs {

int
StorageKey::get_trigger_number() const
{
  return m_trigger_number;
}

StorageKey::DataRecordGroupType
StorageKey::get_group_type() const
{
  return m_group_type;
}

int
StorageKey::get_region_number() const
{
  return m_region_number;
}

int
StorageKey::get_element_number() const
{
  return m_element_number;
}

} // namespace hdf5libs
} // namespace dunedaq
