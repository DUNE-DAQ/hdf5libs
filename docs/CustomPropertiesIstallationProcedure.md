This tutorial is to locally install HighFive with Direct mode AND multi threading. 
If you want only oDirect mode, skip to [Install hdf5 v1.14](#install-hdf5-v114)

# install open MPI
source the environment before installing

## Download
[last stable version : 4.1.5](https://www.open-mpi.org/software/ompi/v4.1/)
```
wget https://download.open-mpi.org/release/open-mpi/v4.1/openmpi-4.1.5.tar.gz
tar -xzvf openmpi-4.1.5.tar.gz
cd openmpi-4.1.5
```

## Build
see INSTALL file for details
```
mkdir build
cd build
../configure --prefix=/where/to/install
make all install
OR parrallel builds :
make -j X all
make install
```

## add to PATH
```
export PATH=$PATH:/path/to/bin/dir/
in my case
export PATH=$PATH:/home/ljoly/openmpi-4.1.5/build/bin
```

# Install hdf5 v1.14
source the environment before installing
https://portal.hdfgroup.org/display/support/Building+HDF5+with+CMake

## Download CMake-hdf5-1.14.2.tar.gz

[documentation release notes here](https://portal.hdfgroup.org/display/support/HDF5+1.14.2#files)
```
wget https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.14/hdf5-1.14.2/src/CMake-hdf5-1.14.2.tar.gz
tar -xzvf CMake-hdf5-1.14.2.tar.gz
cd CMake-hdf5-1.14.2
```

## modify HDF5options.cmake
DHDF5_BUILD_JAVA and DHDF5_BUILD_CPP_LIB options are not compatible with DHDF5_ENABLE_PARALLEL option.
For Direct mode we need : 

to add
```
set (ADD_BUILD_OPTIONS "${ADD_BUILD_OPTIONS} -DHDF5_ENABLE_DIRECT_VFD:BOOL=ON")
```

and to modify if you want multithreading
```
set (ADD_BUILD_OPTIONS "${ADD_BUILD_OPTIONS} -DHDF5_ENABLE_PARALLEL:BOOL=ON")
set (ADD_BUILD_OPTIONS "${ADD_BUILD_OPTIONS} -DHDF5_ENABLE_THREADSAFE:BOOL=ON")
```

<details>
  <summary>Click here to see all options</summary>

```
========================================================================
VI. CMake Option Defaults for HDF5
========================================================================

In the options listed below, there are three columns of information:
Option Name, Option Description, and Option Default.
The config/cmake/cacheinit.cmake file can override the following values.

---------------- General Build Options ---------------------
BUILD_SHARED_LIBS  "Build Shared Libraries"    ON
BUILD_STATIC_LIBS  "Build Static Libraries"    ON
BUILD_STATIC_EXECS "Build Static Executables"  OFF
BUILD_TESTING      "Build HDF5 Unit Testing"   ON
if (WINDOWS)
  DISABLE_PDB_FILES "Do not install PDB files" OFF

---------------- HDF5 Build Options ---------------------
HDF5_BUILD_CPP_LIB      "Build HDF5 C++ Library"          OFF
HDF5_BUILD_EXAMPLES     "Build HDF5 Library Examples"     ON
HDF5_BUILD_FORTRAN      "Build FORTRAN support"           OFF
HDF5_BUILD_JAVA         "Build JAVA support"              OFF
HDF5_BUILD_HL_LIB       "Build HIGH Level HDF5 Library"   ON
HDF5_BUILD_TOOLS        "Build HDF5 Tools"                ON
HDF5_BUILD_HL_TOOLS     "Build HIGH Level HDF5 Tools"     ON
HDF5_BUILD_HL_GIF_TOOLS "Build HIGH Level HDF5 GIF Tools" OFF

---------------- HDF5 Folder Build Options ---------------------
Defaults relative to $<INSTALL_PREFIX>
HDF5_INSTALL_BIN_DIR      "bin"
HDF5_INSTALL_LIB_DIR      "lib"
HDF5_INSTALL_INCLUDE_DIR  "include"
HDF5_INSTALL_MODULE_DIR   "mod"
HDF5_INSTALL_CMAKE_DIR    "cmake"
if (MSVC)
  HDF5_INSTALL_DATA_DIR   "."
else ()
  HDF5_INSTALL_DATA_DIR   "share"
HDF5_INSTALL_DOC_DIR      "HDF5_INSTALL_DATA_DIR"

Defaults as defined by the `GNU Coding Standards`
HDF5_INSTALL_BIN_DIR      "bin"
HDF5_INSTALL_LIB_DIR      "lib"
HDF5_INSTALL_INCLUDE_DIR  "include"
HDF5_INSTALL_MODULE_DIR   "HDF5_INSTALL_INCLUDE_DIR/mod"
HDF5_INSTALL_CMAKE_DIR    "HDF5_INSTALL_LIB_DIR/cmake"
HDF5_INSTALL_DATA_DIR     "share"
HDF5_INSTALL_DOC_DIR      "HDF5_INSTALL_DATA_DIR/doc/hdf5"

---------------- HDF5 Advanced Options ---------------------
HDF5_USE_GNU_DIRS              "TRUE to use GNU Coding Standard install directory variables,
                                FALSE to use historical settings"                                FALSE
ONLY_SHARED_LIBS               "Only Build Shared Libraries"                                     OFF
ALLOW_UNSUPPORTED              "Allow unsupported combinations of configure options"             OFF
HDF5_EXTERNAL_LIB_PREFIX       "Use prefix for custom library naming."                           ""
HDF5_DISABLE_COMPILER_WARNINGS "Disable compiler warnings"                                       OFF
HDF5_ENABLE_ALL_WARNINGS       "Enable all warnings"                                             OFF
HDF5_ENABLE_CODESTACK          "Enable the function stack tracing (for developer debugging)."    OFF
HDF5_ENABLE_COVERAGE           "Enable code coverage for Libraries and Programs"                 OFF
HDF5_ENABLE_DEBUG_APIS         "Turn on extra debug output in all packages"                      OFF
HDF5_ENABLE_DEPRECATED_SYMBOLS "Enable deprecated public API symbols"                            ON
HDF5_ENABLE_DIRECT_VFD         "Build the Direct I/O Virtual File Driver"                        OFF
HDF5_ENABLE_EMBEDDED_LIBINFO   "embed library info into executables"                             ON
HDF5_ENABLE_PARALLEL           "Enable parallel build (requires MPI)"                            OFF
HDF5_ENABLE_PREADWRITE         "Use pread/pwrite in sec2/log/core VFDs in place of read/write (when available)" ON
HDF5_ENABLE_TRACE              "Enable API tracing capability"                                   OFF
HDF5_ENABLE_USING_MEMCHECKER   "Indicate that a memory checker is used"                          OFF
HDF5_GENERATE_HEADERS          "Rebuild Generated Files"                                         ON
HDF5_BUILD_GENERATORS          "Build Test Generators"                                           OFF
HDF5_JAVA_PACK_JRE             "Package a JRE installer directory"                               OFF
HDF5_NO_PACKAGES               "Do not include CPack Packaging"                                  OFF
HDF5_PACK_EXAMPLES             "Package the HDF5 Library Examples Compressed File"               OFF
HDF5_PACK_MACOSX_FRAMEWORK     "Package the HDF5 Library in a Frameworks"                        OFF
HDF5_BUILD_FRAMEWORKS          "TRUE to build as frameworks libraries,
                                FALSE to build according to BUILD_SHARED_LIBS"                   FALSE
HDF5_PACKAGE_EXTLIBS           "CPACK - include external libraries"                              OFF
HDF5_STRICT_FORMAT_CHECKS      "Whether to perform strict file format checks"                    OFF
DEFAULT_API_VERSION            "Enable default API (v16, v18, v110, v112, v114)"                 "v114"
HDF5_USE_FOLDERS               "Enable folder grouping of projects in IDEs."                     ON
HDF5_WANT_DATA_ACCURACY        "IF data accuracy is guaranteed during data conversions"          ON
HDF5_WANT_DCONV_EXCEPTION      "exception handling functions is checked during data conversions" ON
HDF5_ENABLE_THREADSAFE         "Enable Threadsafety"                                             OFF
HDF5_MSVC_NAMING_CONVENTION    "Use MSVC Naming conventions for Shared Libraries"                OFF
HDF5_MINGW_STATIC_GCC_LIBS     "Statically link libgcc/libstdc++"                                OFF
if (APPLE)
    HDF5_BUILD_WITH_INSTALL_NAME "Build with library install_name set to the installation path"  OFF
if (CMAKE_BUILD_TYPE MATCHES Debug)
    HDF5_ENABLE_INSTRUMENT     "Instrument The library"                      OFF
if (HDF5_BUILD_FORTRAN)
    HDF5_INSTALL_MOD_FORTRAN "Copy FORTRAN mod files to include directory (NO SHARED STATIC)" SHARED
    if (BUILD_SHARED_LIBS AND BUILD_STATIC_LIBS)         default HDF5_INSTALL_MOD_FORTRAN is SHARED
    if (BUILD_SHARED_LIBS AND NOT BUILD_STATIC_LIBS)     default HDF5_INSTALL_MOD_FORTRAN is SHARED
    if (NOT BUILD_SHARED_LIBS AND BUILD_STATIC_LIBS)     default HDF5_INSTALL_MOD_FORTRAN is STATIC
    if (NOT BUILD_SHARED_LIBS AND NOT BUILD_STATIC_LIBS) default HDF5_INSTALL_MOD_FORTRAN is SHARED
HDF5_BUILD_DOC                 "Build documentation"                                OFF
HDF5_ENABLE_ANALYZER_TOOLS     "enable the use of Clang tools"                      OFF
HDF5_ENABLE_SANITIZERS         "execute the Clang sanitizer"                        OFF
HDF5_ENABLE_FORMATTERS         "format source files"                                OFF
HDF5_DIMENSION_SCALES_NEW_REF  "Use new-style references with dimension scale APIs" OFF

---------------- HDF5 Advanced Test Options ---------------------
if (BUILD_TESTING)
    HDF5_TEST_SERIAL               "Execute non-parallel tests"                                   ON
    HDF5_TEST_TOOLS                "Execute tools tests"                                          ON
    HDF5_TEST_EXAMPLES             "Execute tests on examples"                                    ON
    HDF5_TEST_SWMR                 "Execute SWMR tests"                                           ON
    HDF5_TEST_PARALLEL             "Execute parallel tests"                                       ON
    HDF5_TEST_FORTRAN              "Execute fortran tests"                                        ON
    HDF5_TEST_CPP                  "Execute cpp tests"                                            ON
    HDF5_TEST_JAVA                 "Execute java tests"                                           ON
    HDF_TEST_EXPRESS               "Control testing framework (0-3)"                              "3"
    HDF5_TEST_PASSTHROUGH_VOL      "Execute tests with different passthrough VOL connectors"      OFF
    if (HDF5_TEST_PASSTHROUGH_VOL)
        HDF5_TEST_FHEAP_PASSTHROUGH_VOL "Execute fheap test with different passthrough VOL connectors" ON
    HDF5_TEST_VFD                  "Execute tests with different VFDs"                            OFF
    if (HDF5_TEST_VFD)
        HDF5_TEST_FHEAP_VFD        "Execute fheap test with different VFDs"                       ON
    TEST_SHELL_SCRIPTS             "Enable shell script tests"                                    ON

---------------- External Library Options ---------------------
HDF5_ALLOW_EXTERNAL_SUPPORT "Allow External Library Building (NO GIT TGZ)"        "NO"
HDF5_ENABLE_PLUGIN_SUPPORT  "Enable PLUGIN Filters"                               OFF
HDF5_ENABLE_SZIP_SUPPORT    "Use SZip Filter"                                     OFF
HDF5_ENABLE_Z_LIB_SUPPORT   "Enable Zlib Filters"                                 OFF
PLUGIN_USE_EXTERNAL         "Use External Library Building for PLUGINS"           0
ZLIB_USE_EXTERNAL           "Use External Library Building for ZLIB"              0
SZIP_USE_EXTERNAL           "Use External Library Building for SZIP"              0
if (HDF5_ENABLE_SZIP_SUPPORT)
    HDF5_ENABLE_SZIP_ENCODING "Use SZip Encoding"                                 OFF
if (WINDOWS)
    H5_DEFAULT_PLUGINDIR    "%ALLUSERSPROFILE%/hdf5/lib/plugin"
else ()
    H5_DEFAULT_PLUGINDIR    "/usr/local/hdf5/lib/plugin"
endif ()
if (BUILD_SZIP_WITH_FETCHCONTENT)
    LIBAEC_TGZ_ORIGPATH       "Use LIBAEC from original location"        "https://github.com/MathisRosenhauer/libaec/releases/download/v1.0.6/libaec-1.0.6.tar.gz"
    LIBAEC_TGZ_ORIGNAME       "Use LIBAEC from original compressed file" "libaec-v1.0.6.tar.gz"
    LIBAEC_USE_LOCALCONTENT   "Use local file for LIBAEC FetchContent"    OFF
if (BUILD_ZLIB_WITH_FETCHCONTENT)
    ZLIB_TGZ_ORIGPATH         "Use ZLIB from original location"        "https://github.com/madler/zlib/releases/download/v1.2.13"
    ZLIB_TGZ_ORIGNAME         "Use ZLIB from original compressed file" "zlib-1.2.13.tar.gz"
    ZLIB_USE_LOCALCONTENT     "Use local file for ZLIB FetchContent"    OFF

NOTE:
  The BUILD_STATIC_EXECS ("Build Static Executables") option is only valid
  on some unix operating systems. It adds the "-static" flag to cflags. This
  flag is not available on windows and some modern linux systems will
  ignore the flag.

NOTE:
  The HDF5_USE_GNU_DIRS option is usually recommended for linux platforms, but may
  be useful on other platforms. See the CMake documentation for more details.

  ---------------- Unsupported Library Options ---------------------
    The threadsafe, C++ and Java interfaces are not compatible
    with the HDF5_ENABLE_PARALLEL option.
    Unless ALLOW_UNSUPPORTED has been specified,
    the following options must be disabled:
        HDF5_ENABLE_THREADSAFE, HDF5_BUILD_CPP_LIB, HDF5_BUILD_JAVA

    The high-level, C++, Fortran and Java interfaces are not compatible
    with the HDF5_ENABLE_THREADSAFE option because the lock is not hoisted
    into the higher-level API calls.
    Unless ALLOW_UNSUPPORTED has been specified,
    the following options must be disabled:
    HDF5_BUILD_HL_LIB, HDF5_BUILD_CPP_LIB, HDF5_BUILD_FORTRAN, HDF5_BUILD_JAVA
```

</details>

## modify build-unix.sh
Only to enable multithreading (MPI)
```
ctest -S HDF5config.cmake,BUILD_GENERATOR=Unix,MPI=1 -C Release -V -O hdf5.log
```

Guide for further MPI installation and configuration description
```
https://github.com/HDFGroup/hdf5doc/blob/master/RFCs/HDF5_Library/VFD_Subfiling/user_guide/HDF5_Subfiling_VFD_User_s_Guide.pdf
```

## Build hdf5
```
./build-unix.sh
```
you can check correct output in hdf5.log file, 100% of tests must pass.

then install :
```
./HDF5-1.14.2-Linux.sh
```

follow the installation wizard or press space until [yn] question then y y

# HighFive installation
source the environment before installing
https://github.com/BlueBrain/HighFive

## Download
```
git clone --recursive https://github.com/BlueBrain/HighFive.git HighFive
```

## Configure and Install

DHIGHFIVE_PARALLEL_HDF5=On if multithreading enabled.

```
cmake -DHIGHFIVE_EXAMPLES=Off \
      -DHIGHFIVE_USE_BOOST=On \
      -DHIGHFIVE_PARALLEL_HDF5=On \
      -DHIGHFIVE_UNIT_TESTS=Off \
      -DCMAKE_INSTALL_PREFIX=/path/to/install/HighFive/folder \
      -DHDF5_DIR=/path/to/hdf5/cmake/folder/to/be/sure \
      -B HighFive/build \
      HighFive

for me :
cmake -DHIGHFIVE_EXAMPLES=Off \
      -DHIGHFIVE_USE_BOOST=On \
      -DHIGHFIVE_PARALLEL_HDF5=Off \
      -DHIGHFIVE_UNIT_TESTS=Off \
      -DCMAKE_INSTALL_PREFIX=/home/ljoly/HighFive/build \
      -DHDF5_DIR=/home/ljoly/hdf5_folder/CMake-hdf5-1.14.2/HDF5-1.14.2-Linux/HDF_Group/HDF5/1.14.2/cmake \
      -B HighFive/build \
      HighFive  
```
then install :
```
cmake --build HighFive/build
cmake --install HighFive/build
```

# hdf5lib tesing new functionnalities :

## Compile
in CMakeLists.txt put OFF to enable local packages
```
option(WITH_HIGHFIVE_AS_PACKAGE "HIGHFIVE externals as a dunedaq package" OFF)
option(WITH_HDF5_AS_PACKAGE "HDF5 externals as a dunedaq package" OFF)
```

declare custom HighFive AND hdf5 paths: (in env.sh file)
```
export HIGHFIVE_INC=/home/ljoly/HighFive/build/include
export HDF5_INC=/home/ljoly/hdf5_folder/CMake-hdf5-1.14.2/HDF5-1.14.2-Linux/HDF_Group/HDF5/1.14.2/include
export HDF5_LIB=/home/ljoly/hdf5_folder/CMake-hdf5-1.14.2/HDF5-1.14.2-Linux/HDF_Group/HDF5/1.14.2/lib
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/ljoly/hdf5_folder/CMake-hdf5-1.14.2/HDF5-1.14.2-Linux/HDF_Group/HDF5/1.14.2/lib
```

## Entering the fun

You can find a full example in test/apps/HighFive_full_bench.cpp test file.

You can declare from HighFive a RawPropertyList to add any property from hdf5
```
// auto access_props = HighFive::FileAccessProps::Default(); // OLD
auto access_props = HighFive::RawPropertyList<HighFive::PropertyType::FILE_ACCESS>(); // NEW
```
here, we are making a HighFive::PropertyType::FILE_ACCESS property list for direct write. Then we add the properties like usual :
```
// Adding custom properties
size_t alignment_direct = 524288;     // 512K
size_t block_size_direct = 524288;    // 512K
size_t cbuf_size_direct = 1073741824; // 1G
access_props.add(H5Pset_fapl_direct, alignment_direct, block_size_direct, cbuf_size_direct);
// hid_t fapl_id = access_props.getId();
```
with add function : 
```
/// RawPropertyLists are to be used when advanced H5 properties
/// are desired and are not part of the HighFive API.
/// Therefore this class is mainly for internal use.
template <PropertyType T>
class RawPropertyList: public PropertyList<T> {
  public:
    template <typename F, typename... Args>
    void add(const F& funct, const Args&... args);
};
```

And finnally adding the property list :
```
HighFive::File file(file_name,
                        HighFive::File::ReadWrite | HighFive::File::Create |
                            HighFive::File::Truncate,
                        create_props,
                        access_props);
```

For multithreading example see HighFive examples :
- https://github.com/BlueBrain/HighFive/blob/master/src/examples/parallel_hdf5_independent_io.cpp
- https://github.com/BlueBrain/HighFive/blob/master/src/examples/parallel_hdf5_collective_io.cpp

## Some very simple test results

Using :
- No HighFive chunking
- Early allocation mode
- RAID 4*NVMe (Seagate FireCuda 520 SSD) mounted folder
- Average over 5 tests : 8589986560 bytes file
- Using HighFive_full_bench.cpp test

|      Writing Time Default (ms)      |  Reading Time Default (ms) |      Write Time With oDirect (ms)     |  Read Time With oDirect (ms) |
|:-------------------------:|:-------------------------:|:-------------------------:|:-------------------------:|
| 4346 | 2095 | 2463 | 3242 |