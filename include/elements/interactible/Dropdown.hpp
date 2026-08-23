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

#include "../../utils/Themed.hpp"

namespace uilo {

/*
    DropdownOptions:
    - Desc:     Everything a Dropdown draws, in three groups: the header the
                user clicks, the popup that opens under it, and the items inside
                that popup. Colors come as a literal plus a role, where the role
                wins when it resolves against the active Palette and the literal
                is the fallback.
    - setMaxItems caps how tall the popup grows; beyond that it scrolls rather
      than running off the window.
*/
class DropdownOptions {
public:
    UILO_THEMED(DropdownOptions)

    /*
        inheritFrom(const DropdownOptions& prototype):
        - Params:   const DropdownOptions& prototype
        - Returns:  void
        - Desc:     Fills in every field the call site left alone from the
                    theme's prototype, and leaves the rest exactly as it was
                    set. Run when the element binds to its UILO, and again
                    whenever that UILO's theme changes.
    */
    void inheritFrom(const DropdownOptions& prototype) {
        m_dividerThickness.inherit(prototype.m_dividerThickness);
        m_headerOutlineThickness.inherit(prototype.m_headerOutlineThickness);
        m_popupOutlineThickness.inherit(prototype.m_popupOutlineThickness);
        m_outerPadding.inherit(prototype.m_outerPadding);
        m_headerRounding.inherit(prototype.m_headerRounding);
        m_itemRounding.inherit(prototype.m_itemRounding);
        m_popupRounding.inherit(prototype.m_popupRounding);
        m_charSize.inherit(prototype.m_charSize);
        m_arrowStroke.inherit(prototype.m_arrowStroke);
        m_headerColorRole.inherit(prototype.m_headerColorRole);
        m_popupColorRole.inherit(prototype.m_popupColorRole);
        m_itemColorRole.inherit(prototype.m_itemColorRole);
        m_itemHoverColorRole.inherit(prototype.m_itemHoverColorRole);
        m_textColorRole.inherit(prototype.m_textColorRole);
        m_headerTextColorRole.inherit(prototype.m_headerTextColorRole);
        m_headerOutlineColorRole.inherit(prototype.m_headerOutlineColorRole);
        m_popupOutlineColorRole.inherit(prototype.m_popupOutlineColorRole);
        m_arrowColorRole.inherit(prototype.m_arrowColorRole);
        m_dividerColorRole.inherit(prototype.m_dividerColorRole);
        m_fontPath.inherit(prototype.m_fontPath);
    }

    // Space kept outside this element, inside the slot its parent gave it. It
    // shrinks the element rather than displacing a sibling. Unset follows the theme's default for this type.
    DropdownOptions& setOuterPadding(float px)   { m_outerPadding.set(px); return *this; }
    DropdownOptions& clearOuterPadding()         { m_outerPadding.clear(); return *this; }
    float  getOuterPadding()     const { return m_outerPadding.get().value_or(0.f); }

    DropdownOptions& setHeaderColor(Color c)      { m_headerColor = c;    return *this; }
    DropdownOptions& setHeaderColorRole(const std::string& r) { m_headerColorRole.set(r); return *this; }
    DropdownOptions& setHeaderRounding(float r)       { m_headerRounding.set(r); return *this; }
    DropdownOptions& setPopupColor(Color c)       { m_popupColor = c;     return *this; }
    DropdownOptions& setPopupColorRole(const std::string& r) { m_popupColorRole.set(r); return *this; }
    DropdownOptions& setItemColor(Color c)        { m_itemColor = c;      return *this; }
    DropdownOptions& setItemColorRole(const std::string& r) { m_itemColorRole.set(r); return *this; }
    DropdownOptions& setItemHoverColor(Color c)   { m_itemHoverColor = c; return *this; }
    DropdownOptions& setItemHoverColorRole(const std::string& r) { m_itemHoverColorRole.set(r); return *this; }
    DropdownOptions& setItemHeight(float h)           { m_itemHeight = h;     return *this; }
    DropdownOptions& setMaxItems(int n)             { m_maxItems = n;       return *this; }
    DropdownOptions& setItemRounding(float r)         { m_itemRounding.set(r);   return *this; }
    DropdownOptions& setPopupRounding(float r)        { m_popupRounding.set(r);  return *this; }
    DropdownOptions& setCharSize(unsigned int n)      { m_charSize.set(n);       return *this; }
    DropdownOptions& setTextColor(Color c)        { m_textColor = c;      return *this; }
    DropdownOptions& setTextColorRole(const std::string& r) { m_textColorRole.set(r); return *this; }
    DropdownOptions& setHeaderTextColor(Color c)  { m_headerTextColor = c; return *this; }
    DropdownOptions& setHeaderTextColorRole(const std::string& r) { m_headerTextColorRole.set(r); return *this; }
    DropdownOptions& setPlaceholder(const std::string& s) { m_placeholder = s; return *this; }

    // Padding -------------------------------------------------------------
    // Space either side of the text, in the header and in each popup item, so
    // the labels are not flush against the edge. The header's right-hand inset
    // is setArrowPadding() when an arrow is showing.
    DropdownOptions& setHeaderPadding(float px) { m_headerPadding = px; return *this; }
    DropdownOptions& setItemPadding(float px)   { m_itemPadding = px; return *this; }

    // Outline -------------------------------------------------------------
    // Borders on the two surfaces, drawn inside their bounds. Thickness and
    // role default to whatever Defaults.hpp gives a DropdownOptions.
    DropdownOptions& setHeaderOutlineColor(const Color& c)           { m_headerOutlineColor = c; return *this; }
    DropdownOptions& setHeaderOutlineColorRole(const std::string& r) { m_headerOutlineColorRole.set(r); return *this; }
    DropdownOptions& setHeaderOutlineThickness(float px)             { m_headerOutlineThickness.set(px); return *this; }
    DropdownOptions& setPopupOutlineColor(const Color& c)            { m_popupOutlineColor = c; return *this; }
    DropdownOptions& setPopupOutlineColorRole(const std::string& r)  { m_popupOutlineColorRole.set(r); return *this; }
    DropdownOptions& setPopupOutlineThickness(float px)              { m_popupOutlineThickness.set(px); return *this; }

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
    DropdownOptions& setArrowStrokeWidth(float w)        { m_arrowStroke.set(w); return *this; }
    DropdownOptions& setArrowColor(Color c)              { m_arrowColor = c; m_hasArrowColor = true; return *this; }
    DropdownOptions& setArrowColorRole(const std::string& r) { m_arrowColorRole.set(r); m_hasArrowColor = true; return *this; }

    DropdownOptions& setFont(std::string_view path) { m_fontPath.set(std::string(path)); return *this; }

    DropdownOptions& setSpacer(float s)                { m_spacer = s;           return *this; }
    DropdownOptions& setDividerThickness(float t)        { m_dividerThickness.set(t); return *this; }
    DropdownOptions& setDividerColor(Color c)        { m_dividerColor = c;     return *this; }
    DropdownOptions& setDividerColorRole(const std::string& r) { m_dividerColorRole.set(r); return *this; }
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
    const std::string& getHeaderColorRole() const { return m_headerColorRole.get(); }
    float        getHeaderRounding()  const { return m_headerRounding.get().value_or(0.f); }
    Color        getPopupColor()      const { return m_popupColor; }
    const std::string& getPopupColorRole() const { return m_popupColorRole.get(); }
    Color        getItemColor()       const { return m_itemColor; }
    const std::string& getItemColorRole() const { return m_itemColorRole.get(); }
    Color        getItemHoverColor()  const { return m_itemHoverColor; }
    const std::string& getItemHoverColorRole() const { return m_itemHoverColorRole.get(); }
    float        getItemHeight()      const { return m_itemHeight; }
    int          getMaxItems()       const { return m_maxItems; }
    float        getItemRounding()    const { return m_itemRounding.get().value_or(0.f); }
    float        getPopupRounding()   const { return m_popupRounding.get().value_or(0.f); }
    unsigned int getCharSize()        const { return m_charSize.get().value_or(14); }
    bool         hasCharSize()        const { return m_charSize.get().has_value(); }
    Color        getTextColor()       const { return m_textColor; }
    const std::string& getTextColorRole() const { return m_textColorRole.get(); }
    Color        getHeaderTextColor() const { return m_headerTextColor; }
    const std::string& getHeaderTextColorRole() const { return m_headerTextColorRole.get(); }
    const std::string& getPlaceholder() const { return m_placeholder; }

    float getHeaderPadding()      const { return m_headerPadding; }
    float getItemPadding()        const { return m_itemPadding; }

    Color              getHeaderOutlineColor()     const { return m_headerOutlineColor; }
    const std::string& getHeaderOutlineColorRole() const { return m_headerOutlineColorRole.get(); }
    float              getHeaderOutlineThickness() const { return m_headerOutlineThickness.get(); }
    Color              getPopupOutlineColor()      const { return m_popupOutlineColor; }
    const std::string& getPopupOutlineColorRole()  const { return m_popupOutlineColorRole.get(); }
    float              getPopupOutlineThickness()  const { return m_popupOutlineThickness.get(); }

    bool  getShowArrow()          const { return m_showArrow; }
    const std::string& getArrowIcon()     const { return m_arrowIcon; }
    const std::string& getArrowOpenIcon() const { return m_arrowOpenIcon; }
    float getArrowSize()          const { return m_arrowSize; }
    float getArrowSpacing()       const { return m_arrowSpacing; }
    float getArrowPadding()       const { return m_arrowPadding; }
    const std::optional<float>& getArrowStrokeWidth() const { return m_arrowStroke.get(); }
    // Unset, the arrow follows the header text colour, so it stays legible
    // against the header without being configured.
    bool  hasArrowColor()         const { return m_hasArrowColor; }
    Color getArrowColor()         const { return m_arrowColor; }
    const std::string& getArrowColorRole() const { return m_arrowColorRole.get(); }
    const std::string& getFontPath()  const { return m_fontPath.get(); }
    float        getSpacer()            const { return m_spacer; }
    float        getDividerThickness()  const { return m_dividerThickness.get(); }
    Color        getDividerColor()      const { return m_dividerColor; }
    const std::string& getDividerColorRole() const { return m_dividerColorRole.get(); }
    Align        getHeaderTextAlignX()  const { return m_headerTextAlignX; }
    Align        getHeaderTextAlignY()  const { return m_headerTextAlignY; }
    Align        getPopupTextAlignX()   const { return m_popupTextAlignX; }
    Align        getPopupTextAlignY()   const { return m_popupTextAlignY; }
    const std::function<void(const std::string&)>& getOnItemChanged() const { return m_onItemChanged; }


    // Raw, for handing to a child element that should keep following
    // the theme rather than being pinned to a resolved number.
    const std::optional<float>& getHeaderRoundingOpt() const { return m_headerRounding.get(); }
    static constexpr float getHeaderRoundingOptFallback() { return 0.f; }
    // Raw, for handing to a child element that should keep following
    // the theme rather than being pinned to a resolved number.
    const std::optional<float>& getItemRoundingOpt() const { return m_itemRounding.get(); }
    static constexpr float getItemRoundingOptFallback() { return 0.f; }
    // Raw, for handing to a child element that should keep following
    // the theme rather than being pinned to a resolved number.
    const std::optional<float>& getPopupRoundingOpt() const { return m_popupRounding.get(); }
    static constexpr float getPopupRoundingOptFallback() { return 0.f; }

private:
    Themed<std::optional<float>> m_outerPadding;
    Color        m_headerColor     = Color{60, 60, 60, 255};
    Themed<std::string> m_headerColorRole {"panelAlt"};
    Themed<std::optional<float>> m_headerRounding;
    Color        m_popupColor      = Color{50, 50, 50, 255};
    Themed<std::string> m_popupColorRole {"panel"};
    Color        m_itemColor       = Color{0, 0, 0, 0};
    Themed<std::string> m_itemColorRole;
    Color        m_itemHoverColor  = Color{80, 80, 80, 255};
    Themed<std::string> m_itemHoverColorRole {"accent"};
    float        m_itemHeight      = 30.f;
    int          m_maxItems        = 6;
    Themed<std::optional<float>> m_itemRounding;
    Themed<std::optional<float>> m_popupRounding;
    Themed<std::optional<unsigned int>> m_charSize;
    Color        m_textColor       = Color::White;
    Themed<std::string> m_textColorRole {"text"};
    Color        m_headerTextColor = Color::White;
    Themed<std::string> m_headerTextColorRole {"text"};
    std::string  m_placeholder;
    float        m_headerPadding = 10.f;
    float        m_itemPadding   = 10.f;
    Color                m_headerOutlineColor = Color::Transparent;
    Themed<std::string> m_headerOutlineColorRole;
    Themed<float> m_headerOutlineThickness {0.f};
    Color                m_popupOutlineColor  = Color::Transparent;
    Themed<std::string> m_popupOutlineColorRole;
    Themed<float> m_popupOutlineThickness {0.f};
    bool         m_showArrow     = true;
    std::string  m_arrowIcon     = "chevron-down";
    std::string  m_arrowOpenIcon = "chevron-up";
    float        m_arrowSize     = 16.f;
    float        m_arrowSpacing  = 8.f;
    float        m_arrowPadding  = 10.f;
    Themed<std::optional<float>> m_arrowStroke;
    Color        m_arrowColor    = Color::White;
    Themed<std::string> m_arrowColorRole;
    bool         m_hasArrowColor = false;
    Themed<std::string> m_fontPath;
    float        m_spacer             = 0.f;
    Themed<float> m_dividerThickness {0.f};
    Color        m_dividerColor       = Color{80, 80, 80, 255};
    Themed<std::string> m_dividerColorRole {"outline"};
    Align        m_headerTextAlignX   = Align::CenterX;
    Align        m_headerTextAlignY   = Align::CenterY;
    Align        m_popupTextAlignX    = Align::Left;
    Align        m_popupTextAlignY    = Align::CenterY;
    std::function<void(const std::string&)> m_onItemChanged;
    // The theme role this was constructed with; resolved at bind time.
    std::string m_themeRole;
};

/*
    Dropdown:
    - Desc:     A header button that opens a list of items. The header is a
                Button and the popup a scrollable Column, both built once at
                construction and kept for the element's lifetime -- opening and
                closing only adds and removes the popup from UILO's floating
                layer, so no element is created or destroyed on a click.
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
    void applyTheme(const Theme& theme) override {
        m_options.inheritFrom(theme.cascade<DropdownOptions>(m_options.getThemeRole()));
        Element::applyTheme(theme);
    }

    float getOuterPadding() const override { return m_options.getOuterPadding(); }

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
