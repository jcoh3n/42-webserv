#include "http/upload/UploadConfig.hpp"
#include "utils/Common.hpp"
#include <sys/stat.h>

bool UploadConfig::ensureUploadDirectory() const {
    struct stat info;
    if (stat(upload_directory.c_str(), &info) != 0 || !(info.st_mode & S_IFDIR)) {
        if (mkdir(upload_directory.c_str(), 0755) != 0) {
            LOG_ERROR("Failed to create upload directory: " << upload_directory);
            return false;
        }
    }
    return true;
} 