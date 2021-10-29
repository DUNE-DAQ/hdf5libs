

# Setup the DUNE DAQ environment


```sh
source /cvmfs/dunedaq.opensciencegrid.org/setup_dunedaq.sh v2.8.1

setup_dbt dunedaq-v2.8.1

dbt-workarea-env
```


# Build
```sh
dbt-build.sh
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

