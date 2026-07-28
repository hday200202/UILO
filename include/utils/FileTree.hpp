#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <chrono>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <vector>
#include <memory>

namespace uilo {

namespace fs = std::filesystem;


/*
    getFileTime_t(std::filesystem::file_time_type)
    - convert a filesystem clock timestamp to a time_t
*/
inline std::time_t getFileTime_t(fs::file_time_type ftime) {
    // Rebase the file-clock time point onto the system clock via their now()s.
    // This avoids std::chrono::file_clock::to_sys, which is C++20-only and not
    // exposed by every standard library (notably some MSVC STL versions), so it
    // stays portable across MSVC / libstdc++ / libc++ and C++17 / C++20.
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
    return std::chrono::system_clock::to_time_t(sctp);
}


/*
    FileKind
    - Enum for abstract file types.
    - Scoped (enum class) deliberately: unscoped enumerators named Image and
      Text would collide with the uilo::Image and uilo::Text element classes.
*/
enum class FileKind {
    Image,      Code,       Doc,        Text,
    Binary,     Audio,      Video,      Archive,
    Unknown,
};



/*
    determineFileKind(std::string)
    - maps a file extension (with leading '.') to a FileKind, no disk access
*/
inline FileKind determineFileKind(std::string extension) {
    std::transform(extension.begin(), extension.end(), extension.begin(),
        [](unsigned char c) { return std::tolower(c); });

    using FK = FileKind;
    static const std::unordered_map<std::string, FileKind> kExtToKind = {
        {".png", FK::Image}, {".jpg", FK::Image}, {".jpeg", FK::Image}, {".gif", FK::Image},
        {".bmp", FK::Image}, {".tga", FK::Image}, {".tiff", FK::Image}, {".webp", FK::Image},
        {".svg", FK::Image}, {".ico", FK::Image},

        {".c", FK::Code}, {".cpp", FK::Code}, {".cc", FK::Code}, {".cxx", FK::Code},
        {".h", FK::Code}, {".hpp", FK::Code}, {".hh", FK::Code}, {".hxx", FK::Code},
        {".cs", FK::Code}, {".java", FK::Code}, {".py", FK::Code}, {".js", FK::Code},
        {".ts", FK::Code}, {".jsx", FK::Code}, {".tsx", FK::Code}, {".go", FK::Code},
        {".rs", FK::Code}, {".rb", FK::Code}, {".php", FK::Code}, {".swift", FK::Code},
        {".lua", FK::Code}, {".sh", FK::Code}, {".sql", FK::Code}, {".html", FK::Code},
        {".css", FK::Code}, {".json", FK::Code}, {".xml", FK::Code}, {".yaml", FK::Code},
        {".yml", FK::Code}, {".toml", FK::Code},

        {".pdf", FK::Doc}, {".doc", FK::Doc}, {".docx", FK::Doc}, {".odt", FK::Doc},
        {".rtf", FK::Doc}, {".ppt", FK::Doc}, {".pptx", FK::Doc}, {".xls", FK::Doc},
        {".xlsx", FK::Doc}, {".csv", FK::Doc},

        {".txt", FK::Text}, {".md", FK::Text}, {".log", FK::Text}, {".ini", FK::Text},
        {".cfg", FK::Text}, {".conf", FK::Text},

        {".mp3", FK::Audio}, {".wav", FK::Audio}, {".flac", FK::Audio}, {".ogg", FK::Audio},
        {".m4a", FK::Audio}, {".aac", FK::Audio}, {".wma", FK::Audio},

        {".mp4", FK::Video}, {".mkv", FK::Video}, {".mov", FK::Video}, {".avi", FK::Video},
        {".webm", FK::Video}, {".flv", FK::Video}, {".wmv", FK::Video},

        {".zip", FK::Archive}, {".tar", FK::Archive}, {".gz", FK::Archive}, {".tgz", FK::Archive},
        {".bz2", FK::Archive}, {".7z", FK::Archive}, {".rar", FK::Archive}, {".xz", FK::Archive},

        {".exe", FK::Binary}, {".dll", FK::Binary}, {".so", FK::Binary}, {".dylib", FK::Binary},
        {".bin", FK::Binary}, {".o", FK::Binary}, {".a", FK::Binary}, {".lib", FK::Binary},
        {".app", FK::Binary},
    };

    auto it = kExtToKind.find(extension);
    return it != kExtToKind.end() ? it->second : FileKind::Unknown;
}


class Directory;

/*
    EntryType
    - Discriminates the two concrete FSEntry subclasses without RTTI/dynamic_cast
*/
enum class EntryType { File, Directory };


/*
    FSEntry
    - Common base for File and Directory.
    - Holds everything that's meaningful for both: path, parent, name,
      permissions, and symlink target. All are captured once via error_code
      overloads, so a vanished file or broken symlink mid-scan never throws.
    - type()/isFile()/isDirectory() let callers holding an FSEntry* figure out
      which concrete type they have and cast accordingly (static_cast is safe
      once type() has been checked).
*/
class FSEntry {
public:
    virtual ~FSEntry() = default;

    virtual EntryType type() const = 0;
    bool isFile()                               const { return type() == EntryType::File; }
    bool isDirectory()                          const { return type() == EntryType::Directory; }

    const fs::path& getPath()                   const { return m_path; }
    const std::string& getName()                const { return m_name; }
    Directory* getParent()                      const { return m_parent; }

    std::optional<fs::path> getSymlinkTarget()  const { return m_symlinkTarget; }
    fs::perms getPermissions()                  const { return m_permissions; }
    bool isReadOnly()                            const { return m_readOnly; }

protected:
    FSEntry(fs::path path, Directory* parent) : m_path(std::move(path)), m_parent(parent) {
        m_name = m_path.filename().string();

        std::error_code linkStatEc;
        auto symStatus = fs::symlink_status(m_path, linkStatEc);
        if (!linkStatEc && fs::is_symlink(symStatus)) {
            std::error_code readLinkEc;
            auto target = fs::read_symlink(m_path, readLinkEc);
            if (!readLinkEc)
                m_symlinkTarget.emplace(target);
        }

        std::error_code statEc;
        auto st = fs::status(m_path, statEc);
        if (!statEc) {
            m_permissions = st.permissions();
            m_readOnly = (m_permissions &
                (fs::perms::owner_write | fs::perms::group_write | fs::perms::others_write))
                == fs::perms::none;
        }
    }

    fs::path                m_path;
    std::string             m_name;
    Directory*              m_parent        = nullptr;
    std::optional<fs::path> m_symlinkTarget;
    fs::perms               m_permissions   = fs::perms::none;
    bool                    m_readOnly      = false;
};



/*
    File
    - Abstract representation of a file on disk.
    - Constructor: File(std::filesystem::path, Directory* parent = nullptr)
    - getFileName()      -> string name of file without extension
    - getFileExt()       -> string file extension
    - getFileKind()      -> abstract FileKind derived from extension (no disk access)
    - getFileSize()      -> file size in bytes, empty on stat failure
    - getLastModified()  -> last write time as time_t, empty on stat failure
    - getPermissions()   -> std::filesystem::perms captured at construction (inherited)
    - isReadOnly()       -> true if no write permission bit is set (inherited)
    - getSymlinkTarget() -> resolved target path, if this file is a symlink (inherited)
    - getPath()          -> full path on disk (inherited)
*/
class File : public FSEntry {
public:
    File(fs::path path, Directory* parent = nullptr) : FSEntry(std::move(path), parent) {
        m_extension = m_path.extension().string();
        m_kind = determineFileKind(m_extension);
        m_name = m_path.stem().string(); // override base's full-filename default
    }

    EntryType type()          const override { return EntryType::File; }

    std::string getFileName() const { return m_name; }
    std::string getFileExt()  const { return m_extension; }
    FileKind getFileKind()    const { return m_kind; }

    std::optional<size_t> getFileSize() const {
        std::error_code ec;
        auto size = fs::file_size(m_path, ec);
        if (ec) return std::nullopt;
        return size;
    }

    std::optional<std::time_t> getLastModified() const {
        std::error_code ec;
        auto ftime = fs::last_write_time(m_path, ec);
        if (ec) return std::nullopt;
        return getFileTime_t(ftime);
    }

private:
    std::string m_extension = "";
    FileKind    m_kind      = FileKind::Unknown;
};



/*
    Directory
    - Abstract representation of a directory on disk.
    - Constructor: Directory(std::filesystem::path, Directory* parent = nullptr)
    - expand()     -> lazily lists immediate children (files + subdirectories),
                      caching them in m_children. A no-op if already loaded, so
                      it's safe to call every time a widget "opens" this
                      directory without re-hitting the disk.
    - invalidate() -> drops the cache; pass reload=true to re-list immediately.
    - getChildren()/getFiles()/getSubdirectories() -> cached results.
    - Children are stored as a single vector of FSEntry so callers can walk
      one ordered list and cast per-entry (isDirectory()/isFile()) rather than
      juggling two parallel containers.
    - Note: expand() only ever lists one level. Nothing here recurses
      automatically, so a symlink that loops back to an ancestor directory
      is harmless unless something walks the tree eagerly.
*/
class Directory : public FSEntry {
public:
    Directory(fs::path path, Directory* parent = nullptr) : FSEntry(std::move(path), parent) {}

    EntryType type() const override { return EntryType::Directory; }

    bool isLoaded() const { return m_loaded; }

    void expand() {
        if (m_loaded) return;

        std::error_code ec;
        fs::directory_iterator it(m_path, fs::directory_options::skip_permission_denied, ec);
        fs::directory_iterator end;

        for (; !ec && it != end; it.increment(ec)) {
            const fs::directory_entry& entry = *it;

            std::error_code typeEc;
            bool isDir = entry.is_directory(typeEc);

            if (!typeEc && isDir)
                m_children.push_back(std::make_unique<Directory>(entry.path(), this));
            else
                m_children.push_back(std::make_unique<File>(entry.path(), this));
        }

        std::sort(m_children.begin(), m_children.end(),
            [](const auto& a, const auto& b) {
                if (a->isDirectory() != b->isDirectory())
                    return a->isDirectory(); // directories before files
                return a->getName() < b->getName();
            });

        m_loaded = true;
    }

    void invalidate(bool reload = false) {
        m_children.clear();
        m_loaded = false;
        if (reload) expand();
    }

    const std::vector<std::unique_ptr<FSEntry>>& getChildren() const { return m_children; }

    std::vector<Directory*> getSubdirectories() const {
        std::vector<Directory*> result;
        result.reserve(m_children.size());
        for (const auto& e : m_children)
            if (e->isDirectory())
                result.push_back(static_cast<Directory*>(e.get()));
        return result;
    }

    std::vector<File*> getFiles() const {
        std::vector<File*> result;
        result.reserve(m_children.size());
        for (const auto& e : m_children)
            if (e->isFile())
                result.push_back(static_cast<File*>(e.get()));
        return result;
    }

private:
    std::vector<std::unique_ptr<FSEntry>> m_children;
    bool m_loaded = false;
};



/*
    FileTree
    - Builds and owns a lazily-expanded directory tree rooted at a given path.
    - Constructor: FileTree(std::filesystem::path rootPath)
      Expands the root immediately so the top level is ready to display;
      everything below the root stays unloaded until a widget calls
      expand() on the Directory it's opening.
    - getRoot()  -> root Directory*
    - refresh()  -> drops and re-lists the root's immediate children
*/
class FileTree {
public:
    explicit FileTree(fs::path rootPath)
        : m_root(std::make_unique<Directory>(std::move(rootPath), nullptr)) {
        m_root->expand();
    }

    Directory* getRoot() const { return m_root.get(); }
    void refresh() { m_root->invalidate(true); }

private:
    std::unique_ptr<Directory> m_root;
};

}
