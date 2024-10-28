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

static void logOutputToFile(const std::string &output,
                            const std::string &filename) {
    std::ofstream file(filename, std::ios::app);
    file << output;
    file.close();
}

static int processCommand(const std::string &command, const std::string &path,
                          bool verbose) {
    FILE *pipe = popen(command.c_str(), "r");
    std::string output{};
    int exitCode = -1;
    if (pipe) {
        char buffer[128];
        while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
            if (verbose)
                std::cout << "Debug: " << buffer << std::endl;
            output.append(buffer);
        }
        exitCode = pclose(pipe);
    }
    logOutputToFile(output, path);
    return exitCode;
}

#endif //ATLAS_MISC_HPP
