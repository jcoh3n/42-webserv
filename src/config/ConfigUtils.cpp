#include "config/ConfigParser.hpp"
#include "config/ConfigTypes.hpp"
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>

std::vector<std::string> ConfigParser::split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    
    while (std::getline(ss, token, delimiter)) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    
    return tokens;
}

std::string ConfigParser::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r\f\v");
    if (first == std::string::npos) {
        return "";
    }
    size_t last = str.find_last_not_of(" \t\n\r\f\v");
    return str.substr(first, last - first + 1);
}

size_t ConfigParser::parseSize(const std::string& size_str) {
    if (size_str.empty()) {
        return 0;
    }
    
    size_t len = size_str.length();
    
    if (size_str.find_first_not_of("0123456789") == std::string::npos) {
        return static_cast<size_t>(atol(size_str.c_str()));
    }
    
    size_t value = static_cast<size_t>(atol(size_str.substr(0, len - 1).c_str()));
    char unit = size_str[len - 1];
    
    switch (unit) {
        case 'k':
        case 'K':
            return value * 1024;
        case 'm':
        case 'M':
            return value * 1024 * 1024;
        case 'g':
        case 'G':
            return value * 1024 * 1024 * 1024;
        default:
            throw std::runtime_error("Invalid size unit");
    }
}

bool ConfigParser::fileExists(const std::string& path) {
    std::ifstream file(path.c_str());
    return file.good();
}

bool ConfigParser::directoryExists(const std::string& path) {
    std::ifstream dir((path + "/.").c_str());
    return dir.good();
} 