/**
 * @file DataRecordGroupType.hpp Collection of the data record types used, and translations for strings
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef HDF5LIBS_INCLUDE_HDF5LIBS_DATARECORDGROUPTYPE_HPP_
#define HDF5LIBS_INCLUDE_HDF5LIBS_DATARECORDGROUPTYPE_HPP_

#include <string>
#include <map>
#include <iostream>

namespace dunedaq {
  namespace hdf5libs {

    /**
     * @brief 
     * 
     */

    enum DataRecordGroupTypeID
    {
      kTriggerRecordHeader = 1,
      kTPC = 2,
      kPDS = 3,
      kTrigger = 4,
      kTPC_TP = 5,
      kNDLArTPC = 6,
      
      kInvalid = 0
    };

    class DataRecordGroupType {

    public:
      
      DataRecordGroupTypeID get_id() const
      { return m_id; }

      static const std::map<DataRecordGroupTypeID,std::string> 
      s_group_name_by_type;

      static const std::map<DataRecordGroupTypeID,std::string> 
      s_region_prefix_by_type; 

      static const std::map<DataRecordGroupTypeID,std::string> 
      s_element_prefix_by_type;

      //for lookup based on paths we may find in files
      DataRecordGroupTypeID id_from_string(std::string name) const;

      inline std::string get_group_name() const
      { return s_group_name_by_type.at(m_id); } 
      inline std::string get_region_prefix() const
      { return s_region_prefix_by_type.at(m_id); }
      inline std::string get_element_prefix() const
      { return s_element_prefix_by_type.at(m_id); }


      DataRecordGroupType()
	: m_id(DataRecordGroupTypeID::kInvalid) {}

      DataRecordGroupType(DataRecordGroupTypeID t)
	: m_id(t) {}

      DataRecordGroupType(std::string gname)
      { m_id = id_from_string(gname); }

    private:
      
      DataRecordGroupTypeID m_id;


    };

    std::ostream& operator<<(std::ostream& os, const DataRecordGroupType& gt);


  } // namespace hdf5libs
} // namespace dunedaq

#endif // HDF5LIBS_INCLUDE_HDF5LIBS_DataRecordGroupType_HPP_
