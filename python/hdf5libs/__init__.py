# rawdatafilelibs registers the backend base classes and the types used by the HDF5 backend bindings
import rawdatafilelibs  # noqa: F401

import warnings

from ._daq_hdf5libs_py import *
from ._daq_hdf5libs_py import _HDF5RawDataFile

# from . import _daq_hdf5libs_py as x
# print(dir(x))


class HDF5RawDataFile(_HDF5RawDataFile):
    """DEPRECATED: use rawdatafilelibs.open(path), whose RawDataFile also has these method names."""

    def __init__(self, *args, **kwargs):
        warnings.warn(
            "hdf5libs.HDF5RawDataFile is deprecated; use rawdatafilelibs.open(), whose RawDataFile "
            "also provides the HDF5RawDataFile method names",
            DeprecationWarning,
            stacklevel=2,
        )
        super().__init__(*args, **kwargs)

    #return sorted record numbers here
    def get_all_record_ids(self):
        return sorted(super().get_all_record_ids())
    def get_all_trigger_record_ids(self):
        return sorted(super().get_all_trigger_record_ids())
    def get_all_timeslice_ids(self):
        return sorted(super().get_all_timeslice_ids())
