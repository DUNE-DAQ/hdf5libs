## Overview

The _hdflibs_ repository contains the classes that are used for interfacing between _dunedaq_ data applications (writers and readers) and the [HighFive](https://github.com/BlueBrain/HighFive) library to read/write HDF5 files. 

HDF5RawDataFile interface changes between file format version 2 and version 3:

| Version 2 method | Corresponding Version 3 method |
| ---- | ---- |
| (public) void write(TriggerRecordHeader&); | (private) void write(TriggerRecordHeader&); |
