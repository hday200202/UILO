#pragma once

#include <functional>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

#include "../containers/Column.hpp"
#include "../containers/Row.hpp"
#include "../decoration/Icon.hpp"
#include "../decoration/Image.hpp"
#include "../decoration/Spacer.hpp"
#include "../decoration/Text.hpp"
#include "../../utils/FileTree.hpp"

namespace uilo {

/*
    FileBrowserSort
    - Ordering applied to the entries of each directory. Size and Modified
      stat each file the first time a directory is listed; Name and Extension
      are free (they only read the cached path).
*/
enum class FileBrowserSort { Name, Extension, Size, Modified };


/*
    FileBrowserOptions
    - Everything the widget draws is configurable here. Colors come in
      literal + role pairs like the rest of the library: the role wins when
      it resolves against the active Palette, otherwise the literal is used.
*/
class FileBrowserOptions {
public:
    FileBrowserOptions() = default;

    // Source ------------------------------------------------------------
    FileBrowserOptions& setRootPath(const std::string& p)   { m_rootPath = p; return *this; }

    // Panel (the scrollable surface behind the entries) ------------------
    FileBrowserOptions& setBackgroundColor(const Color& c)          { m_bgColor = c; return *this; }
    FileBrowserOptions& setBackgroundColorRole(const std::string& r){ m_bgColorRole = r; return *this; }
    FileBrowserOptions& setRounding(float r)                        { m_rounding = r; return *this; }
    FileBrowserOptions& setScrollable(bool v)                       { m_scrollable = v; return *this; }
    FileBrowserOptions& setScrollSpeed(float s)                     { m_scrollSpeed = s; return *this; }

    // Entry metrics -----------------------------------------------------
    // THE control for how tightly entries pack: one entry occupies exactly this
    // much vertical space, so the distance between two labels is this value and
    // nothing else. Lower it to tighten the list (36 -> 24 is noticeably
    // denser). Keep it above setIconSize(), or the icon overflows its row.
    FileBrowserOptions& setEntryHeight(float px)        { m_entryHeight = px; return *this; }
    // Uniform inset on all four sides of the row *within* its slot. This does
    // NOT move entries closer together or further apart -- the slot is still
    // entryHeight tall and the label stays centred in it. What it changes is the
    // horizontal inset of the icon and label, and how far the hover/selection
    // fill stops short of the slot edges. With a transparent entry background
    // (the default) it has no visible vertical effect at all.
    FileBrowserOptions& setEntryPadding(float px)       { m_entryPadding = px; return *this; }
    // Extra dead space between consecutive rows, added on top of entryHeight.
    // Only useful for spreading entries further apart than their slot height;
    // to pack them tighter, lower setEntryHeight() instead.
    FileBrowserOptions& setEntrySpacing(float px)       { m_entrySpacing = px; return *this; }
    FileBrowserOptions& setEntryRounding(float r)       { m_entryRounding = r; return *this; }
    FileBrowserOptions& setIndent(float px)             { m_indent = px; return *this; }
    FileBrowserOptions& setTextPaddingLeft(float px)    { m_textPaddingLeft = px; return *this; }

    // Entry text --------------------------------------------------------
    FileBrowserOptions& setFont(const std::string& path){ m_fontPath = path; return *this; }
    FileBrowserOptions& setCharSize(unsigned int n)     { m_charSize = n; return *this; }
    FileBrowserOptions& setTextAlignY(Align a)          { m_textAlignY = a; return *this; }
    FileBrowserOptions& setBoldDirectories(bool v)      { m_boldDirectories = v; return *this; }

    // Entry icons -------------------------------------------------------
    // An icon ahead of each name is what separates a folder from a file, so
    // neither has to be tinted differently to be recognisable.
    FileBrowserOptions& setShowIcons(bool v)        { m_showIcons = v; return *this; }
    FileBrowserOptions& setIconSize(float px)       { m_iconSize = px; return *this; }
    FileBrowserOptions& setIconSpacing(float px)    { m_iconSpacing = px; return *this; }
    FileBrowserOptions& setIconStrokeWidth(float w) { m_iconStrokeWidth = w; m_hasIconStroke = true; return *this; }
    // Any name the Resources icon registry knows.
    FileBrowserOptions& setDirectoryIcon(std::string_view n) { m_dirIcon = std::string(n); return *this; }
    // Shown for a directory that is currently open, when set; otherwise an
    // open directory keeps setDirectoryIcon().
    FileBrowserOptions& setDirectoryOpenIcon(std::string_view n) { m_dirOpenIcon = std::string(n); return *this; }
    FileBrowserOptions& setFileIcon(std::string_view n)      { m_fileIcon = std::string(n); return *this; }
    // With this on (the default) a file's icon comes from its FileKind, so an
    // image reads as an image and a zip as an archive. Off, every file uses
    // setFileIcon().
    FileBrowserOptions& setUseFileKindIcons(bool v)          { m_useKindIcons = v; return *this; }
    // Overrides the icon for one FileKind.
    FileBrowserOptions& setIconForKind(FileKind kind, std::string_view n) {
        m_kindIcons[static_cast<std::size_t>(kind)] = std::string(n);
        return *this;
    }
    // Icons follow the entry's text colour unless given one of their own.
    FileBrowserOptions& setIconColor(const Color& c)           { m_iconColor = c; m_hasIconColor = true; return *this; }
    FileBrowserOptions& setIconColorRole(const std::string& r) { m_iconColorRole = r; m_hasIconColor = true; return *this; }

    // Entry backgrounds -------------------------------------------------
    FileBrowserOptions& setEntryBackgroundColor(const Color& c)           { m_entryColor = c; return *this; }
    FileBrowserOptions& setEntryBackgroundColorRole(const std::string& r) { m_entryColorRole = r; return *this; }
    FileBrowserOptions& setDirectoryBackgroundColor(const Color& c)           { m_dirColor = c; return *this; }
    FileBrowserOptions& setDirectoryBackgroundColorRole(const std::string& r) { m_dirColorRole = r; return *this; }
    FileBrowserOptions& setHoverColor(const Color& c)            { m_hoverColor = c; return *this; }
    FileBrowserOptions& setHoverColorRole(const std::string& r)  { m_hoverColorRole = r; return *this; }
    FileBrowserOptions& setSelectedColor(const Color& c)           { m_selectedColor = c; return *this; }
    FileBrowserOptions& setSelectedColorRole(const std::string& r) { m_selectedColorRole = r; return *this; }

    // Entry text colors -------------------------------------------------
    FileBrowserOptions& setFileTextColor(const Color& c)            { m_fileTextColor = c; return *this; }
    FileBrowserOptions& setFileTextColorRole(const std::string& r)  { m_fileTextColorRole = r; return *this; }
    FileBrowserOptions& setDirectoryTextColor(const Color& c)           { m_dirTextColor = c; return *this; }
    FileBrowserOptions& setDirectoryTextColorRole(const std::string& r) { m_dirTextColorRole = r; return *this; }
    FileBrowserOptions& setSelectedTextColor(const Color& c)           { m_selectedTextColor = c; return *this; }
    FileBrowserOptions& setSelectedTextColorRole(const std::string& r) { m_selectedTextColorRole = r; return *this; }

    // Labels ------------------------------------------------------------
    // Prefixes are drawn ahead of the name; defaults are empty so the widget
    // never depends on a glyph the configured font might not carry.
    FileBrowserOptions& setExpandedPrefix(const std::string& s)  { m_expandedPrefix = s; return *this; }
    FileBrowserOptions& setCollapsedPrefix(const std::string& s) { m_collapsedPrefix = s; return *this; }
    FileBrowserOptions& setFilePrefix(const std::string& s)      { m_filePrefix = s; return *this; }
    FileBrowserOptions& setDirectorySuffix(const std::string& s) { m_directorySuffix = s; return *this; }
    FileBrowserOptions& setShowExtensions(bool v)                { m_showExtensions = v; return *this; }

    // Header ------------------------------------------------------------
    // The root is a title strip pinned to the top of the panel, not a row in
    // the list: it is where you already are, so there is nothing to expand or
    // collapse. The list below it is the root's children.
    FileBrowserOptions& setShowHeader(bool v)        { m_showHeader = v; return *this; }
    // Defaults to the root directory's own name.
    FileBrowserOptions& setHeaderText(const std::string& s) { m_headerText = s; return *this; }
    FileBrowserOptions& setHeaderHeight(float px)    { m_headerHeight = px; return *this; }
    FileBrowserOptions& setHeaderPadding(float px)   { m_headerPadding = px; return *this; }
    FileBrowserOptions& setHeaderRounding(float r)   { m_headerRounding = r; return *this; }
    FileBrowserOptions& setHeaderCharSize(unsigned int n) { m_headerCharSize = n; m_hasHeaderCharSize = true; return *this; }
    FileBrowserOptions& setHeaderBold(bool v)        { m_headerBold = v; return *this; }
    // Empty hides the header icon.
    FileBrowserOptions& setHeaderIcon(std::string_view n)  { m_headerIcon = std::string(n); return *this; }
    FileBrowserOptions& setHeaderColor(const Color& c)           { m_headerColor = c; return *this; }
    FileBrowserOptions& setHeaderColorRole(const std::string& r) { m_headerColorRole = r; return *this; }
    FileBrowserOptions& setHeaderTextColor(const Color& c)           { m_headerTextColor = c; return *this; }
    FileBrowserOptions& setHeaderTextColorRole(const std::string& r) { m_headerTextColorRole = r; return *this; }

    // Contents ----------------------------------------------------------
    FileBrowserOptions& setShowHidden(bool v)       { m_showHidden = v; return *this; }
    FileBrowserOptions& setShowFiles(bool v)        { m_showFiles = v; return *this; }
    FileBrowserOptions& setShowDirectories(bool v)  { m_showDirectories = v; return *this; }
    // Only files whose extension appears here are listed. Entries may be
    // given with or without the leading dot; matching is case-insensitive.
    // An empty filter (the default) lists everything.
    FileBrowserOptions& setExtensionFilter(const std::vector<std::string>& exts) { m_extFilter = exts; return *this; }

    // Ordering ----------------------------------------------------------
    FileBrowserOptions& setSortMode(FileBrowserSort m)  { m_sortMode = m; return *this; }
    FileBrowserOptions& setSortDescending(bool v)       { m_sortDescending = v; return *this; }
    FileBrowserOptions& setDirectoriesFirst(bool v)     { m_directoriesFirst = v; return *this; }
    FileBrowserOptions& setSortCaseSensitive(bool v)    { m_sortCaseSensitive = v; return *this; }

    // Behavior ----------------------------------------------------------
    FileBrowserOptions& setSelectable(bool v)               { m_selectable = v; return *this; }
    FileBrowserOptions& setExpandDirectoryOnClick(bool v)   { m_expandOnClick = v; return *this; }

    // Callbacks ---------------------------------------------------------
    using PathCallback = std::function<void(const fs::path&)>;
    FileBrowserOptions& setOnEntrySelected(PathCallback cb)      { m_onEntrySelected = std::move(cb); return *this; }
    FileBrowserOptions& setOnFileClicked(PathCallback cb)        { m_onFileClicked = std::move(cb); return *this; }
    FileBrowserOptions& setOnDirectoryClicked(PathCallback cb)   { m_onDirectoryClicked = std::move(cb); return *this; }
    FileBrowserOptions& setOnDirectoryExpanded(PathCallback cb)  { m_onDirExpanded = std::move(cb); return *this; }
    FileBrowserOptions& setOnDirectoryCollapsed(PathCallback cb) { m_onDirCollapsed = std::move(cb); return *this; }

    // Getters -----------------------------------------------------------
    const std::string& getRootPath()        const { return m_rootPath; }

    Color              getBackgroundColor()     const { return m_bgColor; }
    const std::string& getBackgroundColorRole() const { return m_bgColorRole; }
    float              getRounding()            const { return m_rounding; }
    bool               getScrollable()          const { return m_scrollable; }
    float              getScrollSpeed()         const { return m_scrollSpeed; }

    float getEntryHeight()      const { return m_entryHeight; }
    float getEntryPadding()     const { return m_entryPadding; }
    float getEntrySpacing()     const { return m_entrySpacing; }
    float getEntryRounding()    const { return m_entryRounding; }
    float getIndent()           const { return m_indent; }
    float getTextPaddingLeft()  const { return m_textPaddingLeft; }

    const std::string& getFontPath()    const { return m_fontPath; }
    unsigned int getCharSize()          const { return m_charSize; }
    Align getTextAlignY()               const { return m_textAlignY; }
    bool  getBoldDirectories()          const { return m_boldDirectories; }

    bool  getShowIcons()        const { return m_showIcons; }
    float getIconSize()         const { return m_iconSize; }
    float getIconSpacing()      const { return m_iconSpacing; }
    float getIconStrokeWidth()  const { return m_iconStrokeWidth; }
    bool  hasIconStrokeWidth()  const { return m_hasIconStroke; }
    const std::string& getDirectoryIcon()     const { return m_dirIcon; }
    const std::string& getDirectoryOpenIcon() const { return m_dirOpenIcon; }
    const std::string& getFileIcon()          const { return m_fileIcon; }
    bool  getUseFileKindIcons() const { return m_useKindIcons; }
    const std::string& getIconForKind(FileKind kind) const {
        return m_kindIcons[static_cast<std::size_t>(kind)];
    }
    Color              getIconColor()     const { return m_iconColor; }
    const std::string& getIconColorRole() const { return m_iconColorRole; }
    bool               hasIconColor()     const { return m_hasIconColor; }

    Color              getEntryBackgroundColor()     const { return m_entryColor; }
    const std::string& getEntryBackgroundColorRole() const { return m_entryColorRole; }
    Color              getDirectoryBackgroundColor()     const { return m_dirColor; }
    const std::string& getDirectoryBackgroundColorRole() const { return m_dirColorRole; }
    Color              getHoverColor()          const { return m_hoverColor; }
    const std::string& getHoverColorRole()      const { return m_hoverColorRole; }
    Color              getSelectedColor()       const { return m_selectedColor; }
    const std::string& getSelectedColorRole()   const { return m_selectedColorRole; }

    Color              getFileTextColor()           const { return m_fileTextColor; }
    const std::string& getFileTextColorRole()       const { return m_fileTextColorRole; }
    Color              getDirectoryTextColor()      const { return m_dirTextColor; }
    const std::string& getDirectoryTextColorRole()  const { return m_dirTextColorRole; }
    Color              getSelectedTextColor()       const { return m_selectedTextColor; }
    const std::string& getSelectedTextColorRole()   const { return m_selectedTextColorRole; }

    const std::string& getExpandedPrefix()  const { return m_expandedPrefix; }
    const std::string& getCollapsedPrefix() const { return m_collapsedPrefix; }
    const std::string& getFilePrefix()      const { return m_filePrefix; }
    const std::string& getDirectorySuffix() const { return m_directorySuffix; }
    bool  getShowExtensions()               const { return m_showExtensions; }

    bool getShowHeader()        const { return m_showHeader; }
    const std::string& getHeaderText() const { return m_headerText; }
    float getHeaderHeight()     const { return m_headerHeight; }
    float getHeaderPadding()    const { return m_headerPadding; }
    float getHeaderRounding()   const { return m_headerRounding; }
    unsigned int getHeaderCharSize() const { return m_headerCharSize; }
    bool  hasHeaderCharSize()   const { return m_hasHeaderCharSize; }
    bool  getHeaderBold()       const { return m_headerBold; }
    const std::string& getHeaderIcon()          const { return m_headerIcon; }
    Color              getHeaderColor()         const { return m_headerColor; }
    const std::string& getHeaderColorRole()     const { return m_headerColorRole; }
    Color              getHeaderTextColor()     const { return m_headerTextColor; }
    const std::string& getHeaderTextColorRole() const { return m_headerTextColorRole; }
    bool getShowHidden()        const { return m_showHidden; }
    bool getShowFiles()         const { return m_showFiles; }
    bool getShowDirectories()   const { return m_showDirectories; }
    const std::vector<std::string>& getExtensionFilter() const { return m_extFilter; }

    FileBrowserSort getSortMode()   const { return m_sortMode; }
    bool getSortDescending()        const { return m_sortDescending; }
    bool getDirectoriesFirst()      const { return m_directoriesFirst; }
    bool getSortCaseSensitive()     const { return m_sortCaseSensitive; }

    bool getSelectable()                const { return m_selectable; }
    bool getExpandDirectoryOnClick()    const { return m_expandOnClick; }

    const PathCallback& getOnEntrySelected()        const { return m_onEntrySelected; }
    const PathCallback& getOnFileClicked()          const { return m_onFileClicked; }
    const PathCallback& getOnDirectoryClicked()     const { return m_onDirectoryClicked; }
    const PathCallback& getOnDirectoryExpanded()    const { return m_onDirExpanded; }
    const PathCallback& getOnDirectoryCollapsed()   const { return m_onDirCollapsed; }

private:
    std::string m_rootPath;

    Color       m_bgColor       = Color{0, 0, 0, 0};
    std::string m_bgColorRole   = "panel";
    float       m_rounding      = 8.f;
    bool        m_scrollable    = true;
    float       m_scrollSpeed   = 60.f;

    // Dense by default: a file list is usually scanned, not read.
    float m_entryHeight     = 24.f;
    float m_entryPadding    = 0.f;
    float m_entrySpacing    = 2.f;
    float m_entryRounding   = 4.f;
    float m_indent          = 16.f;
    float m_textPaddingLeft = 8.f;

    std::string  m_fontPath;
    unsigned int m_charSize        = 16;
    Align        m_textAlignY      = Align::CenterY;
    bool         m_boldDirectories = false;

    bool  m_showIcons       = true;
    float m_iconSize        = 16.f;
    float m_iconSpacing     = 8.f;
    float m_iconStrokeWidth = 0.f;
    bool  m_hasIconStroke   = false;
    std::string m_dirIcon     = "folder";
    std::string m_dirOpenIcon;
    std::string m_fileIcon    = "file";
    bool        m_useKindIcons = true;
    // Indexed by FileKind. Feather has no dedicated binary/executable glyph, so
    // that maps to the generic box.
    std::string m_kindIcons[static_cast<std::size_t>(FileKind::Unknown) + 1] = {
        /* Image   */ "image",
        /* Code    */ "code",
        /* Doc     */ "file-text",
        /* Text    */ "file-text",
        /* Binary  */ "box",
        /* Audio   */ "music",
        /* Video   */ "film",
        /* Archive */ "archive",
        /* Unknown */ "file",
    };
    Color       m_iconColor     = Color::White;
    std::string m_iconColorRole;
    bool        m_hasIconColor  = false;

    // Entries are told apart by their icon, so directories and files share one
    // background and one text colour by default. Set the directory-specific
    // roles to bring the old two-tone look back.
    Color       m_entryColor        = Color{0, 0, 0, 0};
    std::string m_entryColorRole;
    Color       m_dirColor          = Color{0, 0, 0, 0};
    std::string m_dirColorRole;
    Color       m_hoverColor        = Color{255, 255, 255, 18};
    std::string m_hoverColorRole;
    // A subdued fill rather than the accent: selection should mark a row, not
    // shout over the icon and label.
    Color       m_selectedColor     = Color{255, 255, 255, 40};
    std::string m_selectedColorRole = "panelAlt";

    Color       m_fileTextColor         = Color::White;
    std::string m_fileTextColorRole     = "text";
    Color       m_dirTextColor          = Color::White;
    std::string m_dirTextColorRole      = "text";
    Color       m_selectedTextColor     = Color::White;
    std::string m_selectedTextColorRole = "text";

    std::string m_expandedPrefix;
    std::string m_collapsedPrefix;
    std::string m_filePrefix;
    std::string m_directorySuffix;
    bool        m_showExtensions  = true;

    bool         m_showHeader        = true;
    std::string  m_headerText;                    // empty = the root's own name
    float        m_headerHeight       = 28.f;
    float        m_headerPadding      = 0.f;
    float        m_headerRounding     = 0.f;
    unsigned int m_headerCharSize     = 16;
    bool         m_hasHeaderCharSize  = false;    // unset = follow setCharSize()
    bool         m_headerBold         = true;
    std::string  m_headerIcon         = "folder";
    Color        m_headerColor        = Color{0, 0, 0, 0};
    std::string  m_headerColorRole    = "panelAlt";
    Color        m_headerTextColor    = Color::White;
    std::string  m_headerTextColorRole = "text";
    bool m_showHidden       = false;
    bool m_showFiles        = true;
    bool m_showDirectories  = true;
    std::vector<std::string> m_extFilter;

    FileBrowserSort m_sortMode          = FileBrowserSort::Name;
    bool            m_sortDescending    = false;
    bool            m_directoriesFirst  = true;
    bool            m_sortCaseSensitive = false;

    bool m_selectable    = true;
    bool m_expandOnClick = true;

    PathCallback m_onEntrySelected;
    PathCallback m_onFileClicked;
    PathCallback m_onDirectoryClicked;
    PathCallback m_onDirExpanded;
    PathCallback m_onDirCollapsed;
};


/*
    FileBrowser
    - A scrollable directory listing built on top of Column. Each visible
      entry is a Row (background + hover/selection) holding an indent Spacer
      and a Text label.
    - Directories expand and collapse on click, one level at a time: the
      underlying FileTree only lists a directory the first time it is opened
      and caches it from then on, so deep trees stay cheap.
    - Expansion state is keyed by path rather than stored on the tree nodes,
      so refresh() can re-scan the disk without losing what the user opened.
*/
class FileBrowser : public Column {
public:
    explicit FileBrowser(Modifier modifier, FileBrowserOptions options = {}, const std::string& name = "");

    const FileBrowserOptions& getOptions() const { return m_fbOptions; }
    FileBrowserOptions&       getOptions()       { return m_fbOptions; }
    // Re-applies panel settings and rebuilds the entry rows.
    void setOptions(const FileBrowserOptions& opts);

    void update(Rectf& parentBounds, float dt) override;

    // Points the browser at a new root. Clears expansion and selection.
    void setRootPath(const fs::path& path);
    fs::path getRootPath() const;

    // Re-scans every directory currently open, keeping expansion state.
    void refresh();

    bool isExpanded(const fs::path& path) const;
    void expandPath(const fs::path& path);
    void collapsePath(const fs::path& path);
    void togglePath(const fs::path& path);
    void collapseAll();

    const fs::path& getSelectedPath() const { return m_selectedPath; }
    void setSelectedPath(const fs::path& path);
    void clearSelection();

    // Entry rows are rebuilt at the top of the next update() rather than
    // immediately: clicks are dispatched while the parent walks its child
    // list, so mutating that list from a click handler would invalidate the
    // iteration in progress.
    void markForRebuild() { m_needsRebuild = true; }

    FileTree* getTree() { return m_tree ? &*m_tree : nullptr; }

private:
    void applyPanelOptions();
    void rebuildRows();
    void clearRows();
    void appendDirectory(Directory* dir, int depth);
    Element* buildRow(FSEntry* entry, int depth);
    // The pinned title strip naming the root directory.
    Element* buildHeader();
    // Display name for the root: its own directory name, resolved through an
    // absolute path so a relative root like "../" reads as the folder it
    // actually points at instead of "..".
    std::string rootDisplayName() const;
    // Appends a row, preceded by the entry-spacing gap when one is configured
    // and this is not the first row.
    void addEntryRow(Element* row);
    // Which registry icon represents this entry: folder (open or closed) for a
    // directory, otherwise the FileKind mapping or the plain file icon.
    const std::string& iconNameFor(const FSEntry* entry) const;
    bool passesFilter(const FSEntry* entry) const;
    void sortEntries(std::vector<FSEntry*>& entries) const;
    void handleEntryClicked(const fs::path& path, bool isDirectory);

    FileBrowserOptions          m_fbOptions;
    std::optional<FileTree>     m_tree;
    std::unordered_set<std::string> m_expanded;
    fs::path                    m_selectedPath;
    bool                        m_needsRebuild = false;
};

}
