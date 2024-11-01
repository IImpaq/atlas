//
// Created by Marcus Gugacs on 01.11.24.
//

#ifndef CONSOLE_HPP
#define CONSOLE_HPP

#include <iostream>
#include <data/String.hpp>

#include <os/Lock.hpp>
#include <os/ScopeLock.hpp>

class Console {
public:
  static Console& GetInstance() {
    static Console instance;
    return instance;
  }

  void PrintLine(const ntl::String& message) {
    ntl::ScopeLock lock(&m_mutex);
    ClearCurrentLine();
    std::cout << message.GetCString() << std::endl;
  }

  void UpdateProgress(const ntl::String& message) {
    ntl::ScopeLock lock(&m_mutex);
    ClearCurrentLine();
    std::cout << message.GetCString() << std::flush;
  }

private:
  Console() = default;
  ntl::Lock m_mutex;

  void ClearCurrentLine() {
    std::cout << "\r" << std::string(120, ' ') << "\r"; // Use a reasonable max width
  }
};


#endif //CONSOLE_HPP
