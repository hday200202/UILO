#include "Filebrowser.hpp"

#include <algorithm>
#include <cctype>

namespace uilo {

namespace {

// The panel itself is a plain Column; FileBrowserOptions owns the subset of
// ColumnOptions that makes sense to expose so the widget keeps a single
// Options type in its public API.
ColumnOptions panelOptionsFrom(const FileBrowserOptions& o) {
    return ColumnOptions()
        .setColor(o.getBackgroundColor())
        .setColorRole(o.getBackgroundColorRole())
        .setRounding(o.getRounding())
        .setScrollable(o.getScrollable())
        .setScrollSpeed(o.getScrollSpeed());
}

std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

// Marks an element and everything below it for deletion. Erasing only the
// row would leave its Spacer/Text in UILO's element pool for the lifetime
// of the program, since the pool sweep is driven by the flag alone.
void eraseSubtree(Element* element) {
    if (!element) return;

    const ElementType type = element->getType();
    if (type == ElementType::Row || type == ElementType::Column) {
        auto* container = static_cast<Container*>(element);
        for (auto* child : container->getChildren()) eraseSubtree(child);
    }

    if (element->getUILO()) {
        element->erase();
    } else {
        // Never registered with a UILO (rebuilt before the page walk), so
        // nothing else owns it and the pool sweep will never see it.
        delete element;
    }
}

} // namespace


FileBrowser::FileBrowser(Modifier modifier, FileBrowserOptions options, const std::string& name)
    : Column(modifier, panelOptionsFrom(options), {}, name), m_fbOptions(std::move(options))
{
    if (!m_fbOptions.getRootPath().empty()) {
        m_tree.emplace(fs::path(m_fbOptions.getRootPath()));
        m_expanded.insert(m_tree->getRoot()->getPath().string());
    }
    rebuildRows();
}

void FileBrowser::setOptions(const FileBrowserOptions& opts) {
    const std::string previousRoot = m_fbOptions.getRootPath();
    m_fbOptions = opts;
    applyPanelOptions();

    if (m_fbOptions.getRootPath() != previousRoot) {
        setRootPath(fs::path(m_fbOptions.getRootPath()));
        return;
    }
    m_needsRebuild = true;
}

void FileBrowser::applyPanelOptions() {
    Column::setOptions(panelOptionsFrom(m_fbOptions));
}

void FileBrowser::update(Rectf& parentBounds, float dt) {
    if (m_needsRebuild) rebuildRows();
    Column::update(parentBounds, dt);
}

void FileBrowser::setRootPath(const fs::path& path) {
    m_fbOptions.setRootPath(path.string());
    m_expanded.clear();
    m_selectedPath.clear();

    if (path.empty()) {
        m_tree.reset();
    } else {
        m_tree.emplace(path);
        m_expanded.insert(m_tree->getRoot()->getPath().string());
    }
    m_needsRebuild = true;
}

fs::path FileBrowser::getRootPath() const {
    return m_tree ? m_tree->getRoot()->getPath() : fs::path{};
}

void FileBrowser::refresh() {
    // Drops every cached listing; directories still in m_expanded are
    // re-listed lazily as the rows are rebuilt.
    if (m_tree) m_tree->refresh();
    m_needsRebuild = true;
}

bool FileBrowser::isExpanded(const fs::path& path) const {
    return m_expanded.find(path.string()) != m_expanded.end();
}

void FileBrowser::expandPath(const fs::path& path) {
    if (!m_expanded.insert(path.string()).second) return;
    if (m_fbOptions.getOnDirectoryExpanded()) m_fbOptions.getOnDirectoryExpanded()(path);
    m_needsRebuild = true;
}

void FileBrowser::collapsePath(const fs::path& path) {
    // Nested expansions are intentionally left in the set so reopening a
    // directory restores the shape the user had before collapsing it.
    if (m_expanded.erase(path.string()) == 0) return;
    if (m_fbOptions.getOnDirectoryCollapsed()) m_fbOptions.getOnDirectoryCollapsed()(path);
    m_needsRebuild = true;
}

void FileBrowser::togglePath(const fs::path& path) {
    if (isExpanded(path)) collapsePath(path);
    else                  expandPath(path);
}

void FileBrowser::collapseAll() {
    m_expanded.clear();
    if (m_tree && m_fbOptions.getShowRoot())
        m_expanded.insert(m_tree->getRoot()->getPath().string());
    m_needsRebuild = true;
}

void FileBrowser::setSelectedPath(const fs::path& path) {
    if (m_selectedPath == path) return;
    m_selectedPath = path;
    m_needsRebuild = true;
}

void FileBrowser::clearSelection() {
    if (m_selectedPath.empty()) return;
    m_selectedPath.clear();
    m_needsRebuild = true;
}

void FileBrowser::handleEntryClicked(const fs::path& path, bool isDirectory) {
    if (m_fbOptions.getSelectable()) {
        m_selectedPath = path;
        if (m_fbOptions.getOnEntrySelected()) m_fbOptions.getOnEntrySelected()(path);
    }

    if (isDirectory) {
        if (m_fbOptions.getOnDirectoryClicked()) m_fbOptions.getOnDirectoryClicked()(path);
        if (m_fbOptions.getExpandDirectoryOnClick()) togglePath(path);
    } else {
        if (m_fbOptions.getOnFileClicked()) m_fbOptions.getOnFileClicked()(path);
    }

    m_needsRebuild = true;
}

void FileBrowser::clearRows() {
    for (auto* child : m_children) eraseSubtree(child);
    m_children.clear();
    m_dirty = true;
}

void FileBrowser::rebuildRows() {
    m_needsRebuild = false;
    clearRows();
    if (!m_tree) return;

    Directory* root = m_tree->getRoot();
    if (m_fbOptions.getShowRoot()) {
        addElement(buildRow(root, 0));
        if (isExpanded(root->getPath())) appendDirectory(root, 1);
    } else {
        appendDirectory(root, 0);
    }
}

void FileBrowser::appendDirectory(Directory* dir, int depth) {
    dir->expand();

    std::vector<FSEntry*> entries;
    entries.reserve(dir->getChildren().size());
    for (const auto& child : dir->getChildren()) {
        if (passesFilter(child.get())) entries.push_back(child.get());
    }
    sortEntries(entries);

    for (auto* entry : entries) {
        addElement(buildRow(entry, depth));
        if (entry->isDirectory() && isExpanded(entry->getPath()))
            appendDirectory(static_cast<Directory*>(entry), depth + 1);
    }
}

bool FileBrowser::passesFilter(const FSEntry* entry) const {
    const bool isDir = entry->isDirectory();
    if (isDir  && !m_fbOptions.getShowDirectories()) return false;
    if (!isDir && !m_fbOptions.getShowFiles())       return false;

    if (!m_fbOptions.getShowHidden()) {
        const std::string filename = entry->getPath().filename().string();
        if (!filename.empty() && filename[0] == '.') return false;
    }

    const auto& filter = m_fbOptions.getExtensionFilter();
    if (!isDir && !filter.empty()) {
        const std::string ext = toLower(static_cast<const File*>(entry)->getFileExt());
        const bool matches = std::any_of(filter.begin(), filter.end(),
            [&](const std::string& wanted) {
                std::string w = toLower(wanted);
                if (!w.empty() && w[0] != '.') w.insert(w.begin(), '.');
                return w == ext;
            });
        if (!matches) return false;
    }

    return true;
}

void FileBrowser::sortEntries(std::vector<FSEntry*>& entries) const {
    const FileBrowserSort mode = m_fbOptions.getSortMode();
    const bool descending      = m_fbOptions.getSortDescending();
    const bool dirsFirst       = m_fbOptions.getDirectoriesFirst();
    const bool caseSensitive   = m_fbOptions.getSortCaseSensitive();

    auto displayName = [&](const FSEntry* e) {
        const std::string n = e->getPath().filename().string();
        return caseSensitive ? n : toLower(n);
    };

    auto sizeOf = [](const FSEntry* e) -> size_t {
        if (e->isDirectory()) return 0;
        return static_cast<const File*>(e)->getFileSize().value_or(0);
    };

    auto modifiedOf = [](const FSEntry* e) -> std::time_t {
        if (e->isDirectory()) return 0;
        return static_cast<const File*>(e)->getLastModified().value_or(0);
    };

    std::sort(entries.begin(), entries.end(), [&](const FSEntry* a, const FSEntry* b) {
        if (dirsFirst && a->isDirectory() != b->isDirectory())
            return a->isDirectory();

        bool less;
        switch (mode) {
            case FileBrowserSort::Extension: {
                const std::string ea = a->isDirectory() ? "" : toLower(static_cast<const File*>(a)->getFileExt());
                const std::string eb = b->isDirectory() ? "" : toLower(static_cast<const File*>(b)->getFileExt());
                if (ea != eb) less = ea < eb;
                else          less = displayName(a) < displayName(b);
                break;
            }
            case FileBrowserSort::Size: {
                const size_t sa = sizeOf(a), sb = sizeOf(b);
                if (sa != sb) less = sa < sb;
                else          less = displayName(a) < displayName(b);
                break;
            }
            case FileBrowserSort::Modified: {
                const std::time_t ma = modifiedOf(a), mb = modifiedOf(b);
                if (ma != mb) less = ma < mb;
                else          less = displayName(a) < displayName(b);
                break;
            }
            case FileBrowserSort::Name:
            default:
                less = displayName(a) < displayName(b);
                break;
        }
        return descending ? !less : less;
    });
}

Element* FileBrowser::buildRow(FSEntry* entry, int depth) {
    const FileBrowserOptions& o = m_fbOptions;
    const bool isDir   = entry->isDirectory();
    const fs::path path = entry->getPath();
    const bool selected = o.getSelectable() && !m_selectedPath.empty() && m_selectedPath == path;

    // Label: name (+ extension) only -- the full path is never shown.
    std::string label;
    if (isDir) {
        label = (isExpanded(path) ? o.getExpandedPrefix() : o.getCollapsedPrefix())
              + entry->getName() + o.getDirectorySuffix();
    } else {
        const auto* file = static_cast<const File*>(entry);
        label = o.getFilePrefix() + file->getFileName();
        if (o.getShowExtensions()) label += file->getFileExt();
    }

    Color       baseColor = isDir ? o.getDirectoryBackgroundColor()     : o.getEntryBackgroundColor();
    std::string baseRole  = isDir ? o.getDirectoryBackgroundColorRole() : o.getEntryBackgroundColorRole();
    Color       textColor = isDir ? o.getDirectoryTextColor()           : o.getFileTextColor();
    std::string textRole  = isDir ? o.getDirectoryTextColorRole()       : o.getFileTextColorRole();

    if (selected) {
        baseColor = o.getSelectedColor();
        baseRole  = o.getSelectedColorRole();
        textColor = o.getSelectedTextColor();
        textRole  = o.getSelectedTextColorRole();
    }

    TextOptions textOpts = TextOptions()
        .setFont(o.getFontPath())
        .setContent(label)
        .setCharSize(o.getCharSize())
        .setColor(textColor)
        .setColorRole(textRole)
        .setTextAlignX(Align::Left)
        .setTextAlignY(o.getTextAlignY())
        .setBold(isDir && o.getBoldDirectories());

    auto* labelText = new Text(
        Modifier().setAlign(Align::Left | o.getTextAlignY()),
        textOpts
    );

    Modifier rowModifier = Modifier()
        .setHeight(Dimension{o.getEntryHeight(), false})
        .setOuterPadding(o.getEntryPadding());

    rowModifier.setOnLeftClick([this, path, isDir](Element*) {
        handleEntryClicked(path, isDir);
    });

    // Hover swaps the row background and restores the resolved base on exit.
    // A selected row keeps its highlight because rebuildRows() re-derives
    // both colors from the current selection.
    const Color       hoverColor = o.getHoverColor();
    const std::string hoverRole  = o.getHoverColorRole();
    rowModifier.setOnHoverEnter([hoverColor, hoverRole](Row* r) {
        r->getOptions().setColor(hoverColor).setColorRole(hoverRole);
    });
    rowModifier.setOnHoverExit([baseColor, baseRole](Row* r) {
        r->getOptions().setColor(baseColor).setColorRole(baseRole);
    });

    RowOptions rowOpts = RowOptions()
        .setColor(baseColor)
        .setColorRole(baseRole)
        .setRounding(o.getEntryRounding());

    const float indentWidth = o.getTextPaddingLeft() + o.getIndent() * static_cast<float>(depth);
    if (indentWidth > 0.f) {
        auto* indent = new Spacer(Modifier().setWidth(Dimension{indentWidth, false}));
        return new Row(rowModifier, rowOpts, contains{ indent, labelText });
    }
    return new Row(rowModifier, rowOpts, contains{ labelText });
}

}
