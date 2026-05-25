#include "Dropdown.hpp"
#include "../../UILO.hpp"

#include <algorithm>

namespace uilo {

Dropdown::Dropdown(
    Modifier modifier, DropdownOptions options,
    std::initializer_list<std::string> items,
    const std::string& name
): m_options(std::move(options)), m_items(items) {
    m_modifier = modifier;
    m_name     = name;
    m_type     = ElementType::Dropdown;

    // Resolve font
    if (m_options.getFontRef()) {
        m_fontPtr = m_options.getFontRef();
    } else if (!m_options.getFontPath().empty()) {
        if (m_ownedFont.openFromFile(m_options.getFontPath()))
            m_fontPtr = &m_ownedFont;
    }

    // --- Header label ---
    TextOptions headerTextOpts;
    if (m_fontPtr) headerTextOpts.setFont(*m_fontPtr);
    headerTextOpts
        .setContent(m_options.getPlaceholder())
        .setCharSize(m_options.getCharSize())
        .setColor(m_options.getHeaderTextColor())
        .setTextAlignX(m_options.getHeaderTextAlignX())
        .setTextAlignY(m_options.getHeaderTextAlignY());

    m_headerLabel = new Text(
        Modifier().setWidth({100.f, true}).setHeight({100.f, true}),
        headerTextOpts, "");

    // --- Header button ---
    m_header = new Button(
        Modifier().setWidth({100.f, true}).setHeight({100.f, true}),
        ButtonOptions()
            .setColor(m_options.getHeaderColor())
            .setRounding(m_options.getHeaderRounding())
            .setLabel(m_headerLabel),
        "");

    // --- Popup column (empty children; items added via addElement below) ---
    m_popup = new Column(
        Modifier().setWidth({100.f, true}).setHeight({100.f, true}),
        ColumnOptions()
            .setScrollable(true)
            .setColor(m_options.getPopupColor())
            .setRounding(m_options.getPopupRounding()),
        contains{}, "");

    // --- Item buttons ---
    for (size_t i = 0; i < m_items.size(); ++i) {
        TextOptions itemTextOpts;
        if (m_fontPtr) itemTextOpts.setFont(*m_fontPtr);
        itemTextOpts
            .setContent(m_items[i])
            .setCharSize(m_options.getCharSize())
            .setColor(m_options.getTextColor())
            .setTextAlignX(m_options.getPopupTextAlignX())
            .setTextAlignY(m_options.getPopupTextAlignY());

        Text* txt = new Text(
            Modifier().setWidth({100.f, true}).setHeight({100.f, true}),
            itemTextOpts, "");

        const size_t idx = i;
        Button* btn = new Button(
            Modifier()
                .setWidth({100.f, true})
                .setHeight({m_options.getItemHeight(), false})
                .setOnLeftClick([this, idx]() {
                    setSelectedIndex(static_cast<int>(idx));
                    closePopup();
                }),
            ButtonOptions()
                .setColor(m_options.getItemColor())
                .setRounding(m_options.getItemRounding())
                .setLabel(txt),
            "");

        m_popup->addElement(btn);
        m_itemTexts.push_back(txt);
        m_itemButtons.push_back(btn);

        // Insert a divider spacer after every item except the last
        if (m_options.getDividerThickness() > 0.f && i + 1 < m_items.size()) {
            Spacer* div = new Spacer(
                Modifier().setWidth({100.f, true}).setHeight({m_options.getDividerThickness(), false}),
                SpacerOptions().setColor(m_options.getDividerColor()));
            m_popup->addElement(div);
            m_dividers.push_back(div);
        }
    }
}

void Dropdown::setUILO(UILO& uiloRef) {
    Element::setUILO(uiloRef); 
    m_header->setUILO(uiloRef);
    m_popup->setUILO(uiloRef); 
}

void Dropdown::updateHeaderLabel() {
    const std::string& txt =
        (m_selectedIndex >= 0 && static_cast<size_t>(m_selectedIndex) < m_items.size())
        ? m_items[static_cast<size_t>(m_selectedIndex)]
        : m_options.getPlaceholder();
    m_headerLabel->setString(txt);
}

void Dropdown::setSelectedIndex(int idx) {
    if (idx < 0 || static_cast<size_t>(idx) >= m_items.size()) return;
    m_selectedIndex = idx;
    updateHeaderLabel();
    if (m_options.getOnItemChanged())
        m_options.getOnItemChanged()(m_items[static_cast<size_t>(idx)]);
}

const std::string& Dropdown::getSelectedItem() const {
    static const std::string empty;
    if (m_selectedIndex < 0 || static_cast<size_t>(m_selectedIndex) >= m_items.size())
        return empty;
    return m_items[static_cast<size_t>(m_selectedIndex)];
}

sf::FloatRect Dropdown::computePopupBounds() const {
    const float scale      = m_uiloRef ? m_uiloRef->getScale() : 1.f;
    const float itemH      = m_options.getItemHeight() * scale;
    const float divH       = m_options.getDividerThickness() * scale;
    const size_t nItems    = m_items.size();
    const size_t nDividers = nItems > 1 ? nItems - 1 : 0;
    const float totalH     = static_cast<float>(nItems) * itemH
                           + static_cast<float>(nDividers) * divH;
    const int   maxN    = m_options.getMaxItems();
    const float maxH    = static_cast<float>(maxN) * itemH
                        + static_cast<float>(std::max(0, maxN - 1)) * divH;
    const float popupH     = std::min(totalH, maxH);
    const float popupX     = m_bounds.position.x;
    const float popupW     = m_bounds.size.x;
    float       popupY     = m_bounds.position.y + m_bounds.size.y
                           + m_options.getSpacer() * scale;

    if (m_uiloRef) {
        const auto winSize = m_uiloRef->getWindowSize();
        if (popupY + popupH > static_cast<float>(winSize.y))
            popupY = m_bounds.position.y - popupH;
    }

    return { {popupX, popupY}, {popupW, popupH} };
}

void Dropdown::openPopup() {
    if (!m_uiloRef) return;
    m_isOpen = true;
    sf::FloatRect popupBounds = computePopupBounds();
    m_popup->update(popupBounds, 0.f);
    m_uiloRef->registerOverlay(m_popup, [this]() { closePopup(); });
}

void Dropdown::closePopup() {
    m_isOpen        = false;
    m_justDismissed = true;

    if (m_hoveredItem >= 0 && static_cast<size_t>(m_hoveredItem) < m_itemButtons.size()) {
        m_itemButtons[static_cast<size_t>(m_hoveredItem)]->setOptions(
            ButtonOptions()
                .setColor(m_options.getItemColor())
                .setRounding(m_options.getItemRounding())
                .setLabel(m_itemTexts[static_cast<size_t>(m_hoveredItem)]));
    }
    m_hoveredItem = -1;

    if (m_uiloRef) m_uiloRef->unregisterOverlay(m_popup);
}

void Dropdown::update(sf::FloatRect& parentBounds, float dt) {
    m_justDismissed = false;
    resize(parentBounds);

    m_header->update(m_bounds, dt);

    if (m_isOpen) {
        sf::FloatRect popupBounds = computePopupBounds();
        m_popup->update(popupBounds, dt);

        const sf::Vector2f mousePos = m_uiloRef ? m_uiloRef->getMousePosition() : sf::Vector2f{};
        int newHovered = -1;
        for (size_t i = 0; i < m_itemButtons.size(); ++i) {
            if (m_itemButtons[i]->getBounds().contains(mousePos)) {
                newHovered = static_cast<int>(i);
                break;
            }
        }

        if (newHovered != m_hoveredItem) {
            if (m_hoveredItem >= 0 && static_cast<size_t>(m_hoveredItem) < m_itemButtons.size()) {
                m_itemButtons[static_cast<size_t>(m_hoveredItem)]->setOptions(
                    ButtonOptions()
                        .setColor(m_options.getItemColor())
                        .setRounding(m_options.getItemRounding())
                        .setLabel(m_itemTexts[static_cast<size_t>(m_hoveredItem)]));
                }
                if (newHovered >= 0) {
                m_itemButtons[static_cast<size_t>(newHovered)]->setOptions(
                    ButtonOptions()
                        .setColor(m_options.getItemHoverColor())
                        .setRounding(m_options.getItemRounding())
                        .setLabel(m_itemTexts[static_cast<size_t>(newHovered)]));
            }
            m_hoveredItem = newHovered;
        }
    }
}

void Dropdown::render(sf::RenderTarget& target) {
    m_header->render(target);
}

bool Dropdown::checkLeftClick(const sf::Vector2f& mousePosition) {
    if (!m_bounds.contains(mousePosition)) return false;
    if (!m_isOpen && !m_justDismissed) openPopup();
    return true;
}

bool Dropdown::checkHover(const sf::Vector2f& mousePosition) {
    if (m_bounds.contains(mousePosition) && m_uiloRef)
        m_uiloRef->requestCursor(sf::Cursor::Type::Hand, 1);
    return Element::checkHover(mousePosition);
}

}
