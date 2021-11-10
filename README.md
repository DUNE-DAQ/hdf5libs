

# Setup the DUNE DAQ environment


```sh
source /cvmfs/dunedaq.opensciencegrid.org/setup_dunedaq.sh

setup_dbt dunedaq-v2.8.2

dbt-workarea-env
```


# Build
```sh
dbt-build.sh
```

# Run (example)

```sh
./apps/hdf5_demo_tpc_decoder [PATH_TO_HDF5_FILE/file.hdf5] [VDColdboxChannelMap|ProtoDUNESP1ChannelMap] [number of events to read]

./apps/hdf5_demo_pd_decoder [PATH_TO_HDF5_FILE/file.hdf5] [number of events to read]
```

# Some links on DUNE-DAQ

`https://github.com/DUNE-DAQ/minidaqapp/blob/develop/docs/InstructionsForCasualUsers.md`


# TODO
- Add more HDF5 features
- ERS issues
