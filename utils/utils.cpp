//
// Created by rip on 6/30/19.
//

#include "utils.h"


bool startsWith(const std::string& s, const std::string& prefix)
{
    return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

std::vector<std::string> tokenize(std::string s)
{
    std::vector<std::string> tokens;
    std::string delimiter = " ";

    size_t pos = 0;
    std::string token;
    while ((pos = s.find(delimiter)) != std::string::npos) {
        token = s.substr(0, pos);
        tokens.push_back(token);
        s.erase(0, pos + delimiter.length());
    }
    tokens.push_back(s);

    return tokens;
}

bool dirExists(std::string path)
{
    struct stat info{};

    if (stat( path.c_str(), &info ) != 0)
    {
        return false;
    }
    else
    {
        return (info.st_mode & S_IFDIR) != 0;
    }
}

std::string readFile(std::string path)
{
    std::string fileBuffer, line;
    std::ifstream localFile(path);
    if (localFile.is_open())
    {
        while ( getline(localFile, line) )
        {
            fileBuffer += line;
        }
        localFile.close();
    }

    return fileBuffer;
}

int convertFlags(int pflags)
{
    int flags = 0;

    if ((pflags & SSH_FXF_READ) && (pflags & SSH_FXF_WRITE))
    {
        flags = O_RDWR;
    }
    else if (pflags & SSH_FXF_READ)
    {
        flags = O_RDONLY;
    }
    else if (pflags & SSH_FXF_WRITE)
    {
        flags = O_WRONLY;
    }

    if (pflags & SSH_FXF_APPEND)
    {
        flags |= O_APPEND;
    }
    if (pflags & SSH_FXF_CREAT)
    {
        flags |= O_CREAT;
    }
    if (pflags & SSH_FXF_TRUNC)
    {
        flags |= O_TRUNC;
    }

    return flags;
}

int convertHandleToFileDescriptor(ssh_string handle)
{
    return (int) *ssh_string_to_char(handle) - 48; // ASCII starts at 48
}

std::string trimEndNewLine(std::string s)
{
    if (!s.empty() && s[s.length()-1] == '\n')
    {
        s.erase(s.length()-1);
    }
    return s;
}