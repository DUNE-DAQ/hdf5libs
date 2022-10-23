from ._daq_hdf5libs_py import *

class HDF5RawDataFile(_HDF5RawDataFile):

    #return sorted record numbers here
    def get_all_record_ids(self):
        return sorted(super().get_all_record_ids())
    def get_all_trigger_record_ids(self):
        return sorted(super().get_all_trigger_record_ids())
    def get_all_timeslice_ids(self):
        return sorted(super().get_all_timeslice_ids())
