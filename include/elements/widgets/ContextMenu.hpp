#pragma once

#include "../containers/Column.hpp"
#include "../decoration/Icon.hpp"
#include "../decoration/Spacer.hpp"
#include "../decoration/Text.hpp"
#include "../interactible/Button.hpp"
#include "../../utils/ContextMenuItem.hpp"

#include <optional>
#include <string>
#include <vector>

namespace uilo {

/*
    ContextMenuOptions:
    - Desc:     How a context menu is drawn. Every colour defaults to a palette
                role, so a menu follows the application's theme with nothing
                named at the call site.
*/
class ContextMenuOptions {
public:
    ContextMenuOptions& setColor(const Color& c)                   { m_color = c; return *this; }
    ContextMenuOptions& setColorRole(const std::string& r)         { m_colorRole = r; return *this; }
    ContextMenuOptions& setHoverColor(const Color& c)              { m_hoverColor = c; return *this; }
    ContextMenuOptions& setHoverColorRole(const std::string& r)    { m_hoverColorRole = r; return *this; }
    ContextMenuOptions& setTextColor(const Color& c)               { m_textColor = c; return *this; }
    ContextMenuOptions& setTextColorRole(const std::string& r)     { m_textColorRole = r; return *this; }
    ContextMenuOptions& setDisabledColor(const Color& c)           { m_disabledColor = c; return *this; }
    ContextMenuOptions& setDisabledColorRole(const std::string& r) { m_disabledColorRole = r; return *this; }
    ContextMenuOptions& setSeparatorColor(const Color& c)          { m_sepColor = c; return *this; }
    ContextMenuOptions& setSeparatorColorRole(const std::string& r){ m_sepColorRole = r; return *this; }
    ContextMenuOptions& setOutlineColor(const Color& c)            { m_outlineColor = c; return *this; }
    ContextMenuOptions& setOutlineColorRole(const std::string& r)  { m_outlineColorRole = r; return *this; }
    ContextMenuOptions& setOutlineThickness(float px)              { m_outlineThickness = px; return *this; }

    ContextMenuOptions& setRounding(float r)          { m_rounding = r; return *this; }
    ContextMenuOptions& setItemHeight(float px)       { m_itemHeight = px; return *this; }
    ContextMenuOptions& setItemPadding(float px)      { m_itemPadding = px; return *this; }
    ContextMenuOptions& setVerticalPadding(float px)  { m_verticalPadding = px; return *this; }
    ContextMenuOptions& setSeparatorThickness(float px) { m_sepThickness = px; return *this; }
    ContextMenuOptions& setSeparatorSpacing(float px)   { m_sepSpacing = px; return *this; }
    ContextMenuOptions& setIconSize(float px)         { m_iconSize = px; return *this; }
    ContextMenuOptions& setIconSpacing(float px)      { m_iconSpacing = px; return *this; }
    ContextMenuOptions& setCharSize(unsigned px)      { m_charSize = px; return *this; }
    ContextMenuOptions& setFont(const std::string& p) { m_fontPath = p; return *this; }
    ContextMenuOptions& setMinWidth(float px)         { m_minWidth = px; return *this; }
    ContextMenuOptions& setMaxWidth(float px)         { m_maxWidth = px; return *this; }
    // Glyph on a row that opens a submenu.
    ContextMenuOptions& setSubmenuIcon(const std::string& p) { m_submenuIcon = p; return *this; }
    // How far a submenu overlaps the row that opened it, so travelling to it
    // does not cross a gap and dismiss it.
    ContextMenuOptions& setSubmenuOverlap(float px)   { m_submenuOverlap = px; return *this; }

    Color              getColor()             const { return m_color; }
    const std::string& getColorRole()         const { return m_colorRole; }
    Color              getHoverColor()        const { return m_hoverColor; }
    const std::string& getHoverColorRole()    const { return m_hoverColorRole; }
    Color              getTextColor()         const { return m_textColor; }
    const std::string& getTextColorRole()     const { return m_textColorRole; }
    Color              getDisabledColor()     const { return m_disabledColor; }
    const std::string& getDisabledColorRole() const { return m_disabledColorRole; }
    Color              getSeparatorColor()    const { return m_sepColor; }
    const std::string& getSeparatorColorRole() const { return m_sepColorRole; }
    Color              getOutlineColor()      const { return m_outlineColor; }
    const std::string& getOutlineColorRole()  const { return m_outlineColorRole; }
    float              getOutlineThickness()  const { return m_outlineThickness; }

    float    getRounding()           const { return m_rounding; }
    float    getItemHeight()         const { return m_itemHeight; }
    float    getItemPadding()        const { return m_itemPadding; }
    float    getVerticalPadding()    const { return m_verticalPadding; }
    float    getSeparatorThickness() const { return m_sepThickness; }
    float    getSeparatorSpacing()   const { return m_sepSpacing; }
    float    getIconSize()           const { return m_iconSize; }
    float    getIconSpacing()        const { return m_iconSpacing; }
    unsigned getCharSize()           const { return m_charSize; }
    const std::string& getFont()     const { return m_fontPath; }
    float    getMinWidth()           const { return m_minWidth; }
    float    getMaxWidth()           const { return m_maxWidth; }
    const std::string& getSubmenuIcon() const { return m_submenuIcon; }
    float    getSubmenuOverlap()     const { return m_submenuOverlap; }

private:
    Color       m_color             = Color{0, 0, 0, 0};
    std::string m_colorRole         = "panel";
    Color       m_hoverColor        = Color{0, 0, 0, 0};
    std::string m_hoverColorRole    = "panelAlt";
    Color       m_textColor         = Color::White;
    std::string m_textColorRole     = "text";
    Color       m_disabledColor     = Color{0, 0, 0, 0};
    std::string m_disabledColorRole = "textDim";
    Color       m_sepColor          = Color{0, 0, 0, 0};
    std::string m_sepColorRole      = "outline";
    Color       m_outlineColor      = Color{0, 0, 0, 0};
    std::string m_outlineColorRole  = "outline";
    float       m_outlineThickness  = 1.f;

    float       m_rounding       = 6.f;
    float       m_itemHeight     = 28.f;
    float       m_itemPadding    = 10.f;
    float       m_verticalPadding = 6.f;
    float       m_sepThickness   = 1.f;
    float       m_sepSpacing     = 4.f;
    float       m_iconSize       = 16.f;
    float       m_iconSpacing    = 8.f;
    unsigned    m_charSize       = 14;
    std::string m_fontPath;
    float       m_minWidth       = 140.f;
    float       m_maxWidth       = 420.f;
    std::string m_submenuIcon    = "chevron-right";
    float       m_submenuOverlap = 4.f;
};


/*
    ContextMenu:
    - Desc:     The popup itself: a Column of rows, sized to its widest label and
                placed at the cursor. UILO owns one at the root of the UI and
                fills it from whichever element was right-clicked, so an
                application never builds or positions one -- it declares items
                with Modifier::setContextMenu and this is what shows them.
    - Rows are pooled. setItems() relabels and shows the slots it needs and hides
      the rest, and only ever grows the pool, because the web bridge translates
      structure once per session and would not see a rebuild. Each slot holds
      both an item row and a separator rule, and shows whichever the item at that
      position is.
    - A submenu is another ContextMenu, created on demand and owned by the parent
      one. It is registered as its own overlay so it is hit-tested above its
      parent, and closed whenever the highlight moves off the row that opened it.
*/
class ContextMenu : public Column {
public:
    ContextMenu(
        Modifier modifier = {},
        ContextMenuOptions options = {},
        const std::string& name = ""
    );

    void update(Rectf& parentBounds, float dt) override;

    // Fills the menu from `items`, growing the row pool if it is short. Safe to
    // call every time the menu opens; nothing is destroyed.
    void setItems(const std::vector<ContextMenuItem>& items);

    // Opens at `cursor`, flipped left and/or up when the menu would otherwise
    // run past the window edge. Registers itself as an overlay, so clicking
    // outside dismisses it.
    void openAt(const Vec2f& cursor);
    void close();
    bool isOpen() const { return m_open; }

    // Keyboard navigation. Each returns true when it consumed the key, so the
    // caller can leave application shortcuts alone while a menu is up.
    bool moveHighlight(int delta);
    bool activateHighlighted();
    bool openHighlightedSubmenu();
    bool closeSubmenu();

    // The size the current items need, which is what openAt() places.
    Vec2f measure() const;

    const ContextMenuOptions& getOptions() const { return m_cmOptions; }
    ContextMenuOptions&       getOptions()       { return m_cmOptions; }

    // The deepest submenu currently open, or this menu when none is.
    ContextMenu* deepestOpen();
    // The submenu this menu has open right now, or null. Walk it to visit every
    // level of an open chain.
    ContextMenu* openSubmenu();

    // Closes this menu and every menu above it, which is what picking an item
    // does: a submenu cannot outlive the menu that opened it.
    void closeChain();

protected:
    // A menu is a surface: it takes every pointer event that lands on it rather
    // than letting one through to the UI it is covering.
    bool claimsPointerEvents() const override { return true; }

private:
    /*
        Slot:
        - Desc: One position in the menu, holding both shapes a position can
                take. Exactly one of `row` and `rule` is visible once the slot is
                in use, and both are hidden when it is not.
    */
    struct Slot {
        Button* row       = nullptr;   /* the item */
        Spacer* rule      = nullptr;   /* the separator */
        Spacer* padLeft   = nullptr;
        Icon*   icon      = nullptr;
        Spacer* iconGap   = nullptr;
        Text*   label     = nullptr;
        Icon*   arrow     = nullptr;
        Spacer* padRight  = nullptr;
    };

    // Places and shows the menu. It would like its left edge at `preferredLeft`;
    // when that does not fit the window, its RIGHT edge goes at `flipRightEdge`
    // instead. Two separate anchors are the whole point: a menu opened at the
    // cursor flips about the cursor, while a submenu has to flip to the far side
    // of its parent rather than back over the top of it.
    // `flipVertically` flips about `top` the same way; a submenu instead shifts
    // up just enough to fit, so it stays level with the row that opened it.
    void  show(float preferredLeft, float flipRightEdge, float top, bool flipVertically);

    void  growTo(size_t count);
    Slot  makeSlot();
    void  applySlot(Slot& slot, const ContextMenuItem& item, bool showIconColumn);
    void  hideSlot(Slot& slot);
    void  highlight(int index);
    void  paintRow(size_t i, bool hovered);
    float rowWidthFor(const ContextMenuItem& item, bool showIconColumn) const;
    ContextMenu* submenuFor(size_t index);
    void         applySurface();

    ContextMenuOptions m_cmOptions;

    std::vector<Slot>            m_slots;
    std::vector<ContextMenuItem> m_items;      /* what is currently shown */
    bool                         m_open = false;
    int                          m_highlight = -1;
    Vec2f                        m_origin;     /* where openAt() placed us */

    /* One submenu per row that needs one, kept for the life of the menu for the
       same reason the rows are. */
    std::vector<ContextMenu*> m_submenus;
    int                       m_openSubmenu = -1;
    ContextMenu*              m_parent      = nullptr;
};

} // namespace uilo
