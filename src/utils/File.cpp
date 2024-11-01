/**
* @file File.cpp
* @author Marcus Gugacs
* @date 31.10.2024
* @copyright Copyright (c) 2024 Marcus Gugacs. All rights reserved.
*/

#include "File.hpp"

namespace atlas {
  File::File(const ntl::String& a_filepath)
    : m_filepath{a_filepath} { }

  bool File::ReadFile(ntl::String& a_data) {
    std::ifstream file(m_filepath.GetCString(), std::ios::binary | std::ios::ate);
    if (!file)
      return false;

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size))
      return false;

    a_data.Clear();
    a_data.Append(buffer.data());
    return true;
  }

  bool File::WriteFile(ntl::Array<ntl::String>& a_data) {
    std::ofstream file(m_filepath.GetCString(), std::ios::binary | std::ios::out | std::ios::app);
    if (!file)
      return false;

    for(ntl::Size i = 0; i < a_data.GetSize(); ++i) {
      file.write(a_data[i].GetCString(), static_cast<long>(a_data[i].GetSize()));
    }
    file.close();
    return true;
  }

  bool File::ResetFile() {
    std::ofstream file(m_filepath.GetCString(), std::ios::binary | std::ios::out);
    if (!file)
      return false;

    file.close();
    return true;
  }
}
