

# Setup the DUNE DAQ environment


```sh
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

# Run (example)

```sh
./apps/hdf5_demo_reader ../swtest.hdf5 1
```

# Some links on DUNE-DAQ

`https://github.com/DUNE-DAQ/minidaqapp/blob/develop/docs/InstructionsForCasualUsers.md`


# TODO
- Clean up
- Add more HDF5 features
- create a library 

