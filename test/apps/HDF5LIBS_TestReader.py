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

    if(len(sys.argv) != 2):
        print_usage()
        sys.exit(1)

    ifile_name = args[1]
    h5_file = HDF5RawDataFile(ifile_name)

    # get attributes using h5py
    # this is a bit messy at the moment: should try to combine interfaces here
    h5py_file = h5py.File(ifile_name, 'r')
    for attr in h5py_file.attrs.items():
        print("File Attribute ", attr[0], " = ", attr[1])

    # get type of record
    record_type = h5_file.get_record_type()

    records = h5_file.get_all_record_ids()
    print("Number of records: %d" % len(records))

    ## accessors for getting all found geo IDs or all dataset paths
    ## note that these take a little while to execute, as there's a lot of string parsing underneath
    
    #all_geo_ids = h5_file.get_all_geo_ids()
    #all_datasets = h5_file.get_dataset_paths()

    #let's do a loop through records now:
    for rid in records[:10]:
        print("Processing record (num,seq): ",rid)

        #get record header datasets
        record_header_dataset = h5_file.get_record_header_dataset_path(rid)
        if record_type=="TriggerRecord":
            trh = h5_file.get_trh(rid)
            print(f"{record_header_dataset}: {trh.get_trigger_number()},{trh.get_sequence_number()},{trh.get_trigger_timestamp()}")
        elif record_type=="TimeSlice":
            tsh = h5_file.get_tsh(rid)
            print(f"{record_header_dataset}: {tsh.timeslice_number}")

        #loop through fragment datasets
        for gid in h5_file.get_geo_ids(rid)[:10]:
            print(gid)
            frag = h5_file.get_frag(rid,gid)
            print(f"\tFragment (rec_num,seq_num)=({frag.get_trigger_number()},{frag.get_sequence_number()})")

    #alternatively, can directly grab a TriggerRecord or TimeSlice object...
    for rid in records[:10]:
        print("Processing record (num,seq): ",rid)

        #declare dummy record
        record = None

        if record_type=="TriggerRecord":
            record = h5_file.get_trigger_record(rid)
            trh = record.get_header_ref()
            print(f"Trigger Record Header: {trh.get_trigger_number()},{trh.get_sequence_number()},{trh.get_trigger_timestamp()}")
        elif record_type=="TimeSlice":
            record = h5_file.get_timeslice(rid)
            tsh = record.get_header()
            print(f"TimeSlice Header: {tsh.timeslice_number}")

        if record==None: break

        #now loop through fragments
        for frag in record.get_fragments_ref()[:10]:
            print(frag.get_element_id())
            print(f"\tFragment (rec_num,seq_num)=({frag.get_trigger_number()},{frag.get_sequence_number()})")


    sys.exit(0)


if __name__ == "__main__":
    main()
