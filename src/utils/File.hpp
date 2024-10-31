/**
* @file File.hpp
* @author Marcus Gugacs
* @date 10/31/2024
* @copyright Copyright (c) 2024 Marcus Gugacs. All rights reserved.
*/

#ifndef ATLAS_FILE_HPP
#define ATLAS_FILE_HPP

#include <fstream>

#include "data/Array.hpp"
#include "data/String.hpp"

namespace atlas {
  /**
   * @brief File class providing convenient file handling.
   */
  class File {
  private:
    ntl::String m_filepath;

  public:
    /**
     * @brief Deletes Default Constructor.
     */
    File() = delete;
    /**
     * @brief Constructs a new file object using the given file path.
     * @param a_filepath the path to the file to create
     */
    explicit File(const ntl::String& a_filepath);
    /**
     * @brief Default Destructor.
     */
    ~File() = default;

    /**
     * @brief Reads the data of the file to the given string.
     * @param a_data the string to store the data in
     * @return if reading the file was successful
     */
    bool ReadFile(ntl::String& a_data);
    /**
     * @brief Writes the data from the given array to the file.
     * @param a_data the array of strings where the data is stored
     * @return if writing to the file was successful
     */
    bool WriteFile(ntl::Array<ntl::String>& a_data);
    /**
     * @brief Resets the content of the file.
     * @return if resetting the file was successful
     */
    bool ResetFile();
  };
}

#endif // ATLAS_FILE_HPP
