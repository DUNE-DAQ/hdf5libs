#include "hdf5libs/DataRecordGroupType.hpp"

namespace dunedaq{
namespace hdf5libs{

  const std::map<DataRecordGroupTypeID,std::string> 
  DataRecordGroupType::s_group_name_by_type = 
    {
      {DataRecordGroupTypeID::kTriggerRecordHeader,"TriggerRecordHeader"},
      {DataRecordGroupTypeID::kTPC,"TPC"},
      {DataRecordGroupTypeID::kPDS,"PDS"},
      {DataRecordGroupTypeID::kTrigger,"Trigger"},
      {DataRecordGroupTypeID::kTPC_TP,"TPC_TP"},
      {DataRecordGroupTypeID::kNDLArTPC,"NDLArTPC"},
      {DataRecordGroupTypeID::kInvalid,"Invalid"}
    };
  
  const std::map<DataRecordGroupTypeID,std::string> 
  DataRecordGroupType::s_region_prefix_by_type = 
    {
      {DataRecordGroupTypeID::kTriggerRecordHeader,""},
      {DataRecordGroupTypeID::kTPC,"APA"},
      {DataRecordGroupTypeID::kPDS,"Region"},
      {DataRecordGroupTypeID::kTrigger,"Region"},
      {DataRecordGroupTypeID::kTPC_TP,"TP_APA"},
      {DataRecordGroupTypeID::kNDLArTPC,"Region"},
      {DataRecordGroupTypeID::kInvalid,"Invalid"}
    };
  
  const std::map<DataRecordGroupTypeID,std::string> 
  DataRecordGroupType::s_element_prefix_by_type = 
    {
      {DataRecordGroupTypeID::kTriggerRecordHeader,""},
      {DataRecordGroupTypeID::kTPC,"Link"},
      {DataRecordGroupTypeID::kPDS,"Element"},
      {DataRecordGroupTypeID::kTrigger,"Element"},
      {DataRecordGroupTypeID::kTPC_TP,"Link"},
      {DataRecordGroupTypeID::kNDLArTPC,"Element"},
      {DataRecordGroupTypeID::kInvalid,"Invalid"}
    };
  
  DataRecordGroupTypeID 
  DataRecordGroupType::id_from_string(std::string name) const
  {

    for(auto it : s_group_name_by_type){
      if(name == it.second)
	return (it.first);
    }
    return DataRecordGroupTypeID::kInvalid;
  }
  
  
  std::ostream& operator<<(std::ostream& os, const DataRecordGroupType& gt)
  {
    os << gt.get_id() << "('" << gt.get_group_name() << "')";
    return os;
  }
  
}
}
