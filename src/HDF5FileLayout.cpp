/**
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 *
 */

#include "hdf5libs/HDF5FileLayout.hpp"

#include <string>
#include <vector>
#include <stdexcept>

namespace dunedaq {
namespace hdf5libs {

HDF5FileLayout::HDF5FileLayout(hdf5filelayout::FileLayoutParams conf, uint32_t version) // NOLINT(build/unsigned)
	: m_conf_params(conf)
	, m_version(version)
{
	if (m_version < 2)
		m_conf_params = get_v0_file_layout_params();

	fill_path_params_maps(m_conf_params);

	check_config();
}

void HDF5FileLayout::check_config()
{
	//for now, don't do additional config checks for old versions
	if( m_version < 2 )
		return;

	if(m_conf_params.record_name_prefix.compare("TriggerRecord")==0) {
		if(m_conf_params.digits_for_sequence_number==0) {
			ers::error(FileLayoutSequenceIDsCannotBeZero(ERS_HERE,4));
			m_conf_params.digits_for_sequence_number=4;
		}
	} else if(m_conf_params.record_name_prefix.compare("TimeSlice")==0) {
		if(m_conf_params.digits_for_sequence_number!=0) {
			ers::warning(InvalidSequenceDigits(ERS_HERE,m_conf_params.record_name_prefix,0));
			m_conf_params.digits_for_sequence_number=0;
		}
	} else{
		throw InvalidRecordName(ERS_HERE,m_conf_params.record_name_prefix);
	}
}

hdf5filelayout::PathParams
HDF5FileLayout::get_path_params(daqdataformats::GeoID::SystemType type) const
{
	try{
		return m_path_params_map.at(type);
	}catch(std::out_of_range&) {
		throw FileLayoutUnconfiguredSystemType(ERS_HERE,
		                                       type,daqdataformats::GeoID::system_type_to_string(type));
	}
}

std::string HDF5FileLayout::get_record_number_string(uint64_t record_number, // NOLINT(build/unsigned)
                                                     daqdataformats::sequence_number_t seq_num) const
{
	std::ostringstream record_number_string;

	int width=m_conf_params.digits_for_record_number;

	if(record_number >= m_powers_ten[m_conf_params.digits_for_record_number]) {
		ers::warning(FileLayoutNotEnoughDigitsForPath(ERS_HERE,record_number,m_conf_params.digits_for_record_number));
		width=0;             // tells it to revert to normal width
	}

	record_number_string << m_conf_params.record_name_prefix
	                     << std::setw(width) << std::setfill('0') << record_number;

	if (m_conf_params.digits_for_sequence_number > 0) {

		width=m_conf_params.digits_for_sequence_number;
		if(seq_num >= m_powers_ten[m_conf_params.digits_for_sequence_number]) {
			ers::warning(FileLayoutNotEnoughDigitsForPath(ERS_HERE,seq_num,m_conf_params.digits_for_sequence_number));
			width=0;                   // tells it to revert to normal width
		}
		record_number_string << "." << std::setw(width) << std::setfill('0') << seq_num;
	}

	return record_number_string.str();
}

std::string HDF5FileLayout::get_trigger_number_string(daqdataformats::trigger_number_t trig_num,
                                                      daqdataformats::sequence_number_t seq_num) const
{
	return get_record_number_string(trig_num,seq_num);
}

std::string HDF5FileLayout::get_timeslice_number_string(daqdataformats::timeslice_number_t ts_num) const
{
	return get_record_number_string(ts_num);
}

/**
 * @brief get the correct path for the TriggerRecordHeader
 */
std::vector<std::string> HDF5FileLayout::get_path_elements(const daqdataformats::TriggerRecordHeader& trh) const
{

	std::vector<std::string> path_elements;

	// first the Trigger string
	path_elements.push_back(get_trigger_number_string(trh.get_trigger_number(), trh.get_sequence_number()));

	// then the TriggerRecordHeader dataset name
	path_elements.push_back(m_conf_params.record_header_dataset_name);

	return path_elements;
}

/**
 * @brief get the correct path for the TimeSliceHeader
 */
std::vector<std::string> HDF5FileLayout::get_path_elements(const daqdataformats::TimeSliceHeader& tsh) const
{

	std::vector<std::string> path_elements;

	// first the Trigger string
	path_elements.push_back(get_timeslice_number_string(tsh.timeslice_number));

	// then the TriggerRecordHeader dataset name
	path_elements.push_back(m_conf_params.record_header_dataset_name);

	return path_elements;
}

/**
 * @brief get the correct path for the Fragment
 */
std::vector<std::string> HDF5FileLayout::get_path_elements(const daqdataformats::FragmentHeader& fh) const
{

	std::vector<std::string> path_elements;

	// first the Trigger string
	// note, this still works for TimeSlices through enforced proper configuration of layout parameters
	path_elements.push_back(get_trigger_number_string(fh.trigger_number, fh.sequence_number));

	// then get the path params from our file layout for this type
	auto const& path_params = get_path_params(fh.element_id.system_type);

	// next is the detector group name
	path_elements.push_back(path_params.detector_group_name);

	// then the region
	std::ostringstream region_string;

	int width=path_params.digits_for_region_number;
	if(fh.element_id.region_id >= m_powers_ten[path_params.digits_for_region_number]) {
		ers::warning(FileLayoutNotEnoughDigitsForPath(ERS_HERE,fh.element_id.region_id,path_params.digits_for_region_number));
		width=0;             // tells it to revert to normal width
	}

	region_string << path_params.region_name_prefix << std::setw(width)
	              << std::setfill('0') << fh.element_id.region_id;
	path_elements.push_back(region_string.str());

	// finally the element
	std::ostringstream element_string;

	width=path_params.digits_for_element_number;
	if(fh.element_id.element_id >= m_powers_ten[path_params.digits_for_element_number]) {
		ers::warning(FileLayoutNotEnoughDigitsForPath(ERS_HERE,fh.element_id.element_id,path_params.digits_for_element_number));
		width=0;             // tells it to revert to normal width
	}

	element_string << path_params.element_name_prefix << std::setw(width)
	               << std::setfill('0') << fh.element_id.element_id;
	path_elements.push_back(element_string.str());

	return path_elements;
}

/**
 * @brief get the full path for a record header dataset based on trig/seq number
 */
std::string HDF5FileLayout::get_record_header_path(uint64_t rec_num, // NOLINT (build/unsigned)
                                                   daqdataformats::sequence_number_t seq_num) const
{
	return get_record_number_string(rec_num, seq_num) + "/" +
	       m_conf_params.record_header_dataset_name;
}

/**
 * @brief get the full path for a TriggerRecordHeader dataset based on trig/seq number
 */
std::string HDF5FileLayout::get_trigger_record_header_path(daqdataformats::trigger_number_t trig_num,
                                                           daqdataformats::sequence_number_t seq_num) const
{
	return get_trigger_number_string(trig_num, seq_num) + "/" +
	       m_conf_params.record_header_dataset_name;
}

/**
 * @brief get the full path for a TimeSliceHeader dataset based on ts number
 */
std::string HDF5FileLayout::get_timeslice_header_path(daqdataformats::timeslice_number_t ts_num) const
{
	return get_timeslice_number_string(ts_num) + "/" +
	       m_conf_params.record_header_dataset_name;
}

/**
 * @brief get the full path for a Fragment dataset based on trig/seq number and element ID
 */
std::string HDF5FileLayout::get_fragment_path(uint64_t trig_num, // NOLINT(build/unsigned)
                                              daqdataformats::sequence_number_t seq_num,
                                              daqdataformats::GeoID element_id) const
{

	auto const& path_params = get_path_params(element_id.system_type);

	std::ostringstream path_string;
	path_string << get_trigger_number_string(trig_num, seq_num) << "/" << path_params.detector_group_name << "/"
	            << path_params.region_name_prefix << std::setw(path_params.digits_for_region_number)
	            << std::setfill('0') << element_id.region_id << "/" << path_params.element_name_prefix
	            << std::setw(path_params.digits_for_element_number) << std::setfill('0') << element_id.element_id;
	return path_string.str();
}

/**
 * @brief get the full path for a Fragment dataset based on trig/seq number, give element_id pieces
 */
std::string HDF5FileLayout::get_fragment_path(uint64_t trig_num, // NOLINT(build/unsigned)
                                              daqdataformats::sequence_number_t seq_num,
                                              daqdataformats::GeoID::SystemType type,
                                              uint16_t region_id, // NOLINT(build/unsigned)
                                              uint32_t element_id) const // NOLINT(build/unsigned)
{
	daqdataformats::GeoID gid{ type, region_id, element_id };
	return get_fragment_path(trig_num, seq_num, gid);
}

/**
 * @brief get the full path for a Fragment dataset based on trig/seq number, give element_id pieces
 */
std::string HDF5FileLayout::get_fragment_path(uint64_t trig_num, // NOLINT(build/unsigned)
                                              daqdataformats::sequence_number_t seq_num,
                                              const std::string& typestring,
                                              uint16_t region_id, // NOLINT(build/unsigned)
                                              uint32_t element_id) const // NOLINT(build/unsigned)
{
	daqdataformats::GeoID gid{ daqdataformats::GeoID::string_to_system_type(typestring), region_id, element_id };
	return get_fragment_path(trig_num, seq_num, gid);
}

/**
 * @brief get the path for a Fragment type group based on trig/seq number and type
 */
std::string HDF5FileLayout::get_fragment_type_path(uint64_t trig_num, // NOLINT(build/unsigned)
                                                   daqdataformats::sequence_number_t seq_num,
                                                   daqdataformats::GeoID::SystemType type) const
{
	auto const& path_params = get_path_params(type);

	std::ostringstream path_string;
	path_string << get_trigger_number_string(trig_num, seq_num) << "/" << path_params.detector_group_name;
	return path_string.str();
}

/**
 * @brief get the path for a Fragment type group based on trig/seq number and type
 */
std::string HDF5FileLayout::get_fragment_type_path(uint64_t trig_num, // NOLINT(build/unsigned)
                                                   daqdataformats::sequence_number_t seq_num,
                                                   std::string typestring) const
{
	return get_fragment_type_path(trig_num, seq_num, daqdataformats::GeoID::string_to_system_type(typestring));
}

/**
 * @brief get the path for a Fragment region group based on trig/seq number,type, and region ID
 */
std::string HDF5FileLayout::get_fragment_region_path(uint64_t trig_num, // NOLINT(build/unsigned)
                                                     daqdataformats::sequence_number_t seq_num,
                                                     daqdataformats::GeoID::SystemType type,
                                                     uint16_t region_id) const // NOLINT(build/unsigned)
{
	auto const& path_params = get_path_params(type);

	std::ostringstream path_string;
	path_string << get_trigger_number_string(trig_num, seq_num) << "/" << path_params.detector_group_name << "/"
	            << path_params.region_name_prefix << std::setw(path_params.digits_for_region_number)
	            << std::setfill('0') << region_id;
	return path_string.str();
}

/**
 * @brief get the path for a Fragment region group based on trig/seq number,type, and region ID
 */
std::string HDF5FileLayout::get_fragment_region_path(uint64_t trig_num, // NOLINT(build/unsigned)
                                                     daqdataformats::sequence_number_t seq_num,
                                                     std::string typestring,
                                                     uint16_t region_id) const // NOLINT(build/unsigned)
{
	return get_fragment_region_path(trig_num, seq_num,
	                                daqdataformats::GeoID::string_to_system_type(typestring), region_id);
}

daqdataformats::GeoID
HDF5FileLayout::get_geo_id_from_path_elements(std::vector<std::string> const& path_elements) const
{
	//ignore first path element, which is for the record group
	//second path element is detector name.
	daqdataformats::GeoID::SystemType systype = m_detector_group_name_to_type_map.at(path_elements[1]);

	//get back the path parameters for this system type from the file layout
	auto path_params = get_path_params(systype);

	//third path element is region. remove prefix and translate to number
	auto reg_id = std::stoi( path_elements[2].substr(path_params.region_name_prefix.size()) );

	//fourth path element is element. remove prefix and translate to numbers
	auto ele_id = std::stoi( path_elements[3].substr(path_params.element_name_prefix.size()) );

	return daqdataformats::GeoID(systype,reg_id,ele_id);
}

void HDF5FileLayout::fill_path_params_maps(hdf5filelayout::FileLayoutParams const& flp)
{
	for (auto const& path_param : flp.path_param_list) {
		auto sys_type = daqdataformats::GeoID::string_to_system_type(path_param.detector_group_type);

		if (sys_type == daqdataformats::GeoID::SystemType::kInvalid)
			throw FileLayoutInvalidSystemType(ERS_HERE,path_param.detector_group_type);

		m_path_params_map[sys_type] = path_param;
		m_detector_group_name_to_type_map[path_param.detector_group_name] = sys_type;
	}
}

/**
 * @brief Version0 FileLayout parameters, for backward compatibility
 */
hdf5filelayout::FileLayoutParams HDF5FileLayout::get_v0_file_layout_params()
{
	hdf5filelayout::FileLayoutParams flp;
	flp.record_name_prefix = "TriggerRecord";
	flp.digits_for_record_number = 6;
	flp.digits_for_sequence_number = 0;
	flp.record_header_dataset_name = "TriggerRecordHeader";

	hdf5filelayout::PathParams pp;

	pp.detector_group_type = "TPC";
	pp.detector_group_name = "TPC";
	pp.region_name_prefix = "APA";
	pp.digits_for_region_number = 3;
	pp.element_name_prefix = "Link";
	pp.digits_for_element_number = 2;
	flp.path_param_list.push_back(pp);

	pp.detector_group_type = "PDS";
	pp.detector_group_name = "PDS";
	pp.region_name_prefix = "Region";
	pp.digits_for_region_number = 3;
	pp.element_name_prefix = "Element";
	pp.digits_for_element_number = 2;
	flp.path_param_list.push_back(pp);

	pp.detector_group_type = "NDLArTPC";
	pp.detector_group_name = "NDLArTPC";
	pp.region_name_prefix = "Region";
	pp.digits_for_region_number = 3;
	pp.element_name_prefix = "Element";
	pp.digits_for_element_number = 2;
	flp.path_param_list.push_back(pp);

	pp.detector_group_type = "DataSelection";
	pp.detector_group_name = "Trigger";
	pp.region_name_prefix = "Region";
	pp.digits_for_region_number = 3;
	pp.element_name_prefix = "Element";
	pp.digits_for_element_number = 2;
	flp.path_param_list.push_back(pp);

	return flp;
}


}// namespace hdf5libs
}// namespace dunedaq
