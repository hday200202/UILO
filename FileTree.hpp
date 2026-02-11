#pragma once

#include <filesystem>
#include <vector>
#include <memory>

#ifdef _WIN32
    #include <windows.h>
#endif

namespace uilo {

class Entry {
public:
    virtual ~Entry() = default;

    std::filesystem::path getPath() const { return m_path; }
    std::string getName() const { return m_path.filename().string(); }
    std::string getExt() const { return m_path.extension().string(); }
    
    bool exists() const { return !m_path.empty() && std::filesystem::exists(m_path); }
    bool isDirectory() const { return exists() && std::filesystem::is_directory(m_path); }
    bool isHidden() const {
        auto name = getName();
        if (name.empty()) return false;
        
        #ifdef _WIN32
            DWORD attrs = GetFileAttributesW(m_path.c_str());
            return (attrs != INVALID_FILE_ATTRIBUTES) && (attrs & FILE_ATTRIBUTE_HIDDEN);
        #else
            return name[0] == '.';
        #endif
    }

    virtual std::uintmax_t getSize() const = 0;
    virtual void expand() = 0;
    virtual void refresh() = 0;

protected:
    bool m_expanded = false;
    std::filesystem::path m_path;
};

class File : public Entry {
public:
    File() {}
    File(const std::filesystem::path& path);
    ~File() override {}

    std::uintmax_t getSize() const override {
        std::error_code error;
        auto size = std::filesystem::file_size(m_path, error);
        return error ? 0 : size;
    }

    void expand() override {}
    void refresh() override {}
};

struct Directory : public Entry {
public:
    Directory() {}
    Directory(const std::filesystem::path& path);
    ~Directory() override {}

    std::uintmax_t getSize() const override { return 0; }

    void expand() override;
    void refresh() override;

    std::vector<std::unique_ptr<Entry>>& getEntries() { return m_entries; }
    bool isExpanded() const { return m_expanded; }

private:
    std::vector<std::unique_ptr<Entry>> m_entries;
};

class FileTree {
public:
    FileTree() {}
    FileTree(const std::filesystem::path& rootPath);
    ~FileTree();

    void find(const std::string& name, std::vector<Entry*>& foundEntries);
    Directory* getRootDir() const { return m_root.get(); }
    void setRootDir(const std::filesystem::path& rootPath) {
        m_root = std::make_unique<Directory>(rootPath);
        if (m_root->exists()) m_root->expand();
    }

private:
    static void searchEntries(Entry* node, const std::string& name, std::vector<Entry*>& results);
    std::unique_ptr<Directory> m_root = nullptr;
    std::unique_ptr<Entry> m_selectedEntry = nullptr;
    std::unique_ptr<Entry> m_copiedEntry = nullptr;
    std::vector<std::string> m_selectedFileOptions; // copy, paste, delete, rename
};

// ---------------------------------------------------- Implementations

inline File::File(const std::filesystem::path& path) { m_path = path; }
inline Directory::Directory(const std::filesystem::path& path) { m_path = path; }

inline void Directory::expand() {
    if (m_expanded) return; // Already expanded, use cached entries
    
    try {
        for (const auto& entry : std::filesystem::directory_iterator(m_path, std::filesystem::directory_options::skip_permission_denied)) {
            try {
                if (std::filesystem::is_directory(entry))
                    m_entries.push_back(std::make_unique<Directory>(entry.path()));
                else
                    m_entries.push_back(std::make_unique<File>(entry.path()));
            } catch (const std::filesystem::filesystem_error&) {
                // Skip entries we can't access
            }
        }
    } catch (const std::filesystem::filesystem_error&) {
        // Can't iterate this directory (permissions, etc.)
    }
    
    m_expanded = true;
}

inline void Directory::refresh() {
    m_entries.clear();
    m_expanded = false;
    expand();
}

inline FileTree::FileTree(const std::filesystem::path& rootPath) {
    m_root = std::make_unique<Directory>(rootPath);
    if (m_root->exists()) m_root->expand();
}

inline FileTree::~FileTree() {

}

inline void FileTree::searchEntries(Entry* node, const std::string& name, std::vector<Entry*>& results) {
    if (node->getName().find(name) != std::string::npos)
        results.push_back(node);
    if (auto dir = dynamic_cast<Directory*>(node)) {
        for (const auto& child : dir->getEntries())
            searchEntries(child.get(), name, results);
    }
}

inline void FileTree::find(const std::string& name, std::vector<Entry*>& foundEntries) {
    if (m_root)
        searchEntries(m_root.get(), name, foundEntries);
}

}