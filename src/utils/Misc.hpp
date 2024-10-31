//
// Created by Marcus Gugacs on 28.10.24.
//

#ifndef ATLAS_MISC_HPP
#define ATLAS_MISC_HPP

#include <iostream>
#include <fstream>

#include <data/String.hpp>

extern const char *RED;
extern const char *GREEN;
extern const char *YELLOW;
extern const char *RESET;

static void LogOutputToFile(const ntl::String &a_output,
                            const ntl::String &a_filename) {
    std::ofstream file(a_filename.GetCString(), std::ios::app);
    file << a_output;
    file.close();
}

static int ProcessCommand(const ntl::String &a_command, const ntl::String &a_path,
                          bool a_verbose) {
    FILE *pipe = popen(a_command.GetCString(), "r");
    ntl::String output{};
    int exitCode = -1;
    if (pipe) {
        char buffer[128];
        while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
            if (a_verbose)
                std::cout << "Debug: " << buffer << std::endl;
            output.Append(buffer);
        }
        exitCode = pclose(pipe);
    }
    LogOutputToFile(output, a_path);
    return exitCode;
}

#endif //ATLAS_MISC_HPP
