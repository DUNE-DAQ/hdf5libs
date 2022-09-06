## HDF5RawDataFile interface changes for file format version 3

As mentioned in the Overview, the `HDF5RawDataFile` C++ class is the main interface in the _hdf5libs_ package for writing and reading HDF files.

The changes associated with the introduction of _SourceIDs_ in _dunedaq-v3.2.0_ affects the HDF5 file layout and, to a lesser extent, the `HDF5RawDataFile` C++ interface.  The file layout version increases from 2 to 3 as part of this transition.

Many of the methods in the `HDF5RawDataFile` interface do *not* change for file layout version 3, and those are listed in the first table below.  The second table has rows that list each of the methods that *have* changed and the new method signature.  And, the third table lists new methods that become available with the introduction of SourceIDs in the DAQ data.

Note that type `record_id_t` is defined as `std::pair<uint64_t, daqdataformats::sequence_number_t>`.

### `HDF5RawDataFile` C++ methods that did *not* change between file format version 2 and version 3

Note that in some cases, some of the detail of a method signature may have been omitted to save space (e.g. `const` declarations, full namespace details, etc.)  Please see the latest version of the `HDF5RawDataFile.hpp` header file for the full details.

| Unchanged HDF5RawDataFile methods |
| --- | 
| [file read constructor]  HDF5RawDataFile(const std::string& file_name); |
| std::string get_file_name() |
| size_t get_recorded_size() |
| std::string get_record_type() |
| bool is_trigger_record_type() |
| bool is_timeslice_type() |
| HDF5FileLayout get_file_layout() |
| uint32_t get_version() |
| void write(const daqdataformats::TriggerRecord& tr) |
| void write(const daqdataformats::TimeSlice& ts) |
| template<typename T> void write_attribute(const std::string& name, T value); |
| template<typename T> void write_attribute(HighFive::Group& grp, const std::string& name, T value); |
| template<typename T> void write_attribute(HighFive::DataSet& dset, const std::string& name, T value); |
| template<typename T> T get_attribute(const std::string& name); |
| template<typename T> T get_attribute(const HighFive::Group& grp, const std::string& name); |
| template<typename T> T get_attribute(const HighFive::DataSet& dset, const std::string& name); |
| std::vector<std::string> get_dataset_paths(std::string top_level_group_name = ""); |
| record_id_set get_all_record_ids(); |
| record_id_set get_all_trigger_record_ids(); |
| record_id_set get_all_timeslice_ids(); |
| std::set<uint64_t> get_all_record_numbers(); // deprecated |
| std::set<daqdataformats::trigger_number_t> get_all_trigger_record_numbers(); // deprecated |
| std::set<daqdataformats::timeslice_number_t> get_all_timeslice_numbers(); // deprecated |
| std::vector<std::string> get_record_header_dataset_paths(); |
| std::vector<std::string> get_trigger_record_header_dataset_paths(); |
| std::vector<std::string> get_timeslice_header_dataset_paths(); |
| std::string get_record_header_dataset_path(const record_id_t& rid); |
| std::string get_record_header_dataset_path(uint64_t rec_num, daqdataformats::sequence_number_t seq_num; |
| std::string get_trigger_record_header_dataset_path(const record_id_t& rid); |
| std::string get_trigger_record_header_dataset_path(daqdataformats::trigger_number_t trig_num, daqdataformats::sequence_number_t seq_num);
| std::string get_timeslice_header_dataset_path(const record_id_t& rid); |
| std::string get_timeslice_header_dataset_path(const daqdataformats::timeslice_number_t trig_num); |
| std::vector<std::string> get_all_fragment_dataset_paths(); |
| std::vector<std::string> get_fragment_dataset_paths(const record_id_t& rid); |
| std::vector<std::string> get_fragment_dataset_paths(uint64_t rec_num, daqdataformats::sequence_number_t seq_num); |


### `HDF5RawDataFile` C++ interface changes between file format version 2 and version 3:

| Version 2 method | Corresponding Version 3 method |
| ---- | ---- |
| [file write constructor] HDF5RawDataFile(std::string file_name, ..., unsigned open_flags) | HDF5RawDataFile(..., std::shared_ptr&lt;HardwareMapService&gt; hw_map_service, ...) | 
| (public) void write(TriggerRecordHeader&) | (private) HighFive::Group write(TriggerRecordHeader&, HDF5SourceIDHandler::source_id_path_map_t&) |
| (public) void write(TimeSliceHeader&) | (private) HighFive::Group write(TimeSliceHeader&, HDF5SourceIDHandler::source_id_path_map_t&) |
| (public) void write(Fragment&) | (private) HighFive::Group write(Fragment&, HDF5SourceIDHandler::source_id_path_map_t&) |
  
| vector<string> get_fragment_dataset_paths(daqdataformats::GeoID::SystemType type) | vector<string> get_fragment_dataset_paths(detdataformats::DetID::Subdetector subdet) |

| vector<string> get_fragment_dataset_paths(std::string typestring) | vector<string> get_fragment_dataset_paths(string& subdetector_name) |
  
| vector<string> get_fragment_dataset_paths(record_id_t, daqdataformats::GeoID::SystemType) | vector<string> get_fragment_dataset_paths(record_id_t, detdataformats::DetID::Subdetector) |

| vector<string> get_fragment_dataset_paths(record_id_t, std::string typestring) | vector<string> get_fragment_dataset_paths(record_id_t, string& subdetector_name) |


| vector<string> get_fragment_dataset_paths(daqdataformats::GeoID element_id) | 

| vector<string> get_fragment_dataset_paths(daqdataformats::GeoID::SystemType type, uint16_t region_id, uint32_t element_id) |

| vector<string> get_fragment_dataset_paths(string typestring, uint16_t region_id, uint32_t element_id) | 


