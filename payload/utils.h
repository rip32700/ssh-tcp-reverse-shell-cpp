//
// Created by rip on 6/30/19.
//

#ifndef SSH_TCP_REVERSE_SHELL_CPP_UTILS_H
#define SSH_TCP_REVERSE_SHELL_CPP_UTILS_H

#include <iostream>
#include <vector>
#include <sys/stat.h>
#include <fstream>

bool startsWith(const std::string&, const std::string&);
std::vector<std::string> tokenize(std::string);
bool dirExists(std::string);
std::string readFile(std::string);

#endif //SSH_TCP_REVERSE_SHELL_CPP_UTILS_H
