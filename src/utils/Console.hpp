/**
* @file Console.hpp
* @author Marcus Gugacs
* @date 01.11.24
* @copyright Copyright (c) 2024 Marcus Gugacs. All rights reserved.
*/

#ifndef ATLAS_CONSOLE_HPP
#define ATLAS_CONSOLE_HPP

#include <iostream>
#include <data/String.hpp>

#include <os/Lock.hpp>
#include <os/ScopeLock.hpp>

namespace atlas {
  /**
   * @class Console
   * @brief A singleton class responsible for handling console output.
   *
   * The Console class provides methods for printing lines, updating progress,
   * and clearing the current line. It uses a mutex to ensure thread-safety.
   */
  class Console {
  private:
    ntl::Lock m_mutex;
  public:
    /**
     * @brief Gets the singleton instance of the Console class.
     *
     * @return A reference to the singleton Console instance
     */
    static Console& GetInstance();

    /**
     * @brief Prints a line to the console.
     *
     * @param message The text to print (must not be null)
     */
    void PrintLine(const ntl::String& message);

    /**
     * @brief Updates the progress indicator in the console.
     *
     * @param message The new progress text (must not be null)
     */
    void UpdateProgress(const ntl::String& message);

  private:
    Console() = default;

    /**
     * @brief Clears the current line from the console.
     */
    void ClearCurrentLine();
  };
}

#endif // ATLAS_CONSOLE_HPP
