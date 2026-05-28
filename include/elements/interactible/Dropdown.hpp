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

namespace uilo {

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
    float        getHeaderRounding()  const { return m_headerRounding; }
    Color        getPopupColor()      const { return m_popupColor; }
    const std::string& getPopupColorRole() const { return m_popupColorRole; }
    Color        getItemColor()       const { return m_itemColor; }
    const std::string& getItemColorRole() const { return m_itemColorRole; }
    Color        getItemHoverColor()  const { return m_itemHoverColor; }
    const std::string& getItemHoverColorRole() const { return m_itemHoverColorRole; }
    float        getItemHeight()      const { return m_itemHeight; }
    int          getMaxItems()       const { return m_maxItems; }
    float        getItemRounding()    const { return m_itemRounding; }
    float        getPopupRounding()   const { return m_popupRounding; }
    unsigned int getCharSize()        const { return m_charSize.value_or(14); }
    bool         hasCharSize()        const { return m_charSize.has_value(); }
    Color        getTextColor()       const { return m_textColor; }
    const std::string& getTextColorRole() const { return m_textColorRole; }
    Color        getHeaderTextColor() const { return m_headerTextColor; }
    const std::string& getHeaderTextColorRole() const { return m_headerTextColorRole; }
    const std::string& getPlaceholder() const { return m_placeholder; }
    const std::string& getFontPath()  const { return m_fontPath; }
    float        getSpacer()            const { return m_spacer; }
    float        getDividerThickness()  const { return m_dividerThickness; }
    Color        getDividerColor()      const { return m_dividerColor; }
    const std::string& getDividerColorRole() const { return m_dividerColorRole; }
    Align        getHeaderTextAlignX()  const { return m_headerTextAlignX; }
    Align        getHeaderTextAlignY()  const { return m_headerTextAlignY; }
    Align        getPopupTextAlignX()   const { return m_popupTextAlignX; }
    Align        getPopupTextAlignY()   const { return m_popupTextAlignY; }
    const std::function<void(const std::string&)>& getOnItemChanged() const { return m_onItemChanged; }

private:
    Color        m_headerColor     = Color{60, 60, 60, 255};
    std::string  m_headerColorRole;
    float        m_headerRounding  = 0.f;
    Color        m_popupColor      = Color{50, 50, 50, 255};
    std::string  m_popupColorRole;
    Color        m_itemColor       = Color{0, 0, 0, 0};
    std::string  m_itemColorRole;
    Color        m_itemHoverColor  = Color{80, 80, 80, 255};
    std::string  m_itemHoverColorRole;
    float        m_itemHeight      = 30.f;
    int          m_maxItems        = 6;
    float        m_itemRounding    = 0.f;
    float        m_popupRounding   = 0.f;
    std::optional<unsigned int> m_charSize;
    Color        m_textColor       = Color::White;
    std::string  m_textColorRole;
    Color        m_headerTextColor = Color::White;
    std::string  m_headerTextColorRole;
    std::string  m_placeholder;
    std::string  m_fontPath;
    float        m_spacer             = 0.f;
    float        m_dividerThickness   = 0.f;
    Color        m_dividerColor       = Color{80, 80, 80, 255};
    std::string  m_dividerColorRole;
    Align        m_headerTextAlignX   = Align::CenterX;
    Align        m_headerTextAlignY   = Align::CenterY;
    Align        m_popupTextAlignX    = Align::Left;
    Align        m_popupTextAlignY    = Align::CenterY;
    std::function<void(const std::string&)> m_onItemChanged;
};

class Dropdown : public Element {
public:
    Dropdown(Modifier modifier, DropdownOptions options,
             std::initializer_list<std::string> items,
             const std::string& name = "");

    void setUILO(UILO& uiloRef) override;
    void update(Rectf& parentBounds, float dt) override;
    void render() override;
    bool checkLeftClick(const Vec2f& mousePosition) override;
    bool checkHover(const Vec2f& mousePosition) override;

    int                getSelectedIndex() const { return m_selectedIndex; }
    const std::string& getSelectedItem()  const;
    void               setSelectedIndex(int idx);

    const DropdownOptions& getOptions() const { return m_options; }
    DropdownOptions&       getOptions()       { return m_options; }

private:
    Rectf computePopupBounds() const;
    void          openPopup();
    void          closePopup();
    void          updateHeaderLabel();

    DropdownOptions          m_options;
    std::vector<std::string> m_items;
    int  m_selectedIndex = -1;
    bool m_isOpen        = false;
    bool m_justDismissed = false;
    int  m_hoveredItem   = -1;

    // TODO: font handle (FreeType/fontstash) — deferred
    // const sf::Font* m_fontPtr = nullptr;

    Text*   m_headerLabel  = nullptr;
    Button* m_header       = nullptr;

    std::vector<Text*>   m_itemTexts;
    std::vector<Button*> m_itemButtons;
    std::vector<Spacer*> m_dividers;
    Column*              m_popup = nullptr;
};

} // namespace uilo
