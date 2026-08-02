#include "Filebrowser.hpp"

#include <algorithm>
#include <cctype>

namespace uilo {

namespace {

/*
    panelOptionsFrom(const FileBrowserOptions& o):
    - Params:   const FileBrowserOptions& o
    - Returns:  ColumnOptions
    - Desc:     Builds the options for the panel the entries sit in. The panel
                is a plain Column, and FileBrowserOptions owns the subset of
                ColumnOptions that makes sense to expose, so the widget keeps a
                single Options type in its public API. Rounding is passed
                through unresolved so the panel keeps following the Theme.
*/
ColumnOptions panelOptionsFrom(const FileBrowserOptions& o) {
    return ColumnOptions()
        .setColor(o.getBackgroundColor())
        .setColorRole(o.getBackgroundColorRole())
        .inheritRounding(o.getRoundingOpt(), FileBrowserOptions::getRoundingOptFallback())
        .setScrollable(o.getScrollable())
        .setScrollSpeed(o.getScrollSpeed())
        .setOutlineColor(o.getOutlineColor())
        .setOutlineColorRole(o.getOutlineColorRole())
        .setOutlineThickness(o.getOutlineThickness());
}


/*
    toLower(std::string s):
    - Params:   std::string s
    - Returns:  std::string
    - Desc:     ASCII lowercase, for case-insensitive sorting and extension
                matching.
*/
std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}


/*
    eraseSubtree(Element* element):
    - Params:   Element* element
    - Returns:  void
    - Desc:     Marks an element and everything below it for deletion. Erasing
                only the row would leave its Spacer, Icon and Text in UILO's
                element pool for the lifetime of the program, since the pool
                sweep is driven by the flag alone. An element that was never
                registered with a UILO -- rebuilt before the page walk reached
                it -- has no other owner, so it is deleted outright instead.
*/
void eraseSubtree(Element* element) {
    if (!element) return;

    const ElementType type = element->getType();
    if (type == ElementType::Row || type == ElementType::Column) {
        auto* container = static_cast<Container*>(element);
        for (auto* child : container->getChildren()) eraseSubtree(child);
    }

    if (element->getUILO()) element->erase();
    else                    delete element;
}

} // namespace


/*
    FileBrowser(Modifier modifier, FileBrowserOptions options, const std::string& name):
    - Params:   Modifier modifier, FileBrowserOptions options, const
                std::string& name
    - Returns:  FileBrowser
    - Desc:     Constructs the browser as a Column carrying the panel options,
                opens the tree at the configured root when one was given, and
                builds the first set of entry rows.
*/
FileBrowser::FileBrowser(
    Modifier modifier,
    FileBrowserOptions options,
    const std::string& name
) : Column(modifier, panelOptionsFrom(options), {}, name), m_fbOptions(std::move(options)) {
    if (!m_fbOptions.getRootPath().empty())
        m_tree.emplace(fs::path(m_fbOptions.getRootPath()));
    rebuildRows();
}


/*
    setOptions(const FileBrowserOptions& opts):
    - Params:   const FileBrowserOptions& opts
    - Returns:  void
    - Desc:     Replaces the options, re-applies the panel settings and
                schedules a rebuild of the entry rows. A changed root path is
                handled by setRootPath instead, which also clears expansion and
                selection.
*/
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


/*
    applyPanelOptions():
    - Params:   none
    - Returns:  void
    - Desc:     Pushes the panel half of the widget's options down onto the
                Column it is built from.
*/
void FileBrowser::applyPanelOptions() {
    Column::setOptions(panelOptionsFrom(m_fbOptions));
}


/*
    update(Rectf& parentBounds, float dt):
    - Params:   Rectf& parentBounds, float dt
    - Returns:  void
    - Desc:     Rebuilds the entry rows when one is pending, then lays out as a
                Column. The rebuild happens here rather than at the point that
                asked for it because clicks are dispatched while the parent
                walks its child list, and mutating that list from a click
                handler would invalidate the iteration in progress.
*/
void FileBrowser::update(Rectf& parentBounds, float dt) {
    if (m_needsRebuild) rebuildRows();
    Column::update(parentBounds, dt);
}


/*
    setRootPath(const fs::path& path):
    - Params:   const fs::path& path
    - Returns:  void
    - Desc:     Points the browser at a new root, clearing expansion and
                selection since neither means anything under a different tree.
                An empty path drops the tree entirely, leaving the panel blank.
*/
void FileBrowser::setRootPath(const fs::path& path) {
    m_fbOptions.setRootPath(path.string());
    m_expanded.clear();
    m_selectedPath.clear();

    if (path.empty()) m_tree.reset();
    else              m_tree.emplace(path);
    m_needsRebuild = true;
}


/*
    getRootPath():
    - Params:   none
    - Returns:  fs::path
    - Desc:     The root the tree is currently open at, empty when there is no
                tree.
*/
fs::path FileBrowser::getRootPath() const {
    return m_tree ? m_tree->getRoot()->getPath() : fs::path{};
}


/*
    refresh():
    - Params:   none
    - Returns:  void
    - Desc:     Re-scans the disk, keeping expansion state. Every cached listing
                is dropped; the directories still in the expanded set are re-
                listed lazily as the rows are rebuilt, so what the user had open
                comes back.
*/
void FileBrowser::refresh() {
    if (m_tree) m_tree->refresh();
    m_needsRebuild = true;
}


/*
    isExpanded(const fs::path& path):
    - Params:   const fs::path& path
    - Returns:  bool
    - Desc:     Whether a directory is currently open.
*/
bool FileBrowser::isExpanded(const fs::path& path) const {
    return m_expanded.find(path.string()) != m_expanded.end();
}


/*
    expandPath(const fs::path& path):
    - Params:   const fs::path& path
    - Returns:  void
    - Desc:     Opens a directory and fires onDirectoryExpanded. Opening one
                that is already open is a no-op, so the callback fires on the
                transition rather than on every click.
*/
void FileBrowser::expandPath(const fs::path& path) {
    if (!m_expanded.insert(path.string()).second) return;
    if (m_fbOptions.getOnDirectoryExpanded()) m_fbOptions.getOnDirectoryExpanded()(path);
    m_needsRebuild = true;
}


/*
    collapsePath(const fs::path& path):
    - Params:   const fs::path& path
    - Returns:  void
    - Desc:     Closes a directory and fires onDirectoryCollapsed. Expansions
                nested inside it are deliberately left in the set, so reopening
                the directory restores the shape the user had before collapsing
                it.
*/
void FileBrowser::collapsePath(const fs::path& path) {
    if (m_expanded.erase(path.string()) == 0) return;
    if (m_fbOptions.getOnDirectoryCollapsed()) m_fbOptions.getOnDirectoryCollapsed()(path);
    m_needsRebuild = true;
}


/*
    togglePath(const fs::path& path):
    - Params:   const fs::path& path
    - Returns:  void
    - Desc:     Opens a closed directory or closes an open one.
*/
void FileBrowser::togglePath(const fs::path& path) {
    if (isExpanded(path)) collapsePath(path);
    else                  expandPath(path);
}


/*
    collapseAll():
    - Params:   none
    - Returns:  void
    - Desc:     Closes every directory. The root is deliberately absent from the
                expanded set -- it is the header, and its children are always
                the top level of the list -- so this collapses back to that
                level rather than to nothing.
*/
void FileBrowser::collapseAll() {
    m_expanded.clear();
    m_needsRebuild = true;
}


/*
    setSelectedPath(const fs::path& path):
    - Params:   const fs::path& path
    - Returns:  void
    - Desc:     Marks an entry as selected, rebuilding so the row picks up the
                selection colours.
*/
void FileBrowser::setSelectedPath(const fs::path& path) {
    if (m_selectedPath == path) return;
    m_selectedPath = path;
    m_needsRebuild = true;
}


/*
    clearSelection():
    - Params:   none
    - Returns:  void
    - Desc:     Drops the selection, so no row is highlighted.
*/
void FileBrowser::clearSelection() {
    if (m_selectedPath.empty()) return;
    m_selectedPath.clear();
    m_needsRebuild = true;
}


/*
    handleEntryClicked(const fs::path& path, bool isDirectory):
    - Params:   const fs::path& path, bool isDirectory
    - Returns:  void
    - Desc:     The one place a click on any entry row lands. Updates the
                selection when the browser is selectable, then fires the
                callback for the kind of entry that was hit and expands or
                collapses a directory if that is configured.
*/
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


/*
    clearRows():
    - Params:   none
    - Returns:  void
    - Desc:     Marks every row and its contents for deletion and empties the
                child list.
*/
void FileBrowser::clearRows() {
    for (auto* child : m_children) eraseSubtree(child);
    m_children.clear();
    m_dirty = true;
}


/*
    rebuildRows():
    - Params:   none
    - Returns:  void
    - Desc:     Rebuilds the whole list from the tree and the current expansion
                state. The header goes in first and is marked ignoreScroll, so
                the scrollable Column pins it to the top and scrolls only the
                entries beneath it. The root itself is never a row: its children
                are the top level of the list.
*/
void FileBrowser::rebuildRows() {
    m_needsRebuild = false;
    clearRows();
    if (!m_tree) return;

    if (m_fbOptions.getShowHeader()) addElement(buildHeader());

    appendDirectory(m_tree->getRoot(), 0);
}


/*
    rootDisplayName():
    - Params:   none
    - Returns:  std::string
    - Desc:     Display name for the root. A relative root has no useful
                filename ("../" gives ".."), so the path is resolved to an
                absolute one and normalised first, which makes it read as the
                folder it actually points at. Normalising leaves a trailing
                separator on a directory and so an empty filename, in which case
                the parent's name is used; at a filesystem root even that is
                empty, and the path itself is shown.
*/
std::string FileBrowser::rootDisplayName() const {
    if (!m_fbOptions.getHeaderText().empty()) return m_fbOptions.getHeaderText();
    if (!m_tree) return {};

    const fs::path& root = m_tree->getRoot()->getPath();

    std::error_code ec;
    fs::path resolved = fs::absolute(root, ec);
    if (ec) resolved = root;
    resolved = resolved.lexically_normal();

    std::string name = resolved.filename().string();
    if (name.empty()) name = resolved.parent_path().filename().string();
    if (name.empty()) name = resolved.string();
    return name;
}


/*
    buildHeader():
    - Params:   none
    - Returns:  Element* -- the pinned title strip naming the root
    - Desc:     Builds the header row: an optional left inset, the root's icon,
                a gap, and the root's name. Marked ignoreScroll so the panel
                pins it above the scrolling list.
*/
Element* FileBrowser::buildHeader() {
    const FileBrowserOptions& o = m_fbOptions;

    const unsigned int charSize = o.hasHeaderCharSize() ? o.getHeaderCharSize()
                                                        : o.getCharSize();

    auto* label = new Text(
        Modifier().setAlign(Align::Left | Align::CenterY),
        TextOptions()
            .setFont(o.getFontPath())
            .setContent(rootDisplayName())
            .setCharSize(charSize)
            .setColor(o.getHeaderTextColor())
            .setColorRole(o.getHeaderTextColorRole())
            .setTextAlignX(Align::Left)
            .setTextAlignY(Align::CenterY)
            .setBold(o.getHeaderBold()));

    std::vector<Element*> children;
    children.reserve(4);

    if (o.getTextPaddingLeft() > 0.f)
        children.push_back(new Spacer(
            Modifier().setWidth(Dimension{o.getTextPaddingLeft(), false})));

    if (o.getShowIcons() && !o.getHeaderIcon().empty()) {
        IconOptions iconOpts = IconOptions()
            .setIcon(o.getHeaderIcon())
            .setColor(o.getHeaderTextColor())
            .setColorRole(o.getHeaderTextColorRole());
        if (o.hasIconStrokeWidth()) iconOpts.setStrokeWidth(o.getIconStrokeWidth());

        children.push_back(new Icon(
            Modifier()
                .setWidth(Dimension{o.getIconSize(), false})
                .setHeight(Dimension{o.getIconSize(), false})
                .setAlign(Align::Left | Align::CenterY),
            iconOpts));

        if (o.getIconSpacing() > 0.f)
            children.push_back(new Spacer(
                Modifier().setWidth(Dimension{o.getIconSpacing(), false})));
    }

    children.push_back(label);

    auto* header = new Row(
        Modifier()
            .setHeight(Dimension{o.getHeaderHeight(), false})
            .setOuterPadding(o.getHeaderPadding())
            .ignoreScroll(true),
        RowOptions()
            .setColor(o.getHeaderColor())
            .setColorRole(o.getHeaderColorRole())
            .inheritRounding(o.getHeaderRoundingOpt(),
                             FileBrowserOptions::getHeaderRoundingOptFallback()),
        contains{});
    for (Element* child : children) header->addElement(child);
    return header;
}


/*
    appendDirectory(Directory* dir, int depth):
    - Params:   Directory* dir, int depth
    - Returns:  void
    - Desc:     Appends a row for each of a directory's filtered, sorted
                entries, recursing into any child directory the user has opened.
                Listing a directory is what pulls it off disk the first time;
                afterwards the FileTree serves it from cache.
*/
void FileBrowser::appendDirectory(Directory* dir, int depth) {
    dir->expand();

    std::vector<FSEntry*> entries;
    entries.reserve(dir->getChildren().size());
    for (const auto& child : dir->getChildren()) {
        if (passesFilter(child.get())) entries.push_back(child.get());
    }
    sortEntries(entries);

    for (auto* entry : entries) {
        addEntryRow(buildRow(entry, depth));
        if (entry->isDirectory() && isExpanded(entry->getPath()))
            appendDirectory(static_cast<Directory*>(entry), depth + 1);
    }
}


/*
    passesFilter(const FSEntry* entry):
    - Params:   const FSEntry* entry
    - Returns:  bool
    - Desc:     Whether an entry should be listed at all, testing the
                directory/file toggles, the hidden-file rule, and the extension
                filter. Extensions may be given with or without a leading dot
                and are matched case-insensitively; an empty filter lists
                everything.
*/
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


/*
    sortEntries(std::vector<FSEntry*>& entries):
    - Params:   std::vector<FSEntry*>& entries
    - Returns:  void
    - Desc:     Orders one directory's entries by the configured mode.
                Directories sort ahead of files first when directoriesFirst is
                on, which is what keeps the tree readable; within a group the
                mode decides, and every mode falls back to the name so the order
                is stable when two entries tie. Size and modification time read
                as 0 for a directory, since neither is listed for one.
*/
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


/*
    appendEntryIcon(...):
    - Params:   std::vector<Element*>& children, const FSEntry* entry, const
                Color& textColor, const std::string& textRole, bool selected
    - Returns:  void
    - Desc:     Appends the icon that marks what kind of entry this is, followed
                by the gap before the label. An untinted icon rides the entry's
                own text colour, so selection and theming carry the glyph along
                without extra configuration; a configured icon colour is dropped
                on a selected row so the selection reads as one block.
*/
void FileBrowser::appendEntryIcon(
    std::vector<Element*>& children,
    const FSEntry* entry,
    const Color& textColor,
    const std::string& textRole,
    bool selected
) const {
    const FileBrowserOptions& o = m_fbOptions;
    if (!o.getShowIcons()) return;

    const std::string& iconName = iconNameFor(entry);
    if (iconName.empty()) return;

    const bool tinted = o.hasIconColor() && !selected;
    IconOptions iconOpts = IconOptions()
        .setIcon(iconName)
        .setColor(tinted ? o.getIconColor() : textColor)
        .setColorRole(tinted ? o.getIconColorRole() : textRole);
    if (o.hasIconStrokeWidth()) iconOpts.setStrokeWidth(o.getIconStrokeWidth());

    children.push_back(new Icon(
        Modifier()
            .setWidth(Dimension{o.getIconSize(), false})
            .setHeight(Dimension{o.getIconSize(), false})
            .setAlign(Align::Left | Align::CenterY),
        iconOpts));

    if (o.getIconSpacing() > 0.f)
        children.push_back(new Spacer(
            Modifier().setWidth(Dimension{o.getIconSpacing(), false})));
}


/*
    appendDirectoryArrow(...):
    - Params:   std::vector<Element*>& children, const FSEntry* entry, const
                Color& textColor, const std::string& textRole, bool selected
    - Returns:  void
    - Desc:     Appends the arrow that marks whether a directory is open. It
                goes in after the label, which is the only percent-sized child
                of the row: the label takes whatever width the fixed children
                leave, so the arrow ends up pinned to the right edge however
                wide the row is. A file has nothing to expand and reserves the
                arrow's width with a Spacer instead, which keeps the right-hand
                gutter -- and so the point where a long name clips -- the same
                on every row. The arrow carries no handlers of its own, so
                clicking it falls through to the row and toggles the directory.
*/
void FileBrowser::appendDirectoryArrow(
    std::vector<Element*>& children,
    const FSEntry* entry,
    const Color& textColor,
    const std::string& textRole,
    bool selected
) const {
    const FileBrowserOptions& o = m_fbOptions;

    /* Nothing to draw: reserve the same width so the label still clips where a
       directory's does. */
    auto reserveBlock = [&] {
        const float block = o.getDirectoryArrowBlockWidth();
        if (block > 0.f)
            children.push_back(new Spacer(Modifier().setWidth(Dimension{block, false})));
    };

    if (!entry->isDirectory()) { reserveBlock(); return; }

    /* The open glyph is optional: with only the closed one set, the arrow stays
       put on expanding rather than disappearing. */
    const std::string& openIcon   = o.getDirectoryArrowOpenIcon();
    const std::string& closedIcon = o.getDirectoryArrowIcon();
    const std::string& iconName   = (isExpanded(entry->getPath()) && !openIcon.empty())
        ? openIcon
        : closedIcon;

    if (iconName.empty()) { reserveBlock(); return; }

    const bool tinted = o.hasDirectoryArrowColor() && !selected;
    IconOptions arrowOpts = IconOptions()
        .setIcon(iconName)
        .setColor(tinted ? o.getDirectoryArrowColor() : textColor)
        .setColorRole(tinted ? o.getDirectoryArrowColorRole() : textRole);
    if (o.hasDirectoryArrowStrokeWidth())   arrowOpts.setStrokeWidth(o.getDirectoryArrowStrokeWidth());
    else if (o.hasIconStrokeWidth())        arrowOpts.setStrokeWidth(o.getIconStrokeWidth());

    if (o.getDirectoryArrowSpacing() > 0.f)
        children.push_back(new Spacer(
            Modifier().setWidth(Dimension{o.getDirectoryArrowSpacing(), false})));

    children.push_back(new Icon(
        Modifier()
            .setWidth(Dimension{o.getDirectoryArrowSize(), false})
            .setHeight(Dimension{o.getDirectoryArrowSize(), false})
            .setAlign(Align::Left | Align::CenterY),
        arrowOpts));

    if (o.getDirectoryArrowPadding() > 0.f)
        children.push_back(new Spacer(
            Modifier().setWidth(Dimension{o.getDirectoryArrowPadding(), false})));
}


/*
    buildRow(FSEntry* entry, int depth):
    - Params:   FSEntry* entry, int depth
    - Returns:  Element* -- the row for one entry
    - Desc:     Builds one entry row: indent, icon, gap, label, and the
                directory arrow. The label is name and extension only -- the
                full path is never shown. Colours are derived from whether the
                entry is a directory and whether it is the selected row, and
                every child before the arrow is decoration with no handlers of
                its own, so the row's own click always wins. Hover swaps the row
                background and restores the resolved base on exit; a selected
                row keeps its highlight because a rebuild re-derives both
                colours from the current selection.
*/
Element* FileBrowser::buildRow(FSEntry* entry, int depth) {
    const FileBrowserOptions& o = m_fbOptions;
    const bool isDir    = entry->isDirectory();
    const fs::path path = entry->getPath();
    const bool selected = o.getSelectable() && !m_selectedPath.empty() && m_selectedPath == path;

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

    auto* labelText = new Text(
        Modifier().setAlign(Align::Left | o.getTextAlignY()),
        TextOptions()
            .setFont(o.getFontPath())
            .setContent(label)
            .setCharSize(o.getCharSize())
            .setColor(textColor)
            .setColorRole(textRole)
            .setTextAlignX(Align::Left)
            .setTextAlignY(o.getTextAlignY())
            .setBold(isDir && o.getBoldDirectories()));

    Modifier rowModifier = Modifier()
        .setHeight(Dimension{o.getEntryHeight(), false})
        .setOuterPadding(o.getEntryPadding());

    rowModifier.setOnLeftClick([this, path, isDir](Element*) {
        handleEntryClicked(path, isDir);
    });

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
        .inheritRounding(o.getEntryRoundingOpt(),
                         FileBrowserOptions::getEntryRoundingOptFallback());

    std::vector<Element*> children;
    children.reserve(8);

    const float indentWidth = o.getTextPaddingLeft() + o.getIndent() * static_cast<float>(depth);
    if (indentWidth > 0.f)
        children.push_back(new Spacer(Modifier().setWidth(Dimension{indentWidth, false})));

    appendEntryIcon(children, entry, textColor, textRole, selected);
    children.push_back(labelText);
    if (o.getShowDirectoryArrow())
        appendDirectoryArrow(children, entry, textColor, textRole, selected);

    auto* row = new Row(rowModifier, rowOpts, contains{});
    for (Element* child : children) row->addElement(child);
    return row;
}


/*
    addEntryRow(Element* row):
    - Params:   Element* row
    - Returns:  void
    - Desc:     Appends a row, preceded by the entry-spacing gap when one is
                configured and this is not the first row. The gap is a Spacer
                rather than a taller slot, so it stays out of the row's own
                bounds and is neither clickable nor part of the hover target.
*/
void FileBrowser::addEntryRow(Element* row) {
    const float spacing = m_fbOptions.getEntrySpacing();
    if (spacing > 0.f && !m_children.empty())
        addElement(new Spacer(Modifier().setHeight(Dimension{spacing, false})));
    addElement(row);
}


/*
    iconNameFor(const FSEntry* entry):
    - Params:   const FSEntry* entry
    - Returns:  const std::string&
    - Desc:     Which registry icon represents an entry: the folder icon for a
                directory, or its open variant when one is configured and the
                directory is expanded; otherwise the FileKind mapping when kind
                icons are on, falling back to the plain file icon.
*/
const std::string& FileBrowser::iconNameFor(const FSEntry* entry) const {
    const FileBrowserOptions& o = m_fbOptions;

    if (entry->isDirectory()) {
        const bool open = isExpanded(entry->getPath());
        if (open && !o.getDirectoryOpenIcon().empty()) return o.getDirectoryOpenIcon();
        return o.getDirectoryIcon();
    }

    if (o.getUseFileKindIcons()) {
        const auto* file = static_cast<const File*>(entry);
        const std::string& byKind = o.getIconForKind(file->getFileKind());
        if (!byKind.empty()) return byKind;
    }
    return o.getFileIcon();
}

}
