## HDF5RawDataFile interface changes for file format version 3

As mentioned in the Overview, the `HDF5RawDataFile` C++ class is the main interface in the _hdf5libs_ package for writing and reading HDF files.

The changes associated with the introduction of _SourceIDs_ in _dunedaq-v3.2.0_ affected the HDF5 file layout and, to a lesser extent, the `HDF5RawDataFile` C++ interface.  The file layout version increased from 2 to 3 as part of this transition.

Many of the methods in the `HDF5RawDataFile` interface did not change for file layout version 3, and those are listed in the first table below.  The second table has rows that list each of the methods that *have* changed and the new method signature.

`HDF5RawDataFile` C++ methods that did *not* change between file format version 2 and version 3:
Note that some detail has been omitted to save space...

| /* constructor for reading */  HDF5RawDataFile(std::string file_name); | slfjllsjf |

`HDF5RawDataFile` C++ interface changes between file format version 2 and version 3:

| Version 2 method | Corresponding Version 3 method |
| ---- | ---- |
| (public) void write(TriggerRecordHeader&); | (private) void write(TriggerRecordHeader&); |
