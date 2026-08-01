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

    /* Header label. */
    TextOptions headerTextOpts;
    if (!m_options.getFontPath().empty()) headerTextOpts.setFont(m_options.getFontPath());
    headerTextOpts
        .setContent(m_options.getPlaceholder());
    if (m_options.hasCharSize()) headerTextOpts.setCharSize(m_options.getCharSize());
    headerTextOpts
        .setColor(m_options.getHeaderTextColor())
        .setColorRole(m_options.getHeaderTextColorRole())
        .setTextAlignX(m_options.getHeaderTextAlignX())
        .setTextAlignY(m_options.getHeaderTextAlignY());

    m_headerLabel = new Text(
        Modifier().setWidth({100.f, true}).setHeight({100.f, true}),
        headerTextOpts, "");

    /* Header button. */
    m_header = new Button(
        Modifier().setWidth({100.f, true}).setHeight({100.f, true}),
        ButtonOptions()
            .setColor(m_options.getHeaderColor())
            .setColorRole(m_options.getHeaderColorRole())
            .inheritRounding(m_options.getHeaderRoundingOpt(),
                             DropdownOptions::getHeaderRoundingOptFallback())
            .setOutlineColor(m_options.getHeaderOutlineColor())
            .setOutlineColorRole(m_options.getHeaderOutlineColorRole())
            .setOutlineThickness(m_options.getHeaderOutlineThickness()),
        "");

    /* A child rather than setLabel(), which always lands at index 0 and would
       leave no room for the padding spacer ahead of it. */
    if (m_options.getHeaderPadding() > 0.f)
        m_header->addElement(new Spacer(
            Modifier().setWidth({m_options.getHeaderPadding(), false})));
    m_header->addElement(m_headerLabel);

    /* Header arrow. */
    /* The label keeps its percent width and the arrow block is fixed, so the
       arrow stays pinned to the right edge at any header size. */
    if (m_options.getShowArrow() && !m_options.getArrowIcon().empty()) {
        IconOptions arrowOpts = IconOptions()
            .setIcon(m_options.getArrowIcon())
            .setColor(m_options.hasArrowColor() ? m_options.getArrowColor()
                                                : m_options.getHeaderTextColor())
            .setColorRole(m_options.hasArrowColor() ? m_options.getArrowColorRole()
                                                    : m_options.getHeaderTextColorRole());
        if (m_options.getArrowStrokeWidth())
            arrowOpts.setStrokeWidth(*m_options.getArrowStrokeWidth());

        m_arrowGap = new Spacer(
            Modifier().setWidth({m_options.getArrowSpacing(), false}));
        m_arrow = new Icon(
            Modifier()
                .setWidth({m_options.getArrowSize(), false})
                .setHeight({m_options.getArrowSize(), false})
                .setAlign(Align::CenterY),
            arrowOpts);
        m_arrowPad = new Spacer(
            Modifier().setWidth({m_options.getArrowPadding(), false}));

        m_header->addElement(m_arrowGap);
        m_header->addElement(m_arrow);
        m_header->addElement(m_arrowPad);
    } else if (m_options.getHeaderPadding() > 0.f) {
        /* No arrow, so the right side needs its own inset to stay symmetric. */
        m_header->addElement(new Spacer(
            Modifier().setWidth({m_options.getHeaderPadding(), false})));
    }

    /* Popup column; the items are added below. */
    m_popup = new Column(
        Modifier().setWidth({100.f, true}).setHeight({100.f, true}),
        ColumnOptions()
            .setScrollable(true)
            .setColor(m_options.getPopupColor())
            .setColorRole(m_options.getPopupColorRole())
            .inheritRounding(m_options.getPopupRoundingOpt(),
                             DropdownOptions::getPopupRoundingOptFallback())
            .setOutlineColor(m_options.getPopupOutlineColor())
            .setOutlineColorRole(m_options.getPopupOutlineColorRole())
            .setOutlineThickness(m_options.getPopupOutlineThickness()),
        contains{}, "");

    /* Item buttons. */
    for (size_t i = 0; i < m_items.size(); ++i) {
        TextOptions itemTextOpts;
        if (!m_options.getFontPath().empty()) itemTextOpts.setFont(m_options.getFontPath());
        itemTextOpts
            .setContent(m_items[i]);
        if (m_options.hasCharSize()) itemTextOpts.setCharSize(m_options.getCharSize());
        itemTextOpts
            .setColor(m_options.getTextColor())
            .setColorRole(m_options.getTextColorRole())
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
                .setColorRole(m_options.getItemColorRole())
                .inheritRounding(m_options.getItemRoundingOpt(),
                                 DropdownOptions::getItemRoundingOptFallback()),
            "");

        /* Children rather than setLabel() again, so the text is inset from
           both edges instead of sitting against them. */
        const float ip = m_options.getItemPadding();
        if (ip > 0.f) btn->addElement(new Spacer(Modifier().setWidth({ip, false})));
        btn->addElement(txt);
        if (ip > 0.f) btn->addElement(new Spacer(Modifier().setWidth({ip, false})));

        m_popup->addElement(btn);
        m_itemTexts.push_back(txt);
        m_itemButtons.push_back(btn);

        /* A divider after every item but the last. */
        if (m_options.getDividerThickness() > 0.f && i + 1 < m_items.size()) {
            Spacer* div = new Spacer(
                Modifier().setWidth({100.f, true}).setHeight({m_options.getDividerThickness(), false}),
                SpacerOptions().setColor(m_options.getDividerColor()).setColorRole(m_options.getDividerColorRole()));
            m_popup->addElement(div);
            m_dividers.push_back(div);
        }
    }
}

/*
    setUILO(UILO& uiloRef):
    - Params:   UILO& uiloRef
    - Returns:  void
    - Desc:     Binds the dropdown and everything it built to the owning UILO,
                so the header, the popup and all their children land in the
                element pool. The popup is registered here even though it is not
                a child: it has to be owned before it can ever be shown, and it
                is only added to the floating layer when opened.
*/
void Dropdown::setUILO(UILO& uiloRef) {
    Element::setUILO(uiloRef); 
    m_header->setUILO(uiloRef);   /* recurses into the label and the arrow */
    m_popup->setUILO(uiloRef); 
}


/*
    updateArrowIcon():
    - Params:   none
    - Returns:  void
    - Desc:     Points the arrow at the open glyph while the popup is up and
                back at the closed one when it is not. An empty open icon leaves
                it alone, so the arrow can be made to stay put.
*/
void Dropdown::updateArrowIcon() {
    if (!m_arrow) return;

    const std::string& open   = m_options.getArrowOpenIcon();
    const std::string& closed = m_options.getArrowIcon();
    const std::string& want   = (m_isOpen && !open.empty()) ? open : closed;

    if (m_arrow->getOptions().getIcon() != want) {
        m_arrow->getOptions().setIcon(want);
        m_dirty = true;
    }
}

/*
    updateHeaderLabel():
    - Params:   none
    - Returns:  void
    - Desc:     Puts the selected item's text on the header, or the placeholder
                when nothing is selected yet.
*/
void Dropdown::updateHeaderLabel() {
    const std::string& txt =
        (m_selectedIndex >= 0 && static_cast<size_t>(m_selectedIndex) < m_items.size())
        ? m_items[static_cast<size_t>(m_selectedIndex)]
        : m_options.getPlaceholder();
    m_headerLabel->setString(txt);
    m_dirty = true;
}

/*
    setSelectedIndex(int idx):
    - Params:   int idx
    - Returns:  void
    - Desc:     Selects an item by position, updates the header, and fires
                onItemChanged. An index outside the list clears the selection
                back to the placeholder rather than being ignored.
*/
void Dropdown::setSelectedIndex(int idx) {
    if (idx < 0 || static_cast<size_t>(idx) >= m_items.size()) return;
    m_selectedIndex = idx;
    updateHeaderLabel();
    if (m_options.getOnItemChanged())
        m_options.getOnItemChanged()(m_items[static_cast<size_t>(idx)]);
}

/*
    getSelectedItem():
    - Params:   none
    - Returns:  const std::string& -- empty when nothing is selected
    - Desc:     The text of the selected item.
*/
const std::string& Dropdown::getSelectedItem() const {
    static const std::string empty;
    if (m_selectedIndex < 0 || static_cast<size_t>(m_selectedIndex) >= m_items.size())
        return empty;
    return m_items[static_cast<size_t>(m_selectedIndex)];
}

/*
    computePopupBounds():
    - Params:   none
    - Returns:  Rectf
    - Desc:     Where the popup sits: directly under the header, matching its
                width, and as tall as the items need up to the maxItems cap. It
                is flipped above the header when there is not enough room below,
                so a dropdown near the bottom of the window still shows its
                list.
*/
Rectf Dropdown::computePopupBounds() const {
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

/*
    openPopup():
    - Params:   none
    - Returns:  void
    - Desc:     Shows the list by adding the popup to UILO's floating layer,
                which is what gets it ticked, hit-tested and drawn above the
                page instead of clipped by whatever container the dropdown sits
                in. The popup itself already exists, so this only changes where
                it is registered.
*/
void Dropdown::openPopup() {
    if (!m_uiloRef) return;
    m_isOpen = true;
    updateArrowIcon();
    Rectf popupBounds = computePopupBounds();
    m_popup->tick(popupBounds, 0.f);
    m_uiloRef->registerOverlay(m_popup, [this]() { closePopup(); });
}

/*
    closePopup():
    - Params:   none
    - Returns:  void
    - Desc:     Hides the list by removing the popup from the floating layer and
                points the arrow back at its closed glyph. The popup is kept
                rather than destroyed, so reopening costs nothing and the web
                bridge -- which only translates structure once -- is not
                invalidated.
*/
void Dropdown::closePopup() {
    m_isOpen        = false;
    m_justDismissed = true;
    updateArrowIcon();

    if (m_hoveredItem >= 0 && static_cast<size_t>(m_hoveredItem) < m_itemButtons.size()) {
        m_itemButtons[static_cast<size_t>(m_hoveredItem)]->getOptions()
            .setColor(m_options.getItemColor())
            .setColorRole(m_options.getItemColorRole());
    }
    m_hoveredItem = -1;

    if (m_uiloRef) m_uiloRef->unregisterOverlay(m_popup);
}

/*
    update(Rectf& parentBounds, float dt):
    - Params:   Rectf& parentBounds, float dt
    - Returns:  void
    - Desc:     Resolves the dropdown's bounds, ticks the header with them, and
                ticks the popup against its computed bounds while it is open.
                The Modifier's material is mirrored onto both, so a glass
                dropdown looks the same collapsed and open. Also handles
                dismissal: a click outside closes the popup, and the just-
                dismissed flag stops that same click from immediately reopening
                it through the header.
*/
void Dropdown::update(Rectf& parentBounds, float dt) {
    m_justDismissed = false;
    resize(parentBounds);

    /* Mirror the material onto both, so a glass dropdown looks the same
       collapsed and open. */
    const Material& mat = m_modifier.getMaterial();
    m_header->getModifier().setMaterial(mat);
    m_popup->getModifier().setMaterial(mat);

    m_header->tick(m_bounds, dt);

    if (m_isOpen) {
        Rectf popupBounds = computePopupBounds();
        m_popup->tick(popupBounds, dt);

        const Vec2f mousePos = m_uiloRef ? m_uiloRef->getMousePosition() : Vec2f{};
        int newHovered = -1;
        for (size_t i = 0; i < m_itemButtons.size(); ++i) {
            if (m_itemButtons[i]->getBounds().contains(mousePos)) {
                newHovered = static_cast<int>(i);
                break;
            }
        }

        if (newHovered != m_hoveredItem) {
            if (m_hoveredItem >= 0 && static_cast<size_t>(m_hoveredItem) < m_itemButtons.size()) {
                m_itemButtons[static_cast<size_t>(m_hoveredItem)]->getOptions()
                    .setColor(m_options.getItemColor())
                    .setColorRole(m_options.getItemColorRole());
                }
                if (newHovered >= 0) {
                m_itemButtons[static_cast<size_t>(newHovered)]->getOptions()
                    .setColor(m_options.getItemHoverColor())
                    .setColorRole(m_options.getItemHoverColorRole());
            }
            m_hoveredItem = newHovered;
        }
    }
}

/*
    render():
    - Params:   none
    - Returns:  void
    - Desc:     Draws the header. The popup is not drawn here: it is in the
                floating layer, which UILO renders after the page so the list
                appears above everything rather than under a later sibling.
*/
void Dropdown::render() {
    if (!m_modifier.getVisible()) { m_dirty = false; return; }
    /* The material was mirrored onto the header and popup in update(). */
    m_header->render();
    m_dirty = false;
}

/*
    checkLeftClick(const Vec2f& mousePosition):
    - Params:   const Vec2f& mousePosition
    - Returns:  bool -- true when the click landed on the header
    - Desc:     Toggles the popup. A click that just dismissed an open popup is
                swallowed rather than treated as a fresh open, so clicking the
                header of an open dropdown closes it instead of closing and
                reopening in the same frame.
*/
bool Dropdown::checkLeftClick(const Vec2f& mousePosition) {
    if (!m_bounds.contains(mousePosition)) return false;
    if (!m_isOpen && !m_justDismissed) openPopup();
    return true;
}

/*
    checkHover(const Vec2f& mousePosition):
    - Params:   const Vec2f& mousePosition
    - Returns:  bool -- true when the pointer is over the header or the open
                popup
    - Desc:     Forwards hover to the header and, while open, to the popup's
                items, so an item highlights under the pointer even though the
                popup is not a child of this element.
*/
bool Dropdown::checkHover(const Vec2f& mousePosition) {
    if (m_bounds.contains(mousePosition) && m_uiloRef)
        m_uiloRef->requestCursor(CursorType::Hand, 1);
    return Element::checkHover(mousePosition);
}

}
