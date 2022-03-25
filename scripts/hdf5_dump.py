#!/usr/bin/env python3

import argparse
import datetime
import h5py
import struct


class DAQDataFile:
    def __init__(self, name):
        self.name = name
        self.h5file = h5py.File(self.name, 'r')
        # Assume HDf5 files without file attributes field "record_type"
        # are old data files which only contain "TriggerRecord" data.
        self.record_type = 'TriggerRecord'
        self.clock_speed_hz = 50000000.0
        self.records = []
        if 'record_type' in self.h5file.attrs.keys():
            self.record_type = self.h5file.attrs['record_type']
        for i in self.h5file.keys():
            record = self.Record()
            record.path = i
            self.h5file[i].visititems(record)
            self.records.append(record)

    def __del__(self):
        self.h5file.close()

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
            print(32*"=", "File  Metadata", 32*"=")
            for k in self.h5file.attrs.keys():
                print("{:<30}: {}".format(k, self.h5file.attrs[k]))
        print(80*"=")
        n = 0
        for i in self.records:
            if n >= k_nrecords and k_nrecords > 0:
                break
            if not {"header", "both", "all"}.isdisjoint(k_header_type):
                dset = self.h5file[i.header]
                data_array = bytearray(dset[:])
                print('{:<30}:\t{}'.format("Path", i.path))
                print('{:<30}:\t{}'.format("Size", dset.shape))
                print('{:<30}:\t{}'.format("Data type", dset.dtype))
                print_header(data_array, self.record_type, self.clock_speed_hz,
                             k_list_components)
            if not {"fragment", "both", "all"}.isdisjoint(k_header_type):
                for j in i.fragments:
                    dset = self.h5file[j]
                    data_array = bytearray(dset[:])
                    print(80*'-')
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
                nf = len(i.fragments)
                report.append((h, k, nf, nf - k))
                n += 1
            print("{:-^60}".format("Column Definitions"))
            print("i:           Trigger record number;")
            print("N_frag_exp:  expected no. of fragments stored in header;")
            print("N_frag_act:  no. of fragments written in trigger record;")
            print("N_diff:      N_frag_act - N_frag_exp")
            print("{:-^60}".format("Column Definitions"))
            print("{:^10}{:^15}{:^15}{:^10}".format(
                "i", "N_frag_exp", "N_frag_act", "N_diff"))
            for i in range(len(report)):
                print("{:^10}{:^15}{:^15}{:^10}".format(*report[i]))
        return

    class Record:
        def __init__(self):
            self.path = ''
            self.header = ''
            self.fragments = []

        def __call__(self, name, dset):
            if isinstance(dset, h5py.Dataset):
                if "Header" in name:
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


def unpack_header(data_array, unpack_string, keys):
    values = struct.unpack(unpack_string, data_array)
    header = dict(zip(keys, values))
    return header


def get_geo_id_type(i):
    types = {1: 'TPC', 2: 'PDS', 3: 'DataSelection', 4: 'NDLArTPC'}
    if i in types.keys():
        return types[i]
    else:
        return "Invalid"


def print_header_dict(hdict, clock_speed_hz):
    filtered_list = ['Padding', 'GeoID version', 'Component request version']
    for ik, iv in hdict.items():
        if any(map(ik.__contains__, filtered_list)):
            continue
        elif "time" in ik or "begin" in ik or "end" in ik:
            print("{:<30}: {} ({})".format(
                ik, iv, tick_to_timestamp(iv, clock_speed_hz)))
        elif 'Marker word' in ik:
            print("{:<30}: {}".format(ik, hex(iv)))
        elif 'GeoID type' in ik:
            print("{:<30}: {}".format(ik, get_geo_id_type(iv)))
        else:
            print("{:<30}: {}".format(ik, iv))
    return


def print_fragment_header(data_array, clock_speed_hz):
    keys = ['Marker word', 'Version', 'Frag Size', 'Trig number',
            'Trig timestamp', 'Window begin', 'Window end', 'Run number',
            'Error bits', 'Fragment type', 'Sequence number',
            'Fragment Padding', 'GeoID version', 'GeoID type', 'GeoID region',
            'GeoID element', 'Geo ID Padding']
    unpack_string = '<2I5Q3I2H1I2H2I'
    print_header_dict(unpack_header(data_array[:80], unpack_string, keys),
                      clock_speed_hz)
    return


def print_trigger_record_header(data_array, clock_speed_hz, k_list_components):
    keys = ['Marker word', 'Version', 'Trigger number',
            'Trigger timestamp', 'No. of requested components', 'Run Number',
            'Error bits', 'Trigger type', 'Sequence number',
            'Max sequence num']
    unpack_string = '<2I3Q2I3H'
    print_header_dict(unpack_header(data_array[:46], unpack_string, keys),
                      clock_speed_hz)

    if k_list_components:
        comp_keys = ['Component request version', 'Component Request Padding',
                     'GeoID version', 'GeoID type', 'GeoID region',
                     'GeoID element', 'Geo ID Padding', 'Begin time',
                     'End time']
        comp_unpack_string = "<3I2H2I2Q"
        for i_values in struct.iter_unpack(comp_unpack_string,
                                           data_array[48:]):
            i_comp = dict(zip(comp_keys, i_values))
            print(80*'-')
            print_header_dict(i_comp, clock_speed_hz)
    return


def print_time_slice_header(data_array, clock_speed_hz):
    keys = ['Magic word', 'Version', 'TimeSlice number',
            'Run number']
    unpack_string = '<2IQI'
    print_header_dict(unpack_header(data_array[:20], unpack_string, keys))
    return


def print_header(data_array, record_type, clock_speed_hz, k_list_components):
    if record_type == "TriggerRecord":
        print_trigger_record_header(data_array, clock_speed_hz,
                                    k_list_components)
    elif record_type == "TimeSlice":
        print_time_slice_header(data_array, clock_speed_hz)
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
                        help='''specify clock spped in Hz, default is
                        50000000.0 (50MHz)''',
                        default=50000000.0)

    parser.add_argument('-v', '--version', action='version',
                        version='%(prog)s 1.0')
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
