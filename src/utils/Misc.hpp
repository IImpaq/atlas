/**
* @file Misc.hpp
* @author Marcus Gugacs
* @date 28.10.24
* @copyright Copyright (c) 2024 Marcus Gugacs. All rights reserved.
*/

#ifndef ATLAS_MISC_HPP
#define ATLAS_MISC_HPP

#include <iostream>
#include <fstream>

#include <data/String.hpp>

#include "core/Logger.hpp"

namespace atlas {
  extern const char* RED;
  extern const char* GREEN;
  extern const char* YELLOW;
  extern const char* BLUE;
  extern const char* MAGENTA;
  extern const char* CYAN;
  extern const char* RESET;

  /**
   * Logs output to a file.
   *
   * @param a_output The content to log.
   * @param a_filename The filename where the output will be stored.
   */
  static void LogOutputToFile(const ntl::String& a_output,
                              const ntl::String& a_filename) {
    std::ofstream file(a_filename.GetCString(), std::ios::app);
    file << a_output;
    file.close();
  }

  /**
   * Executes an external command and logs the output.
   *
   * @param a_command The command to execute, e.g. "git status".
   * @param a_path The path where the output will be stored (in a log file).
   * @param a_verbose Whether to enable verbose logging.
   *
   * @return The exit code of the executed command.
   */
  static int ProcessCommand(const ntl::String& a_command, const ntl::String& a_path, bool a_verbose) {
    FILE* pipe = popen(a_command.GetCString(), "r");
    ntl::String output{};
    int exitCode = -1;
    if (pipe) {
      char buffer[128];
      while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        LOG_DEBUG(ntl::String{buffer});
        output.Append(buffer);
      }
      exitCode = pclose(pipe);
    }
    LogOutputToFile(output, a_path);
    return exitCode;
}
}

#endif //ATLAS_MISC_HPP
