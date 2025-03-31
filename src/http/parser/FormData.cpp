#include "http/parser/FormData.hpp"

FormData::FormData() {}

FormData::~FormData() {}

void FormData::clear() {
    form_values.clear();
    uploaded_files.clear();
}

void FormData::addFormValue(const std::string& name, const std::string& value) {
    form_values[name] = value;
}

void FormData::addUploadedFile(const std::string& name, const UploadedFile& file) {
    uploaded_files[name] = file;
}

std::string FormData::getFormValue(const std::string& name) const {
    std::map<std::string, std::string>::const_iterator it = form_values.find(name);
    if (it != form_values.end()) {
        return it->second;
    }
    return "";
}

bool FormData::hasUploadedFile(const std::string& name) const {
    return uploaded_files.find(name) != uploaded_files.end();
}

const UploadedFile* FormData::getUploadedFile(const std::string& name) const {
    std::map<std::string, UploadedFile>::const_iterator it = uploaded_files.find(name);
    if (it != uploaded_files.end()) {
        return &(it->second);
    }
    return NULL;
}

const std::map<std::string, std::string>& FormData::getFormValues() const {
    return form_values;
}

const std::map<std::string, UploadedFile>& FormData::getUploadedFiles() const {
    return uploaded_files;
} 