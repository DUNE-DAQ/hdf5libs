###
# @file HDF5LIBS_TestWriter.py
#
# Demo of HDF5 file writer.
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
import detdataformats

import h5py


def print_usage():
    print("Usage: HDF5LIBS_TestWriter <configuration_file> <hardware_map_file> <output_file_name>")


def main():
    args = sys.argv

    if(len(sys.argv) != 4):
        print_usage()
        sys.exit(1)

    app_name = args[0]
    ifile_name = args[1]
    hw_map_file_name = args[2]
    ofile_name = args[3]

    # read in configuration using h5py
    #h5_file = HDF5RawDataFile(ifile_name)

    




    # get file_layout config using h5py







    


if __name__ == "__main__":
    main()


