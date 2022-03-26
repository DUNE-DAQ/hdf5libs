from ._daq_hdf5libs_py import *

class HDF5RawDataFile(HDF5RawDataFileCPP):
#    pass

    #return sorted record numbers here
    def get_all_record_ids(self):
        return sorted(super().get_all_record_ids())
    def get_all_trigger_record_ids(self):
        return sorted(super().get_all_trigger_record_ids())
    def get_all_timeslice_ids(self):
        return sorted(super().get_all_timeslice_ids())
    def get_all_record_numbers(self):
        return sorted(super().get_all_record_numbers())
    def get_all_trigger_record_numbers(self):
        return sorted(super().get_all_trigger_record_numbers())
    def get_all_timeslice_numbers(self):
        return sorted(super().get_all_timeslice_numbers())

    #return sorted geo ids too
    def get_all_geo_ids(self):
        return sorted(super().get_all_geo_ids())
    def get_geo_ids(self,*args,**kwargs):
        return sorted(super().get_geo_ids(*args,**kwargs))
