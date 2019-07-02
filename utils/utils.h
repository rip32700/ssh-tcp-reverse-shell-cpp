//
// Created by rip on 6/30/19.
//

#ifndef SSH_TCP_REVERSE_SHELL_CPP_UTILS_H
#define SSH_TCP_REVERSE_SHELL_CPP_UTILS_H

#include <iostream>
#include <vector>
#include <sys/stat.h>
#include <fstream>
#include <fcntl.h>
#include "libssh/sftp.h"

// string utilities
bool startsWith(const std::string&, const std::string&);
std::vector<std::string> tokenize(std::string);
std::string trimEndNewLine(std::string);

// file utilities
bool dirExists(std::string);
std::string readFile(std::string);
int convertFlags(int);
int convertHandleToFileDescriptor(ssh_string);


#endif //SSH_TCP_REVERSE_SHELL_CPP_UTILS_H
