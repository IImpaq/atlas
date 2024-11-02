/**
* @file Console.cpp
* @author Marcus Gugacs
* @date 01.11.24
* @copyright Copyright (c) 2024 Marcus Gugacs. All rights reserved.
*/

#include "Console.hpp"

namespace atlas {
  Console& Console::GetInstance() {
    static Console instance;
    return instance;
  }

  void Console::PrintLine(const ntl::String& message) {
    ntl::ScopeLock lock(&m_mutex);
    ClearCurrentLine();
    std::cout << message.GetCString() << std::endl;
  }

  void Console::UpdateProgress(const ntl::String& message) {
    ntl::ScopeLock lock(&m_mutex);
    ClearCurrentLine();
    std::cout << message.GetCString() << std::flush;
  }

  void Console::ClearCurrentLine() {
    std::cout << "\r" << std::string(120, ' ') << "\r"; // Use a reasonable max width
  }
}
