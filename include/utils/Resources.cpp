#include "Resources.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <set>
#include <sstream>

namespace uilo {

/*
    get():
    - Params:   none
    - Returns:  Resources&
    - Desc:     The process-wide registries. A single instance so an icon or
                font registered anywhere is visible everywhere, including to
                elements built before the application set them up.
*/
Resources& Resources::get() {
    static Resources instance;
    return instance;
}

namespace {

/*
    findBuiltIn(std::string_view name):
    - Params:   std::string_view name
    - Returns:  std::string_view
    - Desc:     The generated table is sorted by name, so a built-in lookup
                is a binary search over string_views with no allocation.
*/
std::string_view findBuiltIn(std::string_view name) {
    const auto* begin = std::begin(detail::kEmbeddedIcons);
    const auto* end   = std::end(detail::kEmbeddedIcons);
    const auto* it = std::lower_bound(
        begin, end, name,
        [](const detail::EmbeddedIconEntry& entry, std::string_view target) {
            return entry.name < target;
        });
    if (it != end && it->name == name) return it->markup;
    return {};
}

} // namespace

/*
    find(std::string_view name):
    - Params:   std::string_view name
    - Returns:  std::string_view Resources::
    - Desc:     The SVG markup registered under a name, empty when the name is
                unknown. Application-registered icons are searched before the
                built-in set, so a project can override a built-in by reusing
                its name.
*/
std::string_view Resources::IconRegistry::find(std::string_view name) const {
    if (name.empty()) return {};
    /* Registered entries win, so an application can shadow a built-in by
       re-registering the same name. */
    auto it = m_added.find(name);
    if (it != m_added.end()) return it->second;
    return findBuiltIn(name);
}

/*
    add(std::string name, std::string markup):
    - Params:   std::string name, std::string markup
    - Returns:  void Resources::
    - Desc:     Registers SVG markup under a name, replacing any earlier entry.
*/
void Resources::IconRegistry::add(std::string name, std::string markup) {
    if (name.empty()) return;
    m_added[std::move(name)] = std::move(markup);
}

/*
    addFile(std::string name, const std::filesystem::path& file):
    - Params:   std::string name, const std::filesystem::path& file
    - Returns:  bool Resources::
    - Desc:     Registers an icon from an .svg on disk, read once at call time
                so the file need not outlive the call. Reports false when it
                cannot be read.
*/
bool Resources::IconRegistry::addFile(std::string name, const std::filesystem::path& file) {
    std::ifstream in(file, std::ios::binary);
    if (!in) return false;
    std::ostringstream ss;
    ss << in.rdbuf();
    if (!in && !in.eof()) return false;
    std::string markup = ss.str();
    if (markup.empty()) return false;
    add(std::move(name), std::move(markup));
    return true;
}

/*
    addDirectory(const std::filesystem::path& dir):
    - Params:   const std::filesystem::path& dir
    - Returns:  std::size_t Resources::
    - Desc:     Registers every .svg in a directory, each under its filename
                without the extension. Returns how many were added, so a caller
                can tell an empty directory from a missing one.
*/
std::size_t Resources::IconRegistry::addDirectory(const std::filesystem::path& dir) {
    std::error_code ec;
    std::filesystem::directory_iterator it(dir, ec);
    if (ec) return 0;

    std::size_t added = 0;
    for (const std::filesystem::directory_entry& entry : it) {
        std::error_code fileEc;
        if (!entry.is_regular_file(fileEc) || fileEc) continue;

        std::string ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (ext != ".svg") continue;

        if (addFile(entry.path().stem().string(), entry.path())) ++added;
    }
    return added;
}

/*
    remove(std::string_view name):
    - Params:   std::string_view name
    - Returns:  void Resources::
    - Desc:     Removes an application-registered icon. The built-in set is not
                affected, so a name that was overriding a built-in falls back to
                it.
*/
void Resources::IconRegistry::remove(std::string_view name) {
    auto it = m_added.find(name);
    if (it != m_added.end()) m_added.erase(it);
}

/*
    names():
    - Params:   none
    - Returns:  std::vector<std::string_view> Resources::
    - Desc:     A set rather than a plain append: a registered override
                shares its name with the built-in it shadows and must appear
                once.
*/
std::vector<std::string_view> Resources::IconRegistry::names() const {
    std::set<std::string_view> unique;
    for (const auto& entry : detail::kEmbeddedIcons) unique.insert(entry.name);
    for (const auto& [name, markup] : m_added) {
        (void)markup;
        unique.insert(name);
    }
    return { unique.begin(), unique.end() };
}

/*
    size():
    - Params:   none
    - Returns:  std::size_t Resources::
    - Desc:     How many icons are registered in total, built-ins included.
*/
std::size_t Resources::IconRegistry::size() const {
    return names().size();
}

/*
    builtInCount():
    - Params:   none
    - Returns:  std::size_t Resources::
    - Desc:     How many icons ship with UILO, for telling built-ins from
                additions.
*/
std::size_t Resources::IconRegistry::builtInCount() {
    return detail::kEmbeddedIconCount;
}


/*
    FontRegistry():
    - Params:   none
    - Returns:  none
    - Desc:     An empty path is the renderer's existing signal for "use the
                embedded face", so the default font is a name pointing at
                nothing rather than a copy of the bytes.
*/
Resources::FontRegistry::FontRegistry() {
    m_fonts.emplace(std::string(Resources::fonts::default_), std::string{});
}

/*
    resolve(std::string_view nameOrPath):
    - Params:   std::string_view nameOrPath
    - Returns:  std::string_view Resources::
    - Desc:     Turns a registered font name into its path. Anything not
                registered is handed back unchanged, so a plain path passes
                straight through and callers never have to know which they were
                given.
*/
std::string_view Resources::FontRegistry::resolve(std::string_view nameOrPath) const {
    auto it = m_fonts.find(nameOrPath);
    if (it != m_fonts.end()) return it->second;
    return nameOrPath;
}

/*
    add(std::string name, std::string path):
    - Params:   std::string name, std::string path
    - Returns:  void Resources::
    - Desc:     Registers a font path under a short name.
*/
void Resources::FontRegistry::add(std::string name, std::string path) {
    if (name.empty()) return;
    m_fonts[std::move(name)] = std::move(path);
}

/*
    remove(std::string_view name):
    - Params:   std::string_view name
    - Returns:  void Resources::
    - Desc:     Unregisters a font name.
*/
void Resources::FontRegistry::remove(std::string_view name) {
    auto it = m_fonts.find(name);
    if (it != m_fonts.end()) m_fonts.erase(it);
}

/*
    contains(std::string_view name):
    - Params:   std::string_view name
    - Returns:  bool Resources::
    - Desc:     Whether a name is registered.
*/
bool Resources::FontRegistry::contains(std::string_view name) const {
    return m_fonts.find(name) != m_fonts.end();
}

/*
    names():
    - Params:   none
    - Returns:  std::vector<std::string_view> Resources::
    - Desc:     Every registered font name, for building a font picker.
*/
std::vector<std::string_view> Resources::FontRegistry::names() const {
    std::set<std::string_view> unique;
    for (const auto& [name, path] : m_fonts) {
        (void)path;
        unique.insert(name);
    }
    return { unique.begin(), unique.end() };
}

} // namespace uilo
