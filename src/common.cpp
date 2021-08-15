#include "common.hpp"

std::string getHomeDirectory() {
#ifdef _WIN32
    return string(std::getenv("HOMEDRIVE")) + std::getenv("HOMEPATH");
#else
    return getenv("HOME");
#endif // _WIN32
};

std::string getPathSeparator() {
#ifdef _WIN32
    return "\\";
#else
    return "/";
#endif // _WIN32
};
