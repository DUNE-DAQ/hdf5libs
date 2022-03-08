###
# @file HDF5LIBS_TestReader.py
#
# Demo of HDF5 file reader.
# frames.
#
#
# This is part of the DUNE DAQ Software Suite, copyright 2020.
# Licensing/copyright details are in the COPYING file that you should have
# received with this code.
#

import sys

from hdf5libs import HDF5RawDataFile
import daqdataformats

import h5py

def print_usage():
    print("Usage: HDF5LIBS_TestReader.py <input_file_name>")

def main():
    args = sys.argv
    
    if( len(sys.argv)!=2 ):
        print_usage()
        sys.exit(1)

    ifile_name = args[1]
    h5_file = HDF5RawDataFile(ifile_name)

    #get attributes using h5py
    #this is a bit messy at the moment: should try to combine interfaces here
    h5py_file = h5py.File(ifile_name,'r')
    for attr in h5py_file.attrs.items(): print ("File Attribute ",attr[0]," = ",attr[1])

    #get type of record
    record_type = h5_file.get_record_type()
    
    records = h5_file.get_all_record_numbers()
    print("Number of records: %d"%len(records))
    print("\tRecord numbers: ",records)

    all_datasets = h5_file.get_dataset_paths()
    print("All datasets found:")
    for d in all_datasets: print(d)

    all_rh_paths = h5_file.get_record_header_dataset_paths()
    print("Record Headers:")
    for d in all_rh_paths: 
        if record_type=="TriggerRecord":
            trh = h5_file.get_trh(d)
            print(d,": ",trh.get_trigger_number(),
                  trh.get_sequence_number(),trh.get_trigger_timestamp())
        elif record_type=="TimeSlice":
            tsh = h5_file.get_tsh(d)
            print(d,": ",tsh.timeslice_number)

    all_frag_paths = h5_file.get_all_fragment_dataset_paths()
    print("Fragments:")
    for d in all_frag_paths:
        frag = h5_file.get_frag(d)
        geoid = (frag.get_element_id().system_type,
                 frag.get_element_id().region_id,
                 frag.get_element_id().element_id)
        print(d,": ",frag.get_trigger_number(),
              frag.get_sequence_number(),geoid)

    sys.exit(0)

if __name__=="__main__":
    main()
