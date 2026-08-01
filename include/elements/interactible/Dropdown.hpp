#pragma once

#include <functional>
#include <vector>
#include <string>
#include <optional>

#include "../Element.hpp"
#include "../decoration/Text.hpp"
#include "../decoration/Spacer.hpp"
#include "Button.hpp"
#include "../containers/Column.hpp"
#include "../decoration/Icon.hpp"
#include "../../utils/Theme.hpp"

namespace uilo {

/*
    DropdownOptions:
    - Desc: Everything a Dropdown draws, in three groups: the header the user
            clicks, the popup that opens under it, and the items inside that
            popup. Colors come as a literal plus a role, where the role wins when
            it resolves against the active Palette and the literal is the
            fallback.
    - setMaxItems caps how tall the popup grows; beyond that it scrolls rather
      than running off the window.
*/
class DropdownOptions {
public:
    DropdownOptions() = default;

    DropdownOptions& setHeaderColor(Color c)      { m_headerColor = c;    return *this; }
    DropdownOptions& setHeaderColorRole(const std::string& r) { m_headerColorRole = r; return *this; }
    DropdownOptions& setHeaderRounding(float r)       { m_headerRounding = r; return *this; }
    DropdownOptions& setPopupColor(Color c)       { m_popupColor = c;     return *this; }
    DropdownOptions& setPopupColorRole(const std::string& r) { m_popupColorRole = r; return *this; }
    DropdownOptions& setItemColor(Color c)        { m_itemColor = c;      return *this; }
    DropdownOptions& setItemColorRole(const std::string& r) { m_itemColorRole = r; return *this; }
    DropdownOptions& setItemHoverColor(Color c)   { m_itemHoverColor = c; return *this; }
    DropdownOptions& setItemHoverColorRole(const std::string& r) { m_itemHoverColorRole = r; return *this; }
    DropdownOptions& setItemHeight(float h)           { m_itemHeight = h;     return *this; }
    DropdownOptions& setMaxItems(int n)             { m_maxItems = n;       return *this; }
    DropdownOptions& setItemRounding(float r)         { m_itemRounding = r;   return *this; }
    DropdownOptions& setPopupRounding(float r)        { m_popupRounding = r;  return *this; }
    DropdownOptions& setCharSize(unsigned int n)      { m_charSize = n;       return *this; }
    DropdownOptions& setTextColor(Color c)        { m_textColor = c;      return *this; }
    DropdownOptions& setTextColorRole(const std::string& r) { m_textColorRole = r; return *this; }
    DropdownOptions& setHeaderTextColor(Color c)  { m_headerTextColor = c; return *this; }
    DropdownOptions& setHeaderTextColorRole(const std::string& r) { m_headerTextColorRole = r; return *this; }
    DropdownOptions& setPlaceholder(const std::string& s) { m_placeholder = s; return *this; }

    // Padding -------------------------------------------------------------
    // Space either side of the text, in the header and in each popup item, so
    // the labels are not flush against the edge. The header's right-hand inset
    // is setArrowPadding() when an arrow is showing.
    DropdownOptions& setHeaderPadding(float px) { m_headerPadding = px; return *this; }
    DropdownOptions& setItemPadding(float px)   { m_itemPadding = px; return *this; }

    // Outline -------------------------------------------------------------
    // Borders on the two surfaces, drawn inside their bounds. Thickness falls
    // back to Theme::setOutlineThickness() and an unnamed role to
    // Theme::setOutlineColorRole().
    DropdownOptions& setHeaderOutlineColor(const Color& c)           { m_headerOutlineColor = c; return *this; }
    DropdownOptions& setHeaderOutlineColorRole(const std::string& r) { m_headerOutlineColorRole = r; return *this; }
    DropdownOptions& setHeaderOutlineThickness(float px)             { m_headerOutlineThickness = px; return *this; }
    DropdownOptions& setPopupOutlineColor(const Color& c)            { m_popupOutlineColor = c; return *this; }
    DropdownOptions& setPopupOutlineColorRole(const std::string& r)  { m_popupOutlineColorRole = r; return *this; }
    DropdownOptions& setPopupOutlineThickness(float px)              { m_popupOutlineThickness = px; return *this; }

    // Arrow ---------------------------------------------------------------
    // The marker at the right end of the header, the way a dropdown is
    // normally drawn. Native only: on the web a Dropdown becomes a <select>,
    // which the browser draws its own arrow on.
    DropdownOptions& setShowArrow(bool v)                { m_showArrow = v; return *this; }
    DropdownOptions& setArrowIcon(std::string_view n)    { m_arrowIcon = std::string(n); return *this; }
    // Shown while the popup is up, when set; otherwise the arrow does not
    // change on opening.
    DropdownOptions& setArrowOpenIcon(std::string_view n){ m_arrowOpenIcon = std::string(n); return *this; }
    DropdownOptions& setArrowSize(float px)              { m_arrowSize = px; return *this; }
    // Gap between the label and the arrow, and the inset from the right edge.
    DropdownOptions& setArrowSpacing(float px)           { m_arrowSpacing = px; return *this; }
    DropdownOptions& setArrowPadding(float px)           { m_arrowPadding = px; return *this; }
    DropdownOptions& setArrowStrokeWidth(float w)        { m_arrowStroke = w; return *this; }
    DropdownOptions& setArrowColor(Color c)              { m_arrowColor = c; m_hasArrowColor = true; return *this; }
    DropdownOptions& setArrowColorRole(const std::string& r) { m_arrowColorRole = r; m_hasArrowColor = true; return *this; }

    DropdownOptions& setFont(const std::string& path) { m_fontPath = path; return *this; }

    DropdownOptions& setSpacer(float s)                { m_spacer = s;           return *this; }
    DropdownOptions& setDividerThickness(float t)        { m_dividerThickness = t; return *this; }
    DropdownOptions& setDividerColor(Color c)        { m_dividerColor = c;     return *this; }
    DropdownOptions& setDividerColorRole(const std::string& r) { m_dividerColorRole = r; return *this; }
    DropdownOptions& setHeaderTextAlignment(Align x, Align y = Align::CenterY) {
        m_headerTextAlignX = x; m_headerTextAlignY = y; return *this;
    }
    DropdownOptions& setPopupTextAlignment(Align x, Align y = Align::CenterY) {
        m_popupTextAlignX = x; m_popupTextAlignY = y; return *this;
    }

    DropdownOptions& setOnItemChanged(std::function<void(const std::string&)> f) {
        m_onItemChanged = std::move(f); return *this;
    }

    Color        getHeaderColor()     const { return m_headerColor; }
    const std::string& getHeaderColorRole() const { return m_headerColorRole; }
    float        getHeaderRounding()  const { return Theme::resolveRounding(m_headerRounding, 0.f); }
    Color        getPopupColor()      const { return m_popupColor; }
    const std::string& getPopupColorRole() const { return m_popupColorRole; }
    Color        getItemColor()       const { return m_itemColor; }
    const std::string& getItemColorRole() const { return m_itemColorRole; }
    Color        getItemHoverColor()  const { return m_itemHoverColor; }
    const std::string& getItemHoverColorRole() const { return m_itemHoverColorRole; }
    float        getItemHeight()      const { return m_itemHeight; }
    int          getMaxItems()       const { return m_maxItems; }
    float        getItemRounding()    const { return Theme::resolveRounding(m_itemRounding, 0.f); }
    float        getPopupRounding()   const { return Theme::resolveRounding(m_popupRounding, 0.f); }
    unsigned int getCharSize()        const { return Theme::resolveCharSize(m_charSize, 14); }
    bool         hasCharSize()        const { return m_charSize.has_value(); }
    Color        getTextColor()       const { return m_textColor; }
    const std::string& getTextColorRole() const { return m_textColorRole; }
    Color        getHeaderTextColor() const { return m_headerTextColor; }
    const std::string& getHeaderTextColorRole() const { return m_headerTextColorRole; }
    const std::string& getPlaceholder() const { return m_placeholder; }

    float getHeaderPadding()      const { return m_headerPadding; }
    float getItemPadding()        const { return m_itemPadding; }

    Color              getHeaderOutlineColor()     const { return m_headerOutlineColor; }
    const std::string& getHeaderOutlineColorRole() const { return m_headerOutlineColorRole; }
    float              getHeaderOutlineThickness() const { return m_headerOutlineThickness; }
    Color              getPopupOutlineColor()      const { return m_popupOutlineColor; }
    const std::string& getPopupOutlineColorRole()  const { return m_popupOutlineColorRole; }
    float              getPopupOutlineThickness()  const { return m_popupOutlineThickness; }

    bool  getShowArrow()          const { return m_showArrow; }
    const std::string& getArrowIcon()     const { return m_arrowIcon; }
    const std::string& getArrowOpenIcon() const { return m_arrowOpenIcon; }
    float getArrowSize()          const { return m_arrowSize; }
    float getArrowSpacing()       const { return m_arrowSpacing; }
    float getArrowPadding()       const { return m_arrowPadding; }
    const std::optional<float>& getArrowStrokeWidth() const { return m_arrowStroke; }
    // Unset, the arrow follows the header text colour, so it stays legible
    // against the header without being configured.
    bool  hasArrowColor()         const { return m_hasArrowColor; }
    Color getArrowColor()         const { return m_arrowColor; }
    const std::string& getArrowColorRole() const { return m_arrowColorRole; }
    const std::string& getFontPath()  const { return Theme::resolveFont(m_fontPath); }
    float        getSpacer()            const { return m_spacer; }
    float        getDividerThickness()  const { return m_dividerThickness; }
    Color        getDividerColor()      const { return m_dividerColor; }
    const std::string& getDividerColorRole() const { return m_dividerColorRole; }
    Align        getHeaderTextAlignX()  const { return m_headerTextAlignX; }
    Align        getHeaderTextAlignY()  const { return m_headerTextAlignY; }
    Align        getPopupTextAlignX()   const { return m_popupTextAlignX; }
    Align        getPopupTextAlignY()   const { return m_popupTextAlignY; }
    const std::function<void(const std::string&)>& getOnItemChanged() const { return m_onItemChanged; }


    // Raw, for handing to a child element that should keep following
    // the theme rather than being pinned to a resolved number.
    const std::optional<float>& getHeaderRoundingOpt() const { return m_headerRounding; }
    static constexpr float getHeaderRoundingOptFallback() { return 0.f; }
    // Raw, for handing to a child element that should keep following
    // the theme rather than being pinned to a resolved number.
    const std::optional<float>& getItemRoundingOpt() const { return m_itemRounding; }
    static constexpr float getItemRoundingOptFallback() { return 0.f; }
    // Raw, for handing to a child element that should keep following
    // the theme rather than being pinned to a resolved number.
    const std::optional<float>& getPopupRoundingOpt() const { return m_popupRounding; }
    static constexpr float getPopupRoundingOptFallback() { return 0.f; }

private:
    Color        m_headerColor     = Color{60, 60, 60, 255};
    std::string  m_headerColorRole  = "panelAlt";
    std::optional<float>        m_headerRounding;
    Color        m_popupColor      = Color{50, 50, 50, 255};
    std::string  m_popupColorRole   = "panel";
    Color        m_itemColor       = Color{0, 0, 0, 0};
    std::string  m_itemColorRole;
    Color        m_itemHoverColor  = Color{80, 80, 80, 255};
    std::string  m_itemHoverColorRole = "accent";
    float        m_itemHeight      = 30.f;
    int          m_maxItems        = 6;
    std::optional<float>        m_itemRounding;
    std::optional<float>        m_popupRounding;
    std::optional<unsigned int> m_charSize;
    Color        m_textColor       = Color::White;
    std::string  m_textColorRole    = "text";
    Color        m_headerTextColor = Color::White;
    std::string  m_headerTextColorRole = "text";
    std::string  m_placeholder;
    float        m_headerPadding = 10.f;
    float        m_itemPadding   = 10.f;
    Color                m_headerOutlineColor = Color::Transparent;
    std::string          m_headerOutlineColorRole;
    float m_headerOutlineThickness = 0.f;
    Color                m_popupOutlineColor  = Color::Transparent;
    std::string          m_popupOutlineColorRole;
    float m_popupOutlineThickness = 0.f;
    bool         m_showArrow     = true;
    std::string  m_arrowIcon     = "chevron-down";
    std::string  m_arrowOpenIcon = "chevron-up";
    float        m_arrowSize     = 16.f;
    float        m_arrowSpacing  = 8.f;
    float        m_arrowPadding  = 10.f;
    std::optional<float> m_arrowStroke;
    Color        m_arrowColor    = Color::White;
    std::string  m_arrowColorRole;
    bool         m_hasArrowColor = false;
    std::string  m_fontPath;
    float        m_spacer             = 0.f;
    float        m_dividerThickness   = 0.f;
    Color        m_dividerColor       = Color{80, 80, 80, 255};
    std::string  m_dividerColorRole = "outline";
    Align        m_headerTextAlignX   = Align::CenterX;
    Align        m_headerTextAlignY   = Align::CenterY;
    Align        m_popupTextAlignX    = Align::Left;
    Align        m_popupTextAlignY    = Align::CenterY;
    std::function<void(const std::string&)> m_onItemChanged;
};

/*
    Dropdown:
    - Desc:     A header button that opens a list of items. The header is a
                Button and
            the popup a scrollable Column, both built once at construction and
            kept for the element's lifetime -- opening and closing only adds and
            removes the popup from UILO's floating layer, so no element is created
            or destroyed on a click.
    - It is an Element rather than an Interactible because the header does the
      claiming: the press is consumed there whatever callbacks are attached.
    - The popup has to be floating rather than a child, because UILO only ticks,
      hit-tests and renders the floating layer above the page -- a popup left in
      the tree would be clipped by whatever container the dropdown sits in.
    - On the web this becomes a native <select>, so the popup and the arrow are
      never translated; getItems() exists so that backend can populate its own
      control from the whole list rather than just the selection.
*/
class Dropdown : public Element {
public:
    Dropdown(
        Modifier modifier,
        DropdownOptions options,
        std::initializer_list<std::string> items,
        const std::string& name = ""
    );

    void setUILO(UILO& uiloRef) override;
    void update(Rectf& parentBounds, float dt) override;
    void render() override;
    bool checkLeftClick(const Vec2f& mousePosition) override;
    bool checkHover(const Vec2f& mousePosition) override;

    int                getSelectedIndex() const { return m_selectedIndex; }
    const std::string& getSelectedItem()  const;
    void               setSelectedIndex(int idx);
    const std::vector<std::string>& getItems() const { return m_items; }

    const DropdownOptions& getOptions() const { return m_options; }
    DropdownOptions&       getOptions()       { return m_options; }

protected:
    // Clicking the header opens the popup, so a press is consumed whether or not
    // a callback was attached.
    bool claimsPointerEvents() const override { return true; }

private:
    Rectf computePopupBounds() const;
    void  openPopup();
    void  closePopup();
    void  updateHeaderLabel();
    void  updateArrowIcon();

    DropdownOptions          m_options;
    std::vector<std::string> m_items;
    int  m_selectedIndex = -1;
    bool m_isOpen        = false;
    bool m_justDismissed = false;
    int  m_hoveredItem   = -1;

    Text*   m_headerLabel = nullptr;
    Button* m_header      = nullptr;
    // Children of the header after the label: the gap, the arrow, and the inset
    // from the right edge. Held so opening and closing can swap the glyph.
    Spacer* m_arrowGap    = nullptr;
    Icon*   m_arrow       = nullptr;
    Spacer* m_arrowPad    = nullptr;

    std::vector<Text*>   m_itemTexts;
    std::vector<Button*> m_itemButtons;
    std::vector<Spacer*> m_dividers;
    Column*              m_popup = nullptr;
};

} // namespace uilo
