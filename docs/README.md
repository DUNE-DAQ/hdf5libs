## Overview

The _hdflibs_ repository contains the classes that are used for interfacing between _dunedaq_ data applications (writers and readers) and the [HighFive](https://github.com/BlueBrain/HighFive) library to read/write HDF5 files. 

There are two main classes in use:
- [`HDF5FileLayout`](###HDF5FileLayout) governs the layout of DUNE DAQ raw data files, including configurable parameters for the names of groups and datasets (_e.g._ how many digits to use for numbers in names, what to call the groups for TPC data, its underlying regions, etc.), and it includes member functions that use the layout to allow for construction of group and dataset names.
- [`HDF5RawDataFile`](###HDF5RawDataFile) is the main interface for writing and reading HDF files, containing functions to write data and attributres to the files, and functions for reading back that data and attributes, as well as some utilities for file investigation.

More details on those classes are in the sections below, but some important interface points:
* `HDF5RawDataFile` handles only one file at a time. It opens it on construction, and closes it on deletion. If writing multiple files, one will need multiple `HDF5RawDataFile` objects.
* There is no handling for conditions on if/when data should be written to a file, only where in that file it should be written. It is the responsibility of data writer applications to handle conditions on when to switch to a new file (_e.g._ when reaching maximum file size, or having written a desired number of events). 

The general structure of written files is as follows:
```
<File-Level-Attributes>

GROUP "DUNEDAQFileLayout" <Attributes>
  GROUP System-Type-Path-Parameters <Attributes>

GROUP Top-Level-Record
  
  DATASET Record-Header
  
  GROUP System-Type-Group
    GROUP Region-Group
      DATASET Element-Data
```
Note that except for the "DUNEDAQFileLayout", the names of datasets and groups are not as shown, and instead are configurable on writing, and later determined by the attributes of the DUNEDAQFileLayout Group.

### HDF5FileLayout
This class defines the file layout of _dunedaq_ raw data files. It receives a `hdf5filelayout::FileLayoutParams` object for configuration, which looks like the following in json:
```
  "file_layout_parameters":{
    "trigger_record_name_prefix": "TriggerRecord",
    "digits_for_trigger_number": 6,
    "digits_for_sequence_number": 0,
    "trigger_record_header_dataset_name": "TriggerRecordHeader",

    "path_param_list":[ {"detector_group_type":"TPC",
                         "detector_group_name":"TPC",
                         "region_name_prefix":"APA",
                         "digits_for_region_number":3,
                         "element_name_prefix":"Link",
                         "digits_for_element_number":2},
                        {"detector_group_type":"PDS",
                         "detector_group_name":"PDS",
                         "region_name_prefix":"Region",
                         "digits_for_region_number":3,
                         "element_name_prefix":"Element",
                         "digits_for_element_number":2} ]
  }
```
Under this configuration, the general file structure will look something like this:
```
<File-Level-Attributes>

GROUP "DUNEDAQFileLayout" <Attributes>
  GROUP "TPC" <Attributes>
  GROUP "PDS" <Attributes>

GROUP "TriggerRecord000001"
  
  DATASET "TriggerRecordHeader"
  
  GROUP "TPC"
    GROUP "APA001"
      DATASET "Link00"
      DATASET "Link01"
    GROUP "APA002"
      DATASET "Link00"
      DATASET "Link01"
    ...
    
  GROUP "PDS"
    GROUP "Region001"
      DATASET "Element01"
      DATASET "Element02"
    ...
```

The configuration information for the file layout are written as attributes to the DUNEDAQFileLayout group and subgroups when a file is created. When a file is later opened to be read, the file layout parameters are automatically extracted from the attributes, and used to populate an `HDF5FileLayout` member of the `HDF5RawDataFile`. If no attributes exist, currently a set of defaults are used.

### HDF5RawDataFile

#### Writing
The constructor for creating a new HDF5RawDataFile for writing looks like this:
```
  HDF5RawDataFile(std::string file_name,
                  daqdataformats::run_number_t run_number,
                  size_t file_index,
                  std::string application_name,
                  const hdf5filelayout::FileLayoutParams& fl_params,
                  unsigned open_flags = HighFive::File::Create);
```
Upon opening the file -- at object construction -- the following attributes are written:
- "run_number" (`daqdataformats::run_number_t`)
- "file_index" (`size_t`)
- "creation_timestamp" (`std::string`, string translation of the number of milliseconds since epoch)
- "application_nam" (`std::string)

alongside the file layout paramters as described [above](###HDF5FileLayout).

Upon closing the file -- at object destruction -- the following attributes are written:
- "recorded_size" (`size_t`, number of bytes written in datasets)
- "closing_timestamp" (`std::string`, string translation of the number of milliseconds since epoch).

The key interface for writing is the `HDF5RawDataFile::write(const daqdataformats::TriggerRecord& tr)` member, which takes a TriggerRecord, creates a group in the HDF5 file for it, and then writes all of the underlying data (`TriggerRecordHeader` and `Fragment`s) to appropriate datasets and subgroups.

#### Reading



### Version 1 (Latest) Notes



#### <details><summary>Version 0 Notes</summary>
<p>
Some details here on things to come.
</p>
</details>

