#ifndef FORM_DATA_HPP
#define FORM_DATA_HPP

#include <string>
#include <map>

// Structure pour les fichiers uploadés
struct UploadedFile {
    std::string name;
    std::string filename;
    std::string content_type;
    std::string data;
};

/**
 * @brief Classe gérant les données de formulaire
 */
class FormData {
public:
    FormData();
    ~FormData();
    
    /**
     * @brief Nettoie les données
     */
    void clear();
    
    /**
     * @brief Ajoute une valeur de formulaire
     */
    void addFormValue(const std::string& name, const std::string& value);
    
    /**
     * @brief Ajoute un fichier uploadé
     */
    void addUploadedFile(const std::string& name, const UploadedFile& file);
    
    /**
     * @brief Récupère une valeur de formulaire
     */
    std::string getFormValue(const std::string& name) const;
    
    /**
     * @brief Vérifie si un fichier a été uploadé
     */
    bool hasUploadedFile(const std::string& name) const;
    
    /**
     * @brief Récupère un fichier uploadé
     */
    const UploadedFile* getUploadedFile(const std::string& name) const;
    
    /**
     * @brief Récupère toutes les valeurs de formulaire
     */
    const std::map<std::string, std::string>& getFormValues() const;
    
    /**
     * @brief Récupère tous les fichiers uploadés
     */
    const std::map<std::string, UploadedFile>& getUploadedFiles() const;
    
private:
    std::map<std::string, std::string> form_values;
    std::map<std::string, UploadedFile> uploaded_files;
};

#endif // FORM_DATA_HPP 