#ifndef UPLOAD_CONFIG_HPP
#define UPLOAD_CONFIG_HPP

#include <string>
#include <vector>
#include <sys/stat.h>
#include <cstdlib>

class UploadConfig {
public:
    UploadConfig(const std::string& upload_dir = "./www/uploads/",
                size_t max_file_size = 10 * 1024 * 1024)  // 10 MB par défaut
        : upload_directory(upload_dir)
        , max_file_size(max_file_size) {}
    
    bool ensureUploadDirectory() const;
    
    const std::string& getUploadDirectory() const { return upload_directory; }
    size_t getMaxFileSize() const { return max_file_size; }
    
    void setUploadDirectory(const std::string& dir) { upload_directory = dir; }
    void setMaxFileSize(size_t size) { max_file_size = size; }

private:
    std::string upload_directory;
    size_t max_file_size;
};

#endif // UPLOAD_CONFIG_HPP 