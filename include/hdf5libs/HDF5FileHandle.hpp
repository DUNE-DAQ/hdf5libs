/**
 * @file HDF5FileHandle.hpp
 *
 * Class for managing HDF5 file opening/closing.
 * 
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef HDF5LIBS_INCLUDE_HDF5FILEHANDLE_HPP_
#define HDF5LIBS_INCLUDE_HDF5FILEHANDLE_HPP_

// System
#include <iostream>
#include <string>
#include <variant>
#include <filesystem>
#include <sys/statvfs.h>

// HighFive
#include <highfive/H5File.hpp>
//#include <highfive/H5Object.hpp>


// DUNE-DAQ
//#include "logging/Logging.hpp"
//#include "daqdataformats/TriggerRecord.hpp"
//#include "nlohmann/json.hpp"
//#include "hdf5libs/StorageKey.hpp"
//#include "hdf5libs/HDF5KeyTranslator.hpp"


namespace dunedaq {
namespace hdf5libs {


/**
 * @brief HDF5FileHandle is a small helper class that takes care
 * of common actions on HighFive::File instances.
 */
class HDF5FileHandle
{
public:
  explicit HDF5FileHandle(const std::string& filename, unsigned open_flags)
    : m_original_filename(filename)
  {
    std::string inprogress_filename = m_original_filename;   //WK -- why inprogress?
    m_file_ptr.reset(new HighFive::File(inprogress_filename, 
					open_flags));
  }

  ~HDF5FileHandle()
  {
    if (m_file_ptr.get() != nullptr) {
      m_file_ptr->flush();

      std::string open_filename = m_file_ptr->getName();  //WK -- need temp open_filename?
      std::filesystem::rename(open_filename, m_original_filename);

      m_file_ptr.reset(); // explicit destruction; not really needed, but nice to be clear...
    }
  }

  HighFive::File* get_file_ptr() const { return m_file_ptr.get(); }

private:
  std::string m_original_filename;
  std::unique_ptr<HighFive::File> m_file_ptr;
};


}
}

#endif // HDF5LIBS_INCLUDE_HDF5FILEHANDLE_HPP_

// Local Variables:
// c-basic-offset: 2
// End:
