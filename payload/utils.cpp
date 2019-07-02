//
// Created by rip on 6/30/19.
//

#include "utils.h"

bool startsWith(const std::string& s, const std::string& prefix) {
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