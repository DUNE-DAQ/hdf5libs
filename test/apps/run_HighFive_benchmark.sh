#!/bin/bash
set -eu

executables="HighFive_full_bench"

# help section
Help()
{
    echo "Usage: $0 [-f outputfile] [-p page] [-a alloctime] [-b blk]"
    echo "       -f outputfile: output file name"
    echo "       -p page: page writing method or not (0 or 1 or '0 1' default)"
    echo "       -a alloctime: allocation time (0 default, 1 early, 2 late, 3 incremental or '1 2 3' default)"
    echo "       -b blk: block size ('524288 1048576 2097152 4194304 8388608 16777216 33554432' default)"
    echo "       -h: help"
    echo "Example: $0 -f HighFive_full_bench.h5 -f /mnt/nvme0n1p1/dataset_integer_raw.h5 -p 0 -a 1 -b '524288 1048576'"
}


if [ $# -eq 0 ]; then
    echo "No arguments provided."
    Help
    exit 0;
fi

pages="0 1"
alloctimes="1 2 3"
blks="524288 1048576 2097152 4194304 8388608 16777216 33554432"

while getopts 'f:p:a:b:h' flag;
do
    case "${flag}" in
        f) outputfile=${OPTARG};;
        p) pages=${OPTARG};;
        a) alloctimes=${OPTARG};;
        b) blks=${OPTARG};;
        h)  Help
            exit 0;;
        *) echo "Unexpected option ${flag}"
            Help
            exit 1;;
    esac
done

# Time it : overview and eventual cache warm up
for exe in $executables; do
    for blk in $blks; do
        for page in $pages; do
            for time in $alloctimes; do
                $exe $outputfile ${blk} ${page} ${time}
                sync
                # remove all hdf5 files
                rm -f $outputfile && sync
                sleep 5
                # Trim drive
                sudo fstrim -v /mnt/nvme0n1p1
                sleep 15
            done
        done

    done
done

echo "Done."