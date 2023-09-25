#include "hdf5.h"
#include <iostream>
#include <stdexcept>
#include <vector>
#include <chrono>
#include <thread>

#include <highfive/H5File.hpp>
#include <highfive/H5DataSet.hpp>
#include <highfive/H5DataSpace.hpp>

using namespace std::chrono;

#define DATASET_LENGHT 1073741824 // 1G double * 8 bytes = 8GB
#define DATASET_NUM 20

std::vector<double> make_array(const std::vector<size_t> &dims)
{
    auto n_elements = dims[0] * dims[1];
    std::vector<double> nd_array(n_elements, 0.0);

    for (size_t i = 0; i < dims[0]; ++i)
    {
        for (size_t j = 0; j < dims[1]; ++j)
        {
            nd_array[j + i * dims[1]] = (i + j) % 256;
        }
    }

    return nd_array;
}

HighFive::DataSet do_dataset_creation(HighFive::File &file,
                                      std::string dataset_name,
                                      size_t chunk_size,
                                      int alloc_time,
                                      bool use_chunking)
{
    std::vector<size_t> dims;
    if (use_chunking)
        dims = {DATASET_LENGHT, 1};
    else
        dims = {chunk_size, 1};

    HighFive::DataSetCreateProps dataset_create_props;

    if (use_chunking)
        dataset_create_props.add(HighFive::Chunking(std::vector<hsize_t>{chunk_size, 1}));

    // Allocate parameters
    // H5D_ALLOC_TIME_DEFAULT
    // H5D_ALLOC_TIME_EARLY
    // H5D_ALLOC_TIME_LATE
    // H5D_ALLOC_TIME_INCR
    if (alloc_time == 0)
        dataset_create_props.add(HighFive::AllocationTime(H5D_ALLOC_TIME_DEFAULT));
    if (alloc_time == 1)
        dataset_create_props.add(HighFive::AllocationTime(H5D_ALLOC_TIME_EARLY));
    if (alloc_time == 2)
        dataset_create_props.add(HighFive::AllocationTime(H5D_ALLOC_TIME_LATE));
    if (alloc_time == 3)
        dataset_create_props.add(HighFive::AllocationTime(H5D_ALLOC_TIME_INCR));

    HighFive::DataSetAccessProps dataset_access_props;
    // dataset_access_props.add(HighFive::Caching(0, 0));  // numslot, cachesize

    auto dataset = file.createDataSet<double>(dataset_name,
                                              HighFive::DataSpace(dims),
                                              dataset_create_props,
                                              dataset_access_props);

    return dataset;
}

void do_dataset_write(HighFive::File &file,
                      HighFive::DataSet &dataset,
                      std::vector<double> &nd_array,
                      size_t chunk_size,
                      bool use_chunking)
{
    std::vector<size_t> dims{chunk_size, 1};
    size_t parts = DATASET_LENGHT / chunk_size;

    // Then write, using the raw pointer.
    if (use_chunking)
    {
        for (size_t i = 0; i < parts; i++)
        {
            dataset.select({i * chunk_size, 0}, {chunk_size, 1}).write_raw(nd_array.data());
            file.flush(); // flush each dataset
        }
    }
    else
    {
        dataset.write_raw(nd_array.data());
        file.flush();
    }
}

void do_dataset_read(HighFive::File &file, std::string dataset_name)
{
    auto dataset = file.getDataSet(dataset_name);

    // First read the dimensions.
    auto dims = dataset.getDimensions();

    // Then allocate memory.
    auto n_elements = dims[0] * dims[1];
    auto nd_array = std::vector<double>(n_elements);

    // Finally, read into the memory by passing a raw pointer to the library.
    dataset.read<double>(nd_array.data());
}

int main(int argc, char *argv[])
{
    // get parameters
    if (argc != 5)
    {
        std::cout << "Usage: " << argv[0] << " <output_file> <chunk_size> <page_alloc> <alloc_time>" << std::endl;
        return 1;
    }
    // int data_length = atoi(argv[1]);
    long unsigned int block_size = 4096;
    std::string file_name = argv[1];
    size_t chunk_size = atoi(argv[2]);
    bool is_page_allocated = atoi(argv[3]);
    int alloc_time = atoi(argv[4]);
    bool use_chunking = false;

    int parts = DATASET_LENGHT / chunk_size;
    int dataset_num = parts;
    if (use_chunking)
        dataset_num = DATASET_NUM;

    auto start = high_resolution_clock::now();

    auto create_props = HighFive::FileCreateProps::Default();

    // H5F_FSPACE_STRATEGY_FSM_AGGR : Mechanisms: free-space managers, aggregators, and virtual
    // file drivers This is the library default when not set
    // H5F_FSPACE_STRATEGY_PAGE : Mechanisms: free-space managers with embedded paged aggregation
    // and virtual file drivers H5F_FSPACE_STRATEGY_AGGR : Mechanisms: aggregators and virtual file
    // drivers H5F_FSPACE_STRATEGY_NONE : Mechanisms: virtual file drivers
    // H5F_FSPACE_STRATEGY_NTYPES : Sentinel
    H5F_fspace_strategy_t strategy = H5F_FSPACE_STRATEGY_FSM_AGGR;
    if (is_page_allocated)
        strategy = H5F_FSPACE_STRATEGY_PAGE;
    hbool_t persist = true;
    hsize_t threshold_space = 0;
    create_props.add(HighFive::FileSpaceStrategy(strategy, persist, threshold_space));

    size_t pagesize = block_size * 2; // Must be tuned.
    if (is_page_allocated)
        create_props.add(HighFive::FileSpacePageSize(pagesize));

    // auto access_props = HighFive::FileAccessProps::Default();
    auto access_props = HighFive::RawPropertyList<HighFive::PropertyType::FILE_ACCESS>();

    // Adding custom properties
    size_t alignment_direct = 524288;     // 512K
    size_t block_size_direct = 524288;    // 512K
    size_t cbuf_size_direct = 1073741824; // 1G
    // access_props.add(H5Pset_fapl_direct, alignment_direct, block_size_direct, cbuf_size_direct);
    // hid_t fapl_id = access_props.getId();

    // H5Pset_file_image(fapl_id, NULL, 0);

    // hsize_t threshold_aligned = 0;
    // hsize_t alignment = block_size;
    // H5Pset_alignment(fapl_id, threshold_aligned, alignment);

    // size_t rdcc_nslots = 512;
    // size_t rdcc_nbytes = 1024 * 1024;
    // double rdcc_w0 = 0.75;
    // H5Pset_cache(fapl_id, 0, rdcc_nslots, rdcc_nbytes, rdcc_w0);

    // Cannot add properties from HighFive with RAW properties list, need to use hdf5 one
    // access_props.add(HighFive::MetadataBlockSize(block_size)); // size Metadata block size in bytes
    // if (is_page_allocated)
    //     access_props.add(HighFive::PageBufferSize(pagesize)); // maximum size of the page buffer in bytes.

    HighFive::File file(file_name,
                        HighFive::File::ReadWrite | HighFive::File::Create |
                            HighFive::File::Truncate,
                        create_props,
                        access_props);

    // set properties from hdf5 library with hid

    // size_t alignment_direct = 4096;
    // size_t block_size_drive = 4096;
    // size_t cbuf_size = 4096 * 1024;
    // H5Pset_fapl_direct(fapl_id, alignment_direct, block_size_drive, cbuf_size);

    // Create dataspace and data array
    std::vector<size_t> dims{chunk_size, 1};
    auto nd_array = make_array(dims);

    // Create dataset list
    std::vector<HighFive::DataSet> datasets;

    // create datasets
    auto t1 = high_resolution_clock::now();
    for (int i = 0; i < dataset_num; i++)
    {
        datasets.push_back(do_dataset_creation(
            file, "dataset_" + std::to_string(i) + "_raw", chunk_size, alloc_time, use_chunking));
    }
    file.flush(); // flush to trigger the allocation

    // write in datasets
    auto t2 = high_resolution_clock::now();
    for (auto dataset : datasets)
    {
        do_dataset_write(file, dataset, nd_array, chunk_size, use_chunking);
    }
    system("sync");

    // read in datasets
    auto t3 = high_resolution_clock::now();
    for (int i = 0; i < dataset_num; i++)
    {
        do_dataset_read(file, "dataset_" + std::to_string(i) + "_raw");
    }

    auto stop = high_resolution_clock::now();
    auto total_duration = duration_cast<milliseconds>(stop - start);
    auto create_duration = duration_cast<milliseconds>(t2 - t1);
    auto write_duration = duration_cast<milliseconds>(t3 - t2);
    auto read_duration = duration_cast<milliseconds>(stop - t3);
    // std::cout << "chunk_size: " << chunk_size << " parts:" << parts
    //           << " is_page_allocated:" << is_page_allocated << " alloc_time:" << alloc_time
    //           << std::endl;
    // std::cout << "Creation of dataset duration : " << create_duration.count() << "ms" <<
    // std::endl; std::cout << "Writing duration : " << write_duration.count() << "ms" << std::endl;
    // std::cout << "Reading duration : " << read_duration.count() << "ms" << std::endl;
    // std::cout << "Total duration : " << total_duration.count() << "ms" << std::endl;

    // std::cout << "chunk_size,parts,is_page_allocated,alloc_time,create_duration,write_duration,"
    //              "read_duration,total_duration"
    //           << std::endl;
    std::cout << chunk_size << "," << parts << "," << is_page_allocated << "," << alloc_time << ","
              << create_duration.count() << "," << write_duration.count() << ","
              << read_duration.count() << "," << total_duration.count() << std::endl;
}