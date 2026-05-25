#pragma once

#include <functional>
#include <vector>
#include <string>

#include "../Element.hpp"
#include "../decoration/Text.hpp"
#include "../decoration/Spacer.hpp"
#include "Button.hpp"
#include "../containers/Column.hpp"

namespace uilo {

class DropdownOptions {
public:
    DropdownOptions() = default;

    DropdownOptions& setHeaderColor(sf::Color c)      { m_headerColor = c;    return *this; }
    DropdownOptions& setHeaderRounding(float r)       { m_headerRounding = r; return *this; }
    DropdownOptions& setPopupColor(sf::Color c)       { m_popupColor = c;     return *this; }
    DropdownOptions& setItemColor(sf::Color c)        { m_itemColor = c;      return *this; }
    DropdownOptions& setItemHoverColor(sf::Color c)   { m_itemHoverColor = c; return *this; }
    DropdownOptions& setItemHeight(float h)           { m_itemHeight = h;     return *this; }
    DropdownOptions& setMaxItems(int n)             { m_maxItems = n;       return *this; }
    DropdownOptions& setItemRounding(float r)         { m_itemRounding = r;   return *this; }
    DropdownOptions& setPopupRounding(float r)        { m_popupRounding = r;  return *this; }
    DropdownOptions& setCharSize(unsigned int n)      { m_charSize = n;       return *this; }
    DropdownOptions& setTextColor(sf::Color c)        { m_textColor = c;      return *this; }
    DropdownOptions& setHeaderTextColor(sf::Color c)  { m_headerTextColor = c; return *this; }
    DropdownOptions& setPlaceholder(const std::string& s) { m_placeholder = s; return *this; }

    DropdownOptions& setFont(const sf::Font& f)       { m_fontRef = &f; m_fontPath.clear(); return *this; }
    DropdownOptions& setFont(const std::string& path) { m_fontPath = path; m_fontRef = nullptr; return *this; }

    DropdownOptions& setSpacer(float s)                { m_spacer = s;           return *this; }
    DropdownOptions& setDividerThickness(float t)        { m_dividerThickness = t; return *this; }
    DropdownOptions& setDividerColor(sf::Color c)        { m_dividerColor = c;     return *this; }
    DropdownOptions& setHeaderTextAlignment(Align x, Align y = Align::CenterY) {
        m_headerTextAlignX = x; m_headerTextAlignY = y; return *this;
    }
    DropdownOptions& setPopupTextAlignment(Align x, Align y = Align::CenterY) {
        m_popupTextAlignX = x; m_popupTextAlignY = y; return *this;
    }

    DropdownOptions& setOnItemChanged(std::function<void(const std::string&)> f) {
        m_onItemChanged = std::move(f); return *this;
    }

    sf::Color    getHeaderColor()     const { return m_headerColor; }
    float        getHeaderRounding()  const { return m_headerRounding; }
    sf::Color    getPopupColor()      const { return m_popupColor; }
    sf::Color    getItemColor()       const { return m_itemColor; }
    sf::Color    getItemHoverColor()  const { return m_itemHoverColor; }
    float        getItemHeight()      const { return m_itemHeight; }
    int          getMaxItems()       const { return m_maxItems; }
    float        getItemRounding()    const { return m_itemRounding; }
    float        getPopupRounding()   const { return m_popupRounding; }
    unsigned int getCharSize()        const { return m_charSize; }
    sf::Color    getTextColor()       const { return m_textColor; }
    sf::Color    getHeaderTextColor() const { return m_headerTextColor; }
    const std::string& getPlaceholder() const { return m_placeholder; }
    const sf::Font*    getFontRef()   const { return m_fontRef; }
    const std::string& getFontPath()  const { return m_fontPath; }
    float        getSpacer()            const { return m_spacer; }
    float        getDividerThickness()  const { return m_dividerThickness; }
    sf::Color    getDividerColor()      const { return m_dividerColor; }
    Align        getHeaderTextAlignX()  const { return m_headerTextAlignX; }
    Align        getHeaderTextAlignY()  const { return m_headerTextAlignY; }
    Align        getPopupTextAlignX()   const { return m_popupTextAlignX; }
    Align        getPopupTextAlignY()   const { return m_popupTextAlignY; }
    const std::function<void(const std::string&)>& getOnItemChanged() const { return m_onItemChanged; }

private:
    sf::Color    m_headerColor     = sf::Color(60, 60, 60);
    float        m_headerRounding  = 0.f;
    sf::Color    m_popupColor      = sf::Color(50, 50, 50);
    sf::Color    m_itemColor       = sf::Color::Transparent;
    sf::Color    m_itemHoverColor  = sf::Color(80, 80, 80);
    float        m_itemHeight      = 30.f;
    int          m_maxItems        = 6;
    float        m_itemRounding    = 0.f;
    float        m_popupRounding   = 0.f;
    unsigned int m_charSize        = 14;
    sf::Color    m_textColor       = sf::Color::White;
    sf::Color    m_headerTextColor = sf::Color::White;
    std::string  m_placeholder;
    const sf::Font* m_fontRef      = nullptr;
    std::string  m_fontPath;
    float        m_spacer             = 0.f;
    float        m_dividerThickness   = 0.f;
    sf::Color    m_dividerColor       = sf::Color(80, 80, 80);
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
    void update(sf::FloatRect& parentBounds, float dt) override;
    void render(sf::RenderTarget& target) override;
    bool checkLeftClick(const sf::Vector2f& mousePosition) override;
    bool checkHover(const sf::Vector2f& mousePosition) override;

    int                getSelectedIndex() const { return m_selectedIndex; }
    const std::string& getSelectedItem()  const;
    void               setSelectedIndex(int idx);

private:
    sf::FloatRect computePopupBounds() const;
    void          openPopup();
    void          closePopup();
    void          updateHeaderLabel();

    DropdownOptions          m_options;
    std::vector<std::string> m_items;
    int  m_selectedIndex = -1;
    bool m_isOpen        = false;
    bool m_justDismissed = false;
    int  m_hoveredItem   = -1;

    sf::Font        m_ownedFont;
    const sf::Font* m_fontPtr = nullptr;

    Text*   m_headerLabel  = nullptr;
    Button* m_header       = nullptr;

    std::vector<Text*>   m_itemTexts;
    std::vector<Button*> m_itemButtons;
    std::vector<Spacer*> m_dividers;
    Column*              m_popup = nullptr;
};

} // namespace uilo
