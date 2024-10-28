//
// Created by Marcus Gugacs on 28.10.24.
//

#ifndef ATLAS_MISC_HPP
#define ATLAS_MISC_HPP

#include <iostream>
#include <fstream>

extern const char *RED;
extern const char *GREEN;
extern const char *YELLOW;
extern const char *RESET;

static void LogOutputToFile(const std::string &a_output,
                            const std::string &a_filename) {
    std::ofstream file(a_filename, std::ios::app);
    file << a_output;
    file.close();
}

static int ProcessCommand(const std::string &a_command, const std::string &a_path,
                          bool a_verbose) {
    FILE *pipe = popen(a_command.c_str(), "r");
    std::string output{};
    int exitCode = -1;
    if (pipe) {
        char buffer[128];
        while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
            if (a_verbose)
                std::cout << "Debug: " << buffer << std::endl;
            output.append(buffer);
        }
        exitCode = pclose(pipe);
    }
    LogOutputToFile(output, a_path);
    return exitCode;
}

#endif //ATLAS_MISC_HPP
