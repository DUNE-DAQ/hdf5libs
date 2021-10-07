

# Setup the DUNE DAQ environment


```sh
cd /afs/cern.ch/user/a/aabedabu/work_public/testing_highfive/work_dir

source /cvmfs/dunedaq.opensciencegrid.org/setup_dunedaq.sh

setup_dbt dunedaq-v2.8.0

dbt-workarea-env
```


# Build
```sh
cd build

cmake ..

make -j
```

# Run

```sh
./apps/demo ../swtest.hdf5 1
```

# Some links 

https://github.com/DUNE-DAQ/minidaqapp/blob/develop/docs/InstructionsForCasualUsers.md


# TODO
- make an HDF5 Reader class
- add the HDF5 Reading function
- create a static/dynamic library 
- Add it to the build system of DUNE-DAQ (ask Alessandro)

