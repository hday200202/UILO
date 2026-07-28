#pragma once

#include <functional>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

#include "../containers/Column.hpp"
#include "../containers/Row.hpp"
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
    FileBrowserOptions& setEntryHeight(float px)        { m_entryHeight = px; return *this; }
    FileBrowserOptions& setEntryPadding(float px)       { m_entryPadding = px; return *this; }
    FileBrowserOptions& setEntryRounding(float r)       { m_entryRounding = r; return *this; }
    FileBrowserOptions& setIndent(float px)             { m_indent = px; return *this; }
    FileBrowserOptions& setTextPaddingLeft(float px)    { m_textPaddingLeft = px; return *this; }

    // Entry text --------------------------------------------------------
    FileBrowserOptions& setFont(const std::string& path){ m_fontPath = path; return *this; }
    FileBrowserOptions& setCharSize(unsigned int n)     { m_charSize = n; return *this; }
    FileBrowserOptions& setTextAlignY(Align a)          { m_textAlignY = a; return *this; }
    FileBrowserOptions& setBoldDirectories(bool v)      { m_boldDirectories = v; return *this; }

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

    // Contents ----------------------------------------------------------
    FileBrowserOptions& setShowRoot(bool v)         { m_showRoot = v; return *this; }
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
    float getEntryRounding()    const { return m_entryRounding; }
    float getIndent()           const { return m_indent; }
    float getTextPaddingLeft()  const { return m_textPaddingLeft; }

    const std::string& getFontPath()    const { return m_fontPath; }
    unsigned int getCharSize()          const { return m_charSize; }
    Align getTextAlignY()               const { return m_textAlignY; }
    bool  getBoldDirectories()          const { return m_boldDirectories; }

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

    bool getShowRoot()          const { return m_showRoot; }
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

    float m_entryHeight     = 36.f;
    float m_entryPadding    = 4.f;
    float m_entryRounding   = 4.f;
    float m_indent          = 16.f;
    float m_textPaddingLeft = 8.f;

    std::string  m_fontPath;
    unsigned int m_charSize        = 16;
    Align        m_textAlignY      = Align::CenterY;
    bool         m_boldDirectories = false;

    Color       m_entryColor        = Color{0, 0, 0, 0};
    std::string m_entryColorRole;
    Color       m_dirColor          = Color{0, 0, 0, 0};
    std::string m_dirColorRole      = "panelAlt";
    Color       m_hoverColor        = Color{255, 255, 255, 18};
    std::string m_hoverColorRole;
    Color       m_selectedColor     = Color{255, 255, 255, 40};
    std::string m_selectedColorRole = "accent";

    Color       m_fileTextColor         = Color::White;
    std::string m_fileTextColorRole     = "textDim";
    Color       m_dirTextColor          = Color::White;
    std::string m_dirTextColorRole      = "text";
    Color       m_selectedTextColor     = Color::White;
    std::string m_selectedTextColorRole = "onAccent";

    std::string m_expandedPrefix;
    std::string m_collapsedPrefix;
    std::string m_filePrefix;
    std::string m_directorySuffix;
    bool        m_showExtensions  = true;

    bool m_showRoot         = true;
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
