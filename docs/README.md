## Overview

The _hdflibs_ repository contains the classes that are used for interfacing between _dunedaq_ data applications (writers and readers) and the [HighFive](https://github.com/BlueBrain/HighFive) library to read/write HDF5 files. 

There are two main classes in use:
- [`HDF5FileLayout`](###HDF5FileLayout) governs the layout of DUNE DAQ raw data files, including configurable parameters for the names of groups and datasets (_e.g._ how many digits to use for numbers in names, what to call the groups for TPC data, its underlying regions, etc.), and it includes member functions that use the layout to allow for construction of group and dataset names.
- [`HDF5RawDataFile`](###HDF5RawDataFile) is the main interface for writing and reading HDF files, containing functions to write data and attributres to the files, and functions for reading back that data and attributes, as well as some utilities for file investigation.

More details on those classes are in the sections below, but some important interface points:
* `HDF5RawDataFile` handles only one file at a time. It opens it on construction, and closes it on deletion. If writing multiple files, one will need multiple `HDF5RawDataFile` objects.
* There is no handling for conditions on if/when data should be written to a file, only where in that file it should be written. It is the responsibility of data writer applications to handle conditions on when to switch to a new file (_e.g._ when reaching maximum file size, or having written a desired number of events). 


###HDF5FileLayout


###HDF5RawDataFile


### Version 1 (Latest) Notes



#### <details><summary>Version 0 Notes</summary>
<p>
Some details here on things to come.
</p>
</details>

