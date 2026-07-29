#include "Resources.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <set>
#include <sstream>

namespace uilo {

Resources& Resources::get() {
    static Resources instance;
    return instance;
}

namespace {

// The generated table is sorted by name, so a built-in lookup is a binary
// search over string_views with no allocation.
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

std::string_view Resources::IconRegistry::find(std::string_view name) const {
    if (name.empty()) return {};
    // Registered entries win, so an application can shadow a built-in by
    // re-registering the same name.
    auto it = m_added.find(name);
    if (it != m_added.end()) return it->second;
    return findBuiltIn(name);
}

void Resources::IconRegistry::add(std::string name, std::string markup) {
    if (name.empty()) return;
    m_added[std::move(name)] = std::move(markup);
}

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

void Resources::IconRegistry::remove(std::string_view name) {
    auto it = m_added.find(name);
    if (it != m_added.end()) m_added.erase(it);
}

std::vector<std::string_view> Resources::IconRegistry::names() const {
    // A set rather than a plain append: a registered override shares its name
    // with the built-in it shadows and must appear once.
    std::set<std::string_view> unique;
    for (const auto& entry : detail::kEmbeddedIcons) unique.insert(entry.name);
    for (const auto& [name, markup] : m_added) {
        (void)markup;
        unique.insert(name);
    }
    return { unique.begin(), unique.end() };
}

std::size_t Resources::IconRegistry::size() const {
    return names().size();
}

std::size_t Resources::IconRegistry::builtInCount() {
    return detail::kEmbeddedIconCount;
}


Resources::FontRegistry::FontRegistry() {
    // An empty path is the renderer's existing signal for "use the embedded
    // face", so the default font is a name pointing at nothing rather than a
    // copy of the bytes.
    m_fonts.emplace(std::string(Resources::fonts::default_), std::string{});
}

std::string_view Resources::FontRegistry::resolve(std::string_view nameOrPath) const {
    auto it = m_fonts.find(nameOrPath);
    if (it != m_fonts.end()) return it->second;
    return nameOrPath;
}

void Resources::FontRegistry::add(std::string name, std::string path) {
    if (name.empty()) return;
    m_fonts[std::move(name)] = std::move(path);
}

void Resources::FontRegistry::remove(std::string_view name) {
    auto it = m_fonts.find(name);
    if (it != m_fonts.end()) m_fonts.erase(it);
}

bool Resources::FontRegistry::contains(std::string_view name) const {
    return m_fonts.find(name) != m_fonts.end();
}

std::vector<std::string_view> Resources::FontRegistry::names() const {
    std::set<std::string_view> unique;
    for (const auto& [name, path] : m_fonts) {
        (void)path;
        unique.insert(name);
    }
    return { unique.begin(), unique.end() };
}

} // namespace uilo
