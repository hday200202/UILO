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
#include "../../utils/Theme.hpp"

namespace uilo {

/*
    FileBrowserSort:
    - Desc:     Ordering applied to the entries of each directory. Size and
                Modified stat each file the first time a directory is listed;
                Name and Extension are free, since they only read the cached
                path.
*/
enum class FileBrowserSort { Name, Extension, Size, Modified };


/*
    FileBrowserOptions:
    - Desc:     Everything the widget draws is configurable here. Colors come in
                literal plus role pairs like the rest of the library: the role
                wins when it resolves against the active Palette, otherwise the
                literal is used. Sizes are unscaled content pixels; UILO
                multiplies by its scale at layout time.
*/
class FileBrowserOptions {
public:
    FileBrowserOptions() = default;

    // Space kept outside this element, inside the slot its parent gave it. It
    // shrinks the element rather than displacing a sibling. Unset follows
    // Theme::setOuterPadding().
    FileBrowserOptions& setOuterPadding(float px)   { m_outerPadding = px; return *this; }
    FileBrowserOptions& clearOuterPadding()         { m_outerPadding.reset(); return *this; }
    float  getOuterPadding()     const { return Theme::resolveOuterPadding(m_outerPadding, 0.f); }

    // Space kept between this element's edge and the area its entries are laid out in. Unset follows
    // Theme::setInnerPadding().
    FileBrowserOptions& setInnerPadding(float px)   { m_innerPadding = px; return *this; }
    FileBrowserOptions& clearInnerPadding()         { m_innerPadding.reset(); return *this; }
    float  getInnerPadding()     const { return Theme::resolveInnerPadding(m_innerPadding, 0.f); }

    // Source
    FileBrowserOptions& setRootPath(const std::string& p)   { m_rootPath = p; return *this; }

    // Panel: the scrollable surface behind the entries.
    FileBrowserOptions& setBackgroundColor(const Color& c)          { m_bgColor = c; return *this; }
    FileBrowserOptions& setBackgroundColorRole(const std::string& r){ m_bgColorRole = r; return *this; }
    FileBrowserOptions& setRounding(float r)                        { m_rounding = r; return *this; }
    FileBrowserOptions& setScrollable(bool v)                       { m_scrollable = v; return *this; }
    FileBrowserOptions& setScrollSpeed(float s)                     { m_scrollSpeed = s; return *this; }

    // Outline: a border around the panel, drawn inside its bounds.
    FileBrowserOptions& setOutlineColor(const Color& c)           { m_outlineColor = c; return *this; }
    FileBrowserOptions& setOutlineColorRole(const std::string& r) { m_outlineColorRole = r; return *this; }
    FileBrowserOptions& setOutlineThickness(float px)             { m_outlineThickness = px; return *this; }

    // Entry metrics. setEntryHeight is THE control for how tightly entries pack:
    // one entry occupies exactly this much vertical space, so the distance
    // between two labels is this value and nothing else. Lower it to tighten the
    // list (36 -> 24 is noticeably denser), and keep it above setIconSize() or
    // the icon overflows its row.
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

    // Entry text
    FileBrowserOptions& setFont(std::string_view path){ m_fontPath = std::string(path); return *this; }
    FileBrowserOptions& setCharSize(unsigned int n)     { m_charSize = n; return *this; }
    FileBrowserOptions& setTextAlignY(Align a)          { m_textAlignY = a; return *this; }
    FileBrowserOptions& setBoldDirectories(bool v)      { m_boldDirectories = v; return *this; }

    // Entry icons: an icon ahead of each name is what separates a folder from a
    // file, so neither has to be tinted differently to be recognisable.
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
    FileBrowserOptions& setIconForKind(FileKind kind, std::string_view n);
    // Icons follow the entry's text colour unless given one of their own.
    FileBrowserOptions& setIconColor(const Color& c)           { m_iconColor = c; m_hasIconColor = true; return *this; }
    FileBrowserOptions& setIconColorRole(const std::string& r) { m_iconColorRole = r; m_hasIconColor = true; return *this; }

    // Directory arrow: the marker at the right end of a directory row saying
    // whether it is open, the way a Dropdown marks its header. It sits at the
    // right edge however wide the row is, and a file reserves the same width
    // rather than drawing one, so every label in the list clips at the same
    // place. Native only, like the Dropdown arrow: on the web the entry rows are
    // built once and expanding a directory does not restructure the DOM.
    FileBrowserOptions& setShowDirectoryArrow(bool v)             { m_showDirArrow = v; return *this; }
    FileBrowserOptions& setDirectoryArrowIcon(std::string_view n)     { m_dirArrowIcon = std::string(n); return *this; }
    // Shown while the directory is expanded. Set both to the same name for an
    // arrow that does not change on opening.
    FileBrowserOptions& setDirectoryArrowOpenIcon(std::string_view n) { m_dirArrowOpenIcon = std::string(n); return *this; }
    FileBrowserOptions& setDirectoryArrowSize(float px)           { m_dirArrowSize = px; return *this; }
    // Gap between the label and the arrow, and the inset from the right edge.
    FileBrowserOptions& setDirectoryArrowSpacing(float px)        { m_dirArrowSpacing = px; return *this; }
    FileBrowserOptions& setDirectoryArrowPadding(float px)        { m_dirArrowPadding = px; return *this; }
    // Falls back to setIconStrokeWidth() when unset.
    FileBrowserOptions& setDirectoryArrowStrokeWidth(float w)     { m_dirArrowStroke = w; m_hasDirArrowStroke = true; return *this; }
    // Untinted, the arrow follows the entry's text colour.
    FileBrowserOptions& setDirectoryArrowColor(const Color& c)           { m_dirArrowColor = c; m_hasDirArrowColor = true; return *this; }
    FileBrowserOptions& setDirectoryArrowColorRole(const std::string& r) { m_dirArrowColorRole = r; m_hasDirArrowColor = true; return *this; }

    // Entry backgrounds
    FileBrowserOptions& setEntryBackgroundColor(const Color& c)           { m_entryColor = c; return *this; }
    FileBrowserOptions& setEntryBackgroundColorRole(const std::string& r) { m_entryColorRole = r; return *this; }
    FileBrowserOptions& setDirectoryBackgroundColor(const Color& c)           { m_dirColor = c; return *this; }
    FileBrowserOptions& setDirectoryBackgroundColorRole(const std::string& r) { m_dirColorRole = r; return *this; }
    FileBrowserOptions& setHoverColor(const Color& c)            { m_hoverColor = c; return *this; }
    FileBrowserOptions& setHoverColorRole(const std::string& r)  { m_hoverColorRole = r; return *this; }
    FileBrowserOptions& setSelectedColor(const Color& c)           { m_selectedColor = c; return *this; }
    FileBrowserOptions& setSelectedColorRole(const std::string& r) { m_selectedColorRole = r; return *this; }

    // Entry text colors
    FileBrowserOptions& setFileTextColor(const Color& c)            { m_fileTextColor = c; return *this; }
    FileBrowserOptions& setFileTextColorRole(const std::string& r)  { m_fileTextColorRole = r; return *this; }
    FileBrowserOptions& setDirectoryTextColor(const Color& c)           { m_dirTextColor = c; return *this; }
    FileBrowserOptions& setDirectoryTextColorRole(const std::string& r) { m_dirTextColorRole = r; return *this; }
    FileBrowserOptions& setSelectedTextColor(const Color& c)           { m_selectedTextColor = c; return *this; }
    FileBrowserOptions& setSelectedTextColorRole(const std::string& r) { m_selectedTextColorRole = r; return *this; }

    // Labels. Prefixes are drawn ahead of the name; defaults are empty so the
    // widget never depends on a glyph the configured font might not carry.
    FileBrowserOptions& setExpandedPrefix(const std::string& s)  { m_expandedPrefix = s; return *this; }
    FileBrowserOptions& setCollapsedPrefix(const std::string& s) { m_collapsedPrefix = s; return *this; }
    FileBrowserOptions& setFilePrefix(const std::string& s)      { m_filePrefix = s; return *this; }
    FileBrowserOptions& setDirectorySuffix(const std::string& s) { m_directorySuffix = s; return *this; }
    FileBrowserOptions& setShowExtensions(bool v)                { m_showExtensions = v; return *this; }

    // Header. The root is a title strip pinned to the top of the panel, not a
    // row in the list: it is where you already are, so there is nothing to
    // expand or collapse. The list below it is the root's children.
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

    // Contents
    FileBrowserOptions& setShowHidden(bool v)       { m_showHidden = v; return *this; }
    FileBrowserOptions& setShowFiles(bool v)        { m_showFiles = v; return *this; }
    FileBrowserOptions& setShowDirectories(bool v)  { m_showDirectories = v; return *this; }
    // Only files whose extension appears here are listed. Entries may be given
    // with or without the leading dot; matching is case-insensitive. An empty
    // filter (the default) lists everything.
    FileBrowserOptions& setExtensionFilter(const std::vector<std::string>& exts) { m_extFilter = exts; return *this; }

    // Ordering
    FileBrowserOptions& setSortMode(FileBrowserSort m)  { m_sortMode = m; return *this; }
    FileBrowserOptions& setSortDescending(bool v)       { m_sortDescending = v; return *this; }
    FileBrowserOptions& setDirectoriesFirst(bool v)     { m_directoriesFirst = v; return *this; }
    FileBrowserOptions& setSortCaseSensitive(bool v)    { m_sortCaseSensitive = v; return *this; }

    // Behavior
    FileBrowserOptions& setSelectable(bool v)               { m_selectable = v; return *this; }
    FileBrowserOptions& setExpandDirectoryOnClick(bool v)   { m_expandOnClick = v; return *this; }

    // Callbacks
    using PathCallback = std::function<void(const fs::path&)>;
    FileBrowserOptions& setOnEntrySelected(PathCallback cb)      { m_onEntrySelected = std::move(cb); return *this; }
    FileBrowserOptions& setOnFileClicked(PathCallback cb)        { m_onFileClicked = std::move(cb); return *this; }
    FileBrowserOptions& setOnDirectoryClicked(PathCallback cb)   { m_onDirectoryClicked = std::move(cb); return *this; }
    FileBrowserOptions& setOnDirectoryExpanded(PathCallback cb)  { m_onDirExpanded = std::move(cb); return *this; }
    FileBrowserOptions& setOnDirectoryCollapsed(PathCallback cb) { m_onDirCollapsed = std::move(cb); return *this; }

    const std::string& getRootPath()        const { return m_rootPath; }

    Color              getBackgroundColor()     const { return m_bgColor; }
    const std::string& getBackgroundColorRole() const { return m_bgColorRole; }
    float              getRounding()            const;
    bool               getScrollable()          const { return m_scrollable; }
    float              getScrollSpeed()         const { return m_scrollSpeed; }
    Color              getOutlineColor()     const { return m_outlineColor; }
    const std::string& getOutlineColorRole() const { return m_outlineColorRole; }
    float              getOutlineThickness() const { return m_outlineThickness; }

    float getEntryHeight()      const { return m_entryHeight; }
    float getEntryPadding()     const { return m_entryPadding; }
    float getEntrySpacing()     const { return m_entrySpacing; }
    float getEntryRounding()    const;
    float getIndent()           const { return m_indent; }
    float getTextPaddingLeft()  const { return m_textPaddingLeft; }

    const std::string& getFontPath()    const;
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
    const std::string& getIconForKind(FileKind kind) const;
    Color              getIconColor()     const { return m_iconColor; }
    const std::string& getIconColorRole() const { return m_iconColorRole; }
    bool               hasIconColor()     const { return m_hasIconColor; }

    bool  getShowDirectoryArrow()      const { return m_showDirArrow; }
    const std::string& getDirectoryArrowIcon()     const { return m_dirArrowIcon; }
    const std::string& getDirectoryArrowOpenIcon() const { return m_dirArrowOpenIcon; }
    float getDirectoryArrowSize()      const { return m_dirArrowSize; }
    float getDirectoryArrowSpacing()   const { return m_dirArrowSpacing; }
    float getDirectoryArrowPadding()   const { return m_dirArrowPadding; }
    float getDirectoryArrowStrokeWidth()  const { return m_dirArrowStroke; }
    bool  hasDirectoryArrowStrokeWidth() const { return m_hasDirArrowStroke; }
    Color              getDirectoryArrowColor()     const { return m_dirArrowColor; }
    const std::string& getDirectoryArrowColorRole() const { return m_dirArrowColorRole; }
    bool               hasDirectoryArrowColor()     const { return m_hasDirArrowColor; }
    float getDirectoryArrowBlockWidth() const;

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
    float getHeaderRounding()   const;
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

    // Raw rounding, for handing to a child element that should keep following
    // the theme rather than being pinned to a resolved number.
    const std::optional<float>& getRoundingOpt() const { return m_rounding; }
    static constexpr float getRoundingOptFallback() { return 8.f; }
    const std::optional<float>& getEntryRoundingOpt() const { return m_entryRounding; }
    static constexpr float getEntryRoundingOptFallback() { return 4.f; }
    const std::optional<float>& getHeaderRoundingOpt() const { return m_headerRounding; }
    static constexpr float getHeaderRoundingOptFallback() { return 0.f; }

private:
    std::optional<float> m_outerPadding;
    std::optional<float> m_innerPadding;
    std::string m_rootPath;

    Color       m_bgColor       = Color{0, 0, 0, 0};
    std::string m_bgColorRole   = "panel";
    std::optional<float>       m_rounding;
    bool        m_scrollable    = true;
    float       m_scrollSpeed   = 60.f;
    Color                m_outlineColor = Color::Transparent;
    std::string          m_outlineColorRole;
    float m_outlineThickness = 0.f;

    // Dense by default: a file list is usually scanned, not read.
    float m_entryHeight     = 24.f;
    float m_entryPadding    = 0.f;
    float m_entrySpacing    = 2.f;
    std::optional<float> m_entryRounding;
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

    // A touch smaller than the entry icon: the arrow is a hint about state, not
    // a second thing to read.
    bool        m_showDirArrow      = true;
    std::string m_dirArrowIcon      = "chevron-right";
    std::string m_dirArrowOpenIcon  = "chevron-down";
    float       m_dirArrowSize      = 14.f;
    float       m_dirArrowSpacing   = 6.f;
    float       m_dirArrowPadding   = 8.f;
    float       m_dirArrowStroke    = 0.f;
    bool        m_hasDirArrowStroke = false;
    Color       m_dirArrowColor     = Color::White;
    std::string m_dirArrowColorRole;
    bool        m_hasDirArrowColor  = false;

    // Entries are told apart by their icon, so directories and files share one
    // background and one text colour by default. Set the directory-specific
    // roles to bring the old two-tone look back.
    Color       m_entryColor        = Color{0, 0, 0, 0};
    std::string m_entryColorRole;
    Color       m_dirColor          = Color{0, 0, 0, 0};
    std::string m_dirColorRole;
    Color       m_hoverColor        = Color{255, 255, 255, 18};
    std::string m_hoverColorRole = "panelAlt";
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
    std::string  m_headerText;   /* empty = the root's own name */
    float        m_headerHeight       = 28.f;
    float        m_headerPadding      = 0.f;
    std::optional<float>        m_headerRounding;
    unsigned int m_headerCharSize     = 16;
    bool         m_hasHeaderCharSize  = false;   /* unset = follow setCharSize() */
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
    setIconForKind(FileKind kind, std::string_view n):
    - Params:   FileKind kind, std::string_view n
    - Returns:  FileBrowserOptions&
    - Desc:     Overrides the icon used for one FileKind, so a project can give
                its own file types a recognisable glyph without turning the
                whole kind mapping off.
*/
inline FileBrowserOptions& FileBrowserOptions::setIconForKind(
    FileKind kind,
    std::string_view n
) {
    m_kindIcons[static_cast<std::size_t>(kind)] = std::string(n);
    return *this;
}


/*
    getIconForKind(FileKind kind):
    - Params:   FileKind kind
    - Returns:  const std::string&
    - Desc:     The icon name registered for a FileKind. Empty when that kind
                was deliberately cleared, which sends the caller back to
                setFileIcon().
*/
inline const std::string& FileBrowserOptions::getIconForKind(FileKind kind) const {
    return m_kindIcons[static_cast<std::size_t>(kind)];
}


/*
    getRounding():
    - Params:   none
    - Returns:  float
    - Desc:     Corner radius of the panel, resolved in three steps: the value
                this widget was given, then the active Theme's, then 8. Resolved
                on every read rather than cached, so changing the Theme restyles
                a browser already on screen.
*/
inline float FileBrowserOptions::getRounding() const {
    return Theme::resolveRounding(m_rounding, getRoundingOptFallback());
}


/*
    getEntryRounding():
    - Params:   none
    - Returns:  float
    - Desc:     Corner radius of one entry row, resolved the same three ways as
                getRounding with a fallback of 4.
*/
inline float FileBrowserOptions::getEntryRounding() const {
    return Theme::resolveRounding(m_entryRounding, getEntryRoundingOptFallback());
}


/*
    getHeaderRounding():
    - Params:   none
    - Returns:  float
    - Desc:     Corner radius of the pinned header strip, resolved the same
                three ways as getRounding with a fallback of 0 -- square, since
                the header spans the full width of the panel.
*/
inline float FileBrowserOptions::getHeaderRounding() const {
    return Theme::resolveRounding(m_headerRounding, getHeaderRoundingOptFallback());
}


/*
    getFontPath():
    - Params:   none
    - Returns:  const std::string&
    - Desc:     Font for the entry labels, falling back to the active Theme's
                when this widget was not given one.
*/
inline const std::string& FileBrowserOptions::getFontPath() const {
    return Theme::resolveFont(m_fontPath);
}


/*
    getDirectoryArrowBlockWidth():
    - Params:   none
    - Returns:  float
    - Desc:     Total width the arrow occupies at the right end of a row: the
                gap ahead of it, the arrow itself, and the inset behind it. A
                file row reserves exactly this much so its label clips where a
                directory's does.
*/
inline float FileBrowserOptions::getDirectoryArrowBlockWidth() const {
    return m_dirArrowSpacing + m_dirArrowSize + m_dirArrowPadding;
}


/*
    FileBrowser:
    - Desc:     A scrollable directory listing built on top of Column. Each
                visible entry is a Row -- background plus hover and selection
                state -- holding an indent Spacer, an icon, a Text label, and
                for a directory an arrow pinned to the right edge. Directories
                expand and collapse on click, one level at a time: the
                underlying FileTree only lists a directory the first time it is
                opened and caches it from then on, so deep trees stay cheap.
                Expansion state is keyed by path rather than stored on the tree
                nodes, so refresh() can re-scan the disk without losing what the
                user opened.
*/
class FileBrowser : public Column {
public:
    float getOuterPadding() const override { return m_fbOptions.getOuterPadding(); }
    float getInnerPadding() const override { return m_fbOptions.getInnerPadding(); }

    explicit FileBrowser(
        Modifier modifier,
        FileBrowserOptions options = {},
        const std::string& name = ""
    );

    const FileBrowserOptions& getOptions() const { return m_fbOptions; }
    FileBrowserOptions&       getOptions()       { return m_fbOptions; }
    void setOptions(const FileBrowserOptions& opts);

    void update(Rectf& parentBounds, float dt) override;

    void     setRootPath(const fs::path& path);
    fs::path getRootPath() const;

    void refresh();

    bool isExpanded(const fs::path& path) const;
    void expandPath(const fs::path& path);
    void collapsePath(const fs::path& path);
    void togglePath(const fs::path& path);
    void collapseAll();

    const fs::path& getSelectedPath() const { return m_selectedPath; }
    void setSelectedPath(const fs::path& path);
    void clearSelection();

    void markForRebuild() { m_needsRebuild = true; }

    FileTree* getTree() { return m_tree ? &*m_tree : nullptr; }

private:
    void     applyPanelOptions();
    void     rebuildRows();
    void     clearRows();
    void     appendDirectory(Directory* dir, int depth);
    Element* buildRow(FSEntry* entry, int depth);
    Element* buildHeader();

    void appendEntryIcon(
        std::vector<Element*>& children,
        const FSEntry* entry,
        const Color& textColor,
        const std::string& textRole,
        bool selected
    ) const;
    void appendDirectoryArrow(
        std::vector<Element*>& children,
        const FSEntry* entry,
        const Color& textColor,
        const std::string& textRole,
        bool selected
    ) const;

    std::string        rootDisplayName() const;
    void               addEntryRow(Element* row);
    const std::string& iconNameFor(const FSEntry* entry) const;
    bool               passesFilter(const FSEntry* entry) const;
    void               sortEntries(std::vector<FSEntry*>& entries) const;
    void               handleEntryClicked(const fs::path& path, bool isDirectory);

    FileBrowserOptions              m_fbOptions;
    std::optional<FileTree>         m_tree;
    std::unordered_set<std::string> m_expanded;
    fs::path                        m_selectedPath;
    bool                            m_needsRebuild = false;
};

}
