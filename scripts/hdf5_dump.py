#!/usr/bin/env python3

import argparse
import datetime
import h5py
import struct
import os
import sys


FILELAYOUT_VERSION = 3
# detdataformats/include/detdataformats/DetID.hpp
DETECTOR = {0: 'Unknown', 1: 'DAQ', 2: 'HD_PDS', 3: 'HD_TPC',
            4: 'HD_CRT', 8: 'VD_CathodePDS', 9: 'VD_MembranePDS',
            10: 'VD_BottomTPC', 11: 'VD_TopTPC',
            32: 'ND_LAr', 33: 'ND_GAr'}

# daqdataformats/include/daqdataformats/SourceID.hpp
SUBSYSTEM = {0: 'Unknown', 1: 'DetectorReadout', 2: 'HwSignalsInterface',
             3: 'Trigger', 4: 'TRBuilder'}

DATA_FORMAT = {
    # daqdataformats/include/daqdataformats/TimeSliceHeader.hpp
    "TimeSlice Header": {
        "keys": ['Marker word', 'Version', 'TimeSlice number',                     # I I Q
                 'Run number', "Padding",                                          # I I
                 'Source ID version', 'Source ID subsystem', 'Source ID'],         # H H I
        "size": 32,
        "unpack string": '<2IQ2I2HI'
    },
    # daqdataformats/include/daqdataformats/TriggerRecordHeaderData.hpp
    "TriggerRecord Header": {
        "keys": ['Marker word', 'Version', 'Trigger number',                       # I I Q
                 'Trigger timestamp', 'No. of requested components', 'Run number', # Q Q I
                 'Error bits', 'Trigger type', 'Sequence number',                  # I H H
                 'Max sequence num', 'Padding',                                    # H H
                 'Source ID version', 'Source ID subsystem', 'Source ID'],         # H H I
        "size": 56,
        "unpack string": '<2I3Q2I6HI'
    },
    # daqdataformats/include/daqdataformats/FragmentHeader.hpp
    "Fragment Header":{
        "keys": ['Marker word', 'Version', 'Fragment size', 'Trigger number',      # I I Q Q
                 'Trigger timestamp', 'Window begin', 'Window end', 'Run number',  # Q Q Q I
                 'Error bits', 'Fragment type', 'Sequence number',                 # I I H
                 'Detector',                                                       # H
                 'Source ID version', 'Source ID subsystem', 'Source ID'],         # H H I
        "size": 72,
        "unpack string": '<2I5Q3I4HI'
    },
    # daqdataformats/include/daqdataformats/ComponentRequest.hpp
    "Component Request":{
        "keys": ['Component request version', 'Padding',                           # I I
                 'Source ID version', 'Source ID subsystem', 'Source ID',          # H H I
                 'Begin time', 'End time'],                                        # Q Q
        "size": 32,
        "unpack string": "<2I2HI2Q"
    }
}


class DAQDataFile:
    def __init__(self, name):
        self.name = name
        if os.path.exists(self.name):
            try:
                self.h5file = h5py.File(self.name, 'r')
            except OSError:
                sys.exit(f"ERROR: file \"{self.name}\" couldn't be opened; is it an HDF5 file?")
        else:
            sys.exit(f"ERROR: HDF5 file \"{self.name}\" is not found!")
        # Assume HDf5 files without file attributes field "record_type"
        # are old data files which only contain "TriggerRecord" data.
        self.record_type = 'TriggerRecord'
        self.clock_speed_hz = 50000000.0
        self.records = []
        if 'filelayout_version' in self.h5file.attrs.keys() and \
                self.h5file.attrs['filelayout_version'] == FILELAYOUT_VERSION:
            print(f"INFO: input file matches the supported file layout version: {FILELAYOUT_VERSION}")
        else:
            sys.exit(f"ERROR: this script expects a file layout version {FILELAYOUT_VERSION} but this wasn't confirmed in the HDF5 file \"{self.name}\"")
        if 'record_type' in self.h5file.attrs.keys():
            self.record_type = self.h5file.attrs['record_type']
        for i in self.h5file.keys():
            record = self.Record()
            record.path = i
            self.h5file[i].visititems(record)
            self.records.append(record)

    def __del__(self):
        try:
            self.h5file.close()
        except:
            pass  # OK if the file was never opened

    def set_clock_speed_hz(self, k_clock_speed_hz):
        self.clock_speed_hz = k_clock_speed_hz

    def convert_to_binary(self, binary_file, k_nrecords):
        with open(binary_file, 'wb') as bf:
            n = 0
            for i in self.records:
                if n >= k_nrecords and k_nrecords > 0:
                    break
                dset = self.h5file[i.header]
                idata_array = bytearray(dset[:])
                bf.write(idata_array)
                for j in i.fragments:
                    dset = self.h5file[j]
                    jdata_array = bytearray(dset[:])
                    bf.write(jdata_array)
                n += 1
        return

    def printout(self, k_header_type, k_nrecords, k_list_components=False):
        k_header_type = set(k_header_type)
        if not {"attributes", "all"}.isdisjoint(k_header_type):
            banner_str = " File Attributes "
            print(banner_str.center(80, '='))
            for k in self.h5file.attrs.keys():
                print("{:<30}: {}".format(k, self.h5file.attrs[k]))
        n = 0
        for i in self.records:
            if n >= k_nrecords and k_nrecords > 0:
                break
            if not {"attributes", "all"}.isdisjoint(k_header_type):
                banner_str = " Trigger Record Attributes "
                print(banner_str.center(80, '='))
                for k in self.h5file[i.path].attrs.keys():
                    print("{:<30}: {}".format(k, self.h5file[i.path].attrs[k]))
            if not {"header", "both", "all"}.isdisjoint(k_header_type):
                dset = self.h5file[i.header]
                data_array = bytearray(dset[:])
                banner_str = f" {self.record_type} Header "
                print(banner_str.center(80, '='))
                print('{:<30}:\t{}'.format("Path", i.path))
                print('{:<30}:\t{}'.format("Size", dset.shape))
                print('{:<30}:\t{}'.format("Data type", dset.dtype))
                print_header(data_array, self.record_type, self.clock_speed_hz,
                             k_list_components)
            if not {"fragment", "both", "all"}.isdisjoint(k_header_type):
                for j in i.fragments:
                    dset = self.h5file[j]
                    data_array = bytearray(dset[:])
                    banner_str = " Fragment Header "
                    print(banner_str.center(80, '-'))
                    print('{:<30}:\t{}'.format("Path", j))
                    print('{:<30}:\t{}'.format("Size", dset.shape))
                    print('{:<30}:\t{}'.format("Data type", dset.dtype))
                    print_fragment_header(data_array, self.clock_speed_hz)
            n += 1
        return

    def check_fragments(self, k_nrecords):
        if self.record_type != "TriggerRecord":
            print("Check fragments only works on TriggerRecord data.")
        else:
            report = []
            n = 0
            for i in self.records:
                if n >= k_nrecords and k_nrecords > 0:
                    break
                dset = self.h5file[i.header]
                data_array = bytearray(dset[:])
                (h, j, k) = struct.unpack('<3Q', data_array[8:32])
                (s, ) = struct.unpack('<H', data_array[42:44])
                nf = len(i.fragments)
                report.append((h, s, k, nf, nf - k))
                n += 1
            print("{:-^80}".format("Column Definitions"))
            print("i:           Trigger record number;")
            print("s:           Sequence number;")
            print("N_frag_exp:  expected no. of fragments stored in header;")
            print("N_frag_act:  no. of fragments written in trigger record;")
            print("N_diff:      N_frag_act - N_frag_exp")
            print("{:-^80}".format("Column Definitions"))
            print("{:^10}{:^10}{:^15}{:^15}{:^10}".format(
                "i", "s", "N_frag_exp", "N_frag_act", "N_diff"))
            for i in range(len(report)):
                print("{:^10}{:^10}{:^15}{:^15}{:^10}".format(*report[i]))
        return

    class Record:
        def __init__(self):
            self.path = ''
            self.header = ''
            self.fragments = []

        def __call__(self, name, dset):
            if isinstance(dset, h5py.Dataset):
                if "TR_Builder" in name:
                    self.header = self.path + '/' + name
                    # set ncomponents here
                else:
                    self.fragments.append(self.path + '/' + name)


def tick_to_timestamp(ticks, clock_speed_hz):
    ns = float(ticks)/clock_speed_hz
    if ns < 3000000000:
        return datetime.datetime.fromtimestamp(ns)
    else:
        return "InvalidDateString"


def unpack_header(data_array, entry_type):
    values = struct.unpack(DATA_FORMAT[entry_type]["unpack string"],
                           data_array[:DATA_FORMAT[entry_type]["size"]])
    header = dict(zip(DATA_FORMAT[entry_type]["keys"], values))
    return header


def print_header_dict(hdict, clock_speed_hz):
    filtered_list = ['Padding', 'Source ID version', 'Component request version']
    for ik, iv in hdict.items():
        if any(map(ik.__contains__, filtered_list)):
            continue
        elif "time" in ik or "begin" in ik or "end" in ik:
            print("{:<30}: {} ({})".format(
                ik, iv, tick_to_timestamp(iv, clock_speed_hz)))
        elif 'Marker word' in ik:
            print("{:<30}: {}".format(ik, hex(iv)))
        elif ik == 'Detector':
            print("{:<30}: {}".format(ik, DETECTOR[iv]))
        elif ik == 'Source ID subsystem' in ik:
            print("{:<30}: {}".format(ik, SUBSYSTEM[iv]))
        else:
            print("{:<30}: {}".format(ik, iv))
    return


def print_trigger_record_header(data_array, clock_speed_hz, k_list_components):
    print_header_dict(unpack_header(data_array, "TriggerRecord Header"), clock_speed_hz)

    if k_list_components:
        comp_keys = DATA_FORMAT["Component Request"]["keys"]
        comp_unpack_string = DATA_FORMAT["Component Request"]["unpack string"]
        for i_values in struct.iter_unpack(comp_unpack_string, data_array[56:]):
            i_comp = dict(zip(comp_keys, i_values))
            print(80*'-')
            print_header_dict(i_comp, clock_speed_hz)
    return


def print_fragment_header(data_array, clock_speed_hz):
    print_header_dict(unpack_header(data_array, "Fragment Header"), clock_speed_hz)
    return


def print_header(data_array, record_type, clock_speed_hz, k_list_components):
    if record_type == "TriggerRecord":
        print_trigger_record_header(data_array, clock_speed_hz,
                                    k_list_components)
    elif record_type == "TimeSlice":
        print_header_dict(unpack_header(data_array, "TimeSlice Header"), clock_speed_hz)
    else:
        print(f"Error: Record Type {record_type} is not supported.")
    return


def parse_args():
    parser = argparse.ArgumentParser(
        description='Python script to parse DUNE-DAQ HDF5 output files.')

    parser.add_argument('-f', '--file-name',
                        help='path to HDF5 file',
                        required=True)

    parser.add_argument('-b', '--binary-output',
                        help='convert to the specified binary file')

    parser.add_argument('-p', '--print-out', action='append',
                        choices=['header', 'fragment', 'both', 'attributes',
                                 'all'],
                        help='''select which part of data to be displayed, this
                        option can be repeated multiple times, "-p both" is
                        equivalent to "-p header -p fragment", "-p all" is
                        equivalent to "-p attributes -p header -p fragment"''')

    parser.add_argument('-c', '--check-fragments',
                        help='''check if fragments written in trigger record
                        matches expected number in trigger record header''',
                        action='store_true')

    parser.add_argument('-l', '--list-components',
                        help='''list components in trigger record header, used
                        with "--print-out header" or "--print-out both", not
                        applicable to TimeSlice data''', action='store_true')

    parser.add_argument('-n', '--num-of-records', type=int,
                        help='specify number of records to be parsed',
                        default=0)

    parser.add_argument('-s', '--speed-of-clock', type=float,
                        help='''specify clock speed in Hz, default is
                        50000000.0 (50MHz)''',
                        default=50000000.0)

    parser.add_argument('-v', '--version', action='version',
                        version='%(prog)s 2.0')
    return parser.parse_args()


def main():
    args = parse_args()
    if args.print_out is None and args.check_fragments is False and \
            args.binary_output is None:
        print("Error: use at least one of the two following options:")
        print("       -p, --print-out {header, fragment, both}")
        print("       -c, --check-fragments")
        print("       -b, --binary-output")
        return

    h5 = DAQDataFile(args.file_name)

    if args.binary_output is not None:
        h5.convert_to_binary(args.binary_output, args.num_of_records)
    if args.print_out is not None:
        h5.set_clock_speed_hz(args.speed_of_clock)
        h5.printout(args.print_out, args.num_of_records, args.list_components)
    if args.check_fragments:
        h5.check_fragments(args.num_of_records)

    return


if __name__ == "__main__":
    main()
