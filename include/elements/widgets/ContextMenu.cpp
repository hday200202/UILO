#include "ContextMenu.hpp"
#include "../../UILO.hpp"

#include <algorithm>

namespace uilo {

namespace {

/* Pixel dimension helper: every metric in ContextMenuOptions is an unscaled
   pixel count, and the layout pass applies the UI scale to it. */
Dimension pxDim(float v) { return Dimension{v, false}; }

} // namespace


/*
    ContextMenu(Modifier modifier, ContextMenuOptions options, const std::string& name):
    - Params:   Modifier modifier, ContextMenuOptions options, const std::string&
                name
    - Returns:  ContextMenu
    - Desc:     Builds an empty menu. It stays empty until setItems(), and holds
                no rows at all until then, so a UILO that is never right-clicked
                pays for nothing but the object.
*/
ContextMenu::ContextMenu(
    Modifier modifier,
    ContextMenuOptions options,
    const std::string& name
) : Column(modifier, ColumnOptions(), {}, name), m_cmOptions(std::move(options)) {
    applySurface();
}


/*
    applySurface():
    - Params:   none
    - Returns:  void
    - Desc:     Pushes the panel fill, rounding, border and vertical inset down
                onto the Column the menu is. Inner padding insets all four sides,
                which is what keeps the highlight off the rounded corners.
*/
void ContextMenu::applySurface() {
    Column::setOptions(
        ColumnOptions()
            .setColor(m_cmOptions.getColor())
            .setColorRole(m_cmOptions.getColorRole())
            .setRounding(m_cmOptions.getRounding())
            .setOutlineColor(m_cmOptions.getOutlineColor())
            .setOutlineColorRole(m_cmOptions.getOutlineColorRole())
            .setOutlineThickness(m_cmOptions.getOutlineThickness())
            .setInnerPadding(m_cmOptions.getVerticalPadding())
    );
}


/*
    makeSlot():
    - Params:   none
    - Returns:  ContextMenu::Slot
    - Desc:     Creates one position's worth of elements and appends them to the
                column: the item row, then the separator rule that position could
                be instead. Both start hidden. The row's parts are created once
                and only ever relabelled, so the structure the web bridge
                translated stays valid.
*/
ContextMenu::Slot ContextMenu::makeSlot() {
    const ContextMenuOptions& o = m_cmOptions;
    Slot slot;

    slot.padLeft  = new Spacer(Modifier().setWidth(pxDim(o.getItemPadding())));
    slot.icon     = new Icon(
        Modifier()
            .setWidth(pxDim(o.getIconSize()))
            .setHeight(pxDim(o.getIconSize()))
            .setAlign(Align::CenterY),
        IconOptions().setColorRole(o.getTextColorRole()));
    slot.iconGap  = new Spacer(Modifier().setWidth(pxDim(o.getIconSpacing())));

    TextOptions labelOpts = TextOptions()
        .setCharSize(o.getCharSize())
        .setColorRole(o.getTextColorRole())
        .setTextAlignX(Align::Left)
        .setTextAlignY(Align::CenterY);
    if (!o.getFont().empty()) labelOpts.setFont(o.getFont());
    slot.label = new Text(Modifier().setAlign(Align::Left | Align::CenterY), labelOpts);

    slot.arrow    = new Icon(
        Modifier()
            .setWidth(pxDim(o.getIconSize()))
            .setHeight(pxDim(o.getIconSize()))
            .setAlign(Align::CenterY),
        IconOptions().setIcon(o.getSubmenuIcon()).setColorRole(o.getTextColorRole()));
    slot.padRight = new Spacer(Modifier().setWidth(pxDim(o.getItemPadding())));

    slot.row = new Button(
        Modifier().setHeight(pxDim(o.getItemHeight())),
        ButtonOptions()
            .setColor(o.getColor())
            .setColorRole(o.getColorRole())
            .setRounding(o.getRounding() * 0.5f));
    slot.row->addElement(slot.padLeft);
    slot.row->addElement(slot.icon);
    slot.row->addElement(slot.iconGap);
    /* The label takes the row's whole leftover, which is what holds the arrow
       against the right edge -- and what measure() sized the menu for. A second
       flexible sibling here would halve the label and clip it. */
    slot.row->addElement(slot.label);
    slot.row->addElement(slot.arrow);
    slot.row->addElement(slot.padRight);

    slot.rule = new Spacer(
        Modifier().setHeight(pxDim(o.getSeparatorThickness()
                                 + 2.f * o.getSeparatorSpacing())),
        SpacerOptions()
            .setColor(o.getSeparatorColor())
            .setColorRole(o.getSeparatorColorRole())
            .setOuterPadding(o.getSeparatorSpacing()));

    slot.row->getModifier().setVisible(false);
    slot.rule->getModifier().setVisible(false);

    addElement(slot.row);
    addElement(slot.rule);
    return slot;
}


/*
    growTo(size_t count):
    - Params:   size_t count
    - Returns:  void
    - Desc:     Makes sure the pool has at least `count` positions. The pool only
                ever grows: a menu that once needed eight rows keeps them, hidden,
                so the next one costs no allocation and the translated structure
                is never invalidated.
*/
void ContextMenu::growTo(size_t count) {
    while (m_slots.size() < count) m_slots.push_back(makeSlot());
}


/*
    rowWidthFor(const ContextMenuItem& item, bool showIconColumn):
    - Params:   const ContextMenuItem& item, bool showIconColumn
    - Returns:  float -- the row's natural width in render pixels
    - Desc:     What one row needs: the two insets, the icon column when the menu
                has one, the measured label, and room for the submenu arrow. Falls
                back to a per-character estimate when no renderer is bound, so a
                headless probe still produces sane numbers.
*/
float ContextMenu::rowWidthFor(const ContextMenuItem& item, bool showIconColumn) const {
    const ContextMenuOptions& o = m_cmOptions;
    const float scale = m_uiloRef ? m_uiloRef->getScale() : 1.f;

    float w = 2.f * o.getItemPadding() * scale;
    if (showIconColumn) w += (o.getIconSize() + o.getIconSpacing()) * scale;
    /* The arrow column is reserved on every row, so labels do not shift when a
       submenu sits beside a plain item. */
    w += (o.getIconSize() + o.getIconSpacing()) * scale;

    const float pxH = static_cast<float>(o.getCharSize()) * scale;
    if (m_uiloRef && m_uiloRef->hasRenderer()) {
        Renderer& renderer = m_uiloRef->getRenderer();
        Font f = renderer.loadFont(o.getFont());
        if (f.valid()) return w + renderer.measureText(item.label, f, pxH).size.x;
    }
    return w + static_cast<float>(item.label.size()) * pxH * 0.55f;
}


/*
    measure():
    - Params:   none
    - Returns:  Vec2f -- the size the current items need, in render pixels
    - Desc:     Width from the widest row, clamped between the configured minimum
                and maximum; height from the rows themselves plus the vertical
                inset above and below. openAt() places exactly this.
*/
Vec2f ContextMenu::measure() const {
    const ContextMenuOptions& o = m_cmOptions;
    const float scale = m_uiloRef ? m_uiloRef->getScale() : 1.f;

    bool showIconColumn = false;
    for (const ContextMenuItem& item : m_items)
        if (!item.icon.empty()) { showIconColumn = true; break; }

    float width  = o.getMinWidth() * scale;
    float height = 2.f * o.getVerticalPadding() * scale;

    for (const ContextMenuItem& item : m_items) {
        if (item.separator) {
            height += (o.getSeparatorThickness() + 2.f * o.getSeparatorSpacing()) * scale;
            continue;
        }
        height += o.getItemHeight() * scale;
        width   = std::max(width, rowWidthFor(item, showIconColumn));
    }

    return { std::min(width, o.getMaxWidth() * scale), height };
}


/*
    applySlot(Slot& slot, const ContextMenuItem& item, bool showIconColumn):
    - Params:   Slot& slot, const ContextMenuItem& item, bool showIconColumn
    - Returns:  void
    - Desc:     Points one pooled position at one item: shows the rule for a
                separator, or the row for anything else, and hides the parts that
                item does not use. A disabled item keeps its place and its label
                and only loses its colour, which is what makes it read as "not
                right now" rather than as missing.
*/
void ContextMenu::applySlot(Slot& slot, const ContextMenuItem& item, bool showIconColumn) {
    const ContextMenuOptions& o = m_cmOptions;

    if (item.separator) {
        slot.row->getModifier().setVisible(false);
        slot.rule->getModifier().setVisible(true);
        return;
    }

    slot.rule->getModifier().setVisible(false);
    slot.row->getModifier().setVisible(true);

    /* setString, not getOptions().setContent: a Text keeps a live string of its
       own and only reads the options' content when it is constructed. */
    slot.label->setString(item.label);
    slot.label->getOptions()
        .setColorRole(item.enabled ? o.getTextColorRole() : o.getDisabledColorRole())
        .setColor(item.enabled ? o.getTextColor() : o.getDisabledColor());

    const bool hasIcon = !item.icon.empty();
    slot.icon->getModifier().setVisible(showIconColumn);
    slot.iconGap->getModifier().setVisible(showIconColumn);
    if (hasIcon) slot.icon->getOptions().setIcon(item.icon);
    /* An icon column with nothing in this row still has to take its width, so
       the element stays visible and is pointed at nothing. */
    else if (showIconColumn) slot.icon->getOptions().setIcon("");

    slot.arrow->getModifier().setVisible(item.isSubmenu());
}


/*
    hideSlot(Slot& slot):
    - Params:   Slot& slot
    - Returns:  void
    - Desc:     Takes a pooled position out of use. Both shapes are hidden, so
                the slot occupies no height and is skipped by layout.
*/
void ContextMenu::hideSlot(Slot& slot) {
    slot.row->getModifier().setVisible(false);
    slot.rule->getModifier().setVisible(false);
}


/*
    setItems(const std::vector<ContextMenuItem>& items):
    - Params:   const std::vector<ContextMenuItem>& items
    - Returns:  void
    - Desc:     Fills the menu. Grows the pool if these items need more positions
                than it has, points each position at its item, hides the rest,
                and wires every row's click to either its action or its submenu.
*/
void ContextMenu::setItems(const std::vector<ContextMenuItem>& items) {
    m_items = items;
    m_highlight = -1;
    m_openSubmenu = -1;

    growTo(m_items.size());

    bool showIconColumn = false;
    for (const ContextMenuItem& item : m_items)
        if (!item.icon.empty()) { showIconColumn = true; break; }

    for (size_t i = 0; i < m_slots.size(); ++i) {
        if (i >= m_items.size()) { hideSlot(m_slots[i]); continue; }

        const ContextMenuItem& item = m_items[i];
        applySlot(m_slots[i], item, showIconColumn);

        /* Rebound every time, because slot i is a different item than it was. */
        m_slots[i].row->getModifier().setOnLeftClick([this, i](Element*) {
            if (i >= m_items.size()) return;
            const ContextMenuItem& clicked = m_items[i];
            if (!clicked.enabled) return;
            if (clicked.isSubmenu()) { highlight(static_cast<int>(i)); openHighlightedSubmenu(); return; }
            /* The action runs after the menu is gone, so a handler that opens
               another menu is not immediately dismissed by this one closing. */
            auto action = clicked.action;
            closeChain();
            if (action) action();
        });

        m_slots[i].row->getModifier().setOnHoverEnter([this, i](Element*) {
            if (i >= m_items.size()) return;
            highlight(static_cast<int>(i));
            /* Travelling onto a submenu row opens it, and onto any other row
               closes whichever was open -- the ordinary menu behaviour. */
            if (m_items[i].isSubmenu() && m_items[i].enabled) openHighlightedSubmenu();
            else if (m_openSubmenu >= 0)                      closeSubmenu();
        });
    }

    paintRow(0, false);   /* refreshes every row's fill for the new items */
}


/*
    paintRow(size_t i, bool hovered):
    - Params:   size_t i, bool hovered
    - Returns:  void
    - Desc:     Repaints every row from the current highlight and hover state.
                Takes an index only so callers read naturally; the whole menu is
                cheap enough to repaint in one pass, and doing so is what keeps a
                stale highlight from being left behind.
*/
void ContextMenu::paintRow(size_t, bool) {
    const ContextMenuOptions& o = m_cmOptions;
    for (size_t i = 0; i < m_slots.size(); ++i) {
        if (i >= m_items.size()) continue;
        if (m_items[i].separator) continue;

        const bool lit = m_items[i].enabled
                      && (static_cast<int>(i) == m_highlight || m_slots[i].row->isHovered());
        m_slots[i].row->getOptions()
            .setColor(lit ? o.getHoverColor() : o.getColor())
            .setColorRole(lit ? o.getHoverColorRole() : o.getColorRole());
    }
}


/*
    highlight(int index):
    - Params:   int index
    - Returns:  void
    - Desc:     Moves the keyboard highlight, closing an open submenu when it
                leaves the row that opened it.
*/
void ContextMenu::highlight(int index) {
    if (m_highlight == index) return;
    if (m_openSubmenu >= 0 && m_openSubmenu != index) closeSubmenu();
    m_highlight = index;
    paintRow(0, false);
}


/*
    moveHighlight(int delta):
    - Params:   int delta -- +1 for the next item, -1 for the previous
    - Returns:  bool -- true when the key was consumed
    - Desc:     Steps the highlight over separators and disabled items, wrapping
                at both ends. Applies to the deepest open submenu, so the arrow
                keys always drive the menu the user is looking at.
*/
bool ContextMenu::moveHighlight(int delta) {
    ContextMenu* target = deepestOpen();
    if (target != this) return target->moveHighlight(delta);
    if (m_items.empty() || delta == 0) return false;

    const int n = static_cast<int>(m_items.size());
    int index = m_highlight;
    for (int step = 0; step < n; ++step) {
        index = (index + delta + n) % n;
        const ContextMenuItem& candidate = m_items[static_cast<size_t>(index)];
        if (candidate.separator || !candidate.enabled) continue;
        highlight(index);
        return true;
    }
    return false;
}


/*
    activateHighlighted():
    - Params:   none
    - Returns:  bool -- true when the key was consumed
    - Desc:     Picks the highlighted item, which opens it if it is a submenu and
                otherwise runs its action and closes the whole chain.
*/
bool ContextMenu::activateHighlighted() {
    ContextMenu* target = deepestOpen();
    if (target != this) return target->activateHighlighted();
    if (m_highlight < 0 || static_cast<size_t>(m_highlight) >= m_items.size()) return false;

    const ContextMenuItem& item = m_items[static_cast<size_t>(m_highlight)];
    if (item.separator || !item.enabled) return false;
    if (item.isSubmenu()) return openHighlightedSubmenu();

    auto action = item.action;
    closeChain();
    if (action) action();
    return true;
}


/*
    submenuFor(size_t index):
    - Params:   size_t index
    - Returns:  ContextMenu* -- the child menu belonging to that row
    - Desc:     The submenu for a row, created the first time that row needs one
                and kept afterwards. Inherits the parent's options, so a menu is
                styled once however deep it nests.
*/
ContextMenu* ContextMenu::submenuFor(size_t index) {
    if (m_submenus.size() <= index) m_submenus.resize(index + 1, nullptr);
    if (!m_submenus[index]) {
        auto* child = new ContextMenu(Modifier(), m_cmOptions);
        child->m_parent = this;
        if (m_uiloRef) child->setUILO(*m_uiloRef);
        m_submenus[index] = child;
    }
    return m_submenus[index];
}


/*
    openHighlightedSubmenu():
    - Params:   none
    - Returns:  bool -- true when a submenu opened
    - Desc:     Opens the highlighted row's submenu beside that row, flipped to
                the left when it would run off the right edge. The two overlap
                slightly, so the pointer never crosses a gap on the way over.
*/
bool ContextMenu::openHighlightedSubmenu() {
    if (m_highlight < 0 || static_cast<size_t>(m_highlight) >= m_items.size()) return false;
    const ContextMenuItem& item = m_items[static_cast<size_t>(m_highlight)];
    if (!item.isSubmenu() || !item.enabled) return false;
    if (m_openSubmenu == m_highlight) return true;
    if (m_openSubmenu >= 0) closeSubmenu();

    ContextMenu* child = submenuFor(static_cast<size_t>(m_highlight));
    child->setItems(item.submenu);

    const Rectf rowBounds = m_slots[static_cast<size_t>(m_highlight)].row->getBounds();
    const float overlap = m_cmOptions.getSubmenuOverlap()
                        * (m_uiloRef ? m_uiloRef->getScale() : 1.f);
    /* Beside the row on the right when there is room, and beside it on the LEFT
       when there is not -- its right edge meeting the row's left edge, mirroring
       the other case. Flipping about the row's right edge instead would drop the
       submenu almost exactly on top of its parent and bury the rows below the
       one that opened it, which is unusable. Vertically it shifts rather than
       flips, so it stays level with its row. */
    child->show(rowBounds.position.x + rowBounds.size.x - overlap,
                rowBounds.position.x + overlap,
                rowBounds.position.y,
                false);

    m_openSubmenu = m_highlight;
    return true;
}


/*
    closeSubmenu():
    - Params:   none
    - Returns:  bool -- true when a submenu was open and is now closed
    - Desc:     Closes the deepest open submenu, which is what Escape and the
                left arrow do: they back out one level rather than dismissing
                the whole menu.
*/
bool ContextMenu::closeSubmenu() {
    if (m_openSubmenu < 0) return false;
    ContextMenu* child = m_submenus[static_cast<size_t>(m_openSubmenu)];
    if (child) {
        if (child->m_openSubmenu >= 0) child->closeSubmenu();
        child->close();
    }
    m_openSubmenu = -1;
    return true;
}


/*
    deepestOpen():
    - Params:   none
    - Returns:  ContextMenu* -- the innermost menu currently showing
    - Desc:     Which menu the keyboard is driving. Walks down the open submenu
                chain rather than up, since the innermost one has focus.
*/
ContextMenu* ContextMenu::deepestOpen() {
    if (m_openSubmenu < 0) return this;
    ContextMenu* child = m_submenus[static_cast<size_t>(m_openSubmenu)];
    return child && child->isOpen() ? child->deepestOpen() : this;
}


/*
    openSubmenu():
    - Params:   none
    - Returns:  ContextMenu* -- the open child menu, or null
    - Desc:     One level down, for a caller that has to visit every menu in an
                open chain rather than only the innermost.
*/
ContextMenu* ContextMenu::openSubmenu() {
    if (m_openSubmenu < 0) return nullptr;
    return m_submenus[static_cast<size_t>(m_openSubmenu)];
}


/*
    show(float preferredLeft, float flipRightEdge, float top, bool flipVertically):
    - Params:   float preferredLeft, float flipRightEdge, float top, bool
                flipVertically
    - Returns:  void
    - Desc:     Places the menu and shows it. It takes `preferredLeft` when it
                fits, and otherwise puts its right edge at `flipRightEdge` -- two
                anchors rather than one because the two callers flip differently:
                a menu at the cursor flips about the cursor, while a submenu has
                to land on the far side of its parent. Registering it as an
                overlay is what gets it hit-tested above the page and dismissed by
                a click outside.
    - Whatever the anchors say, the menu is finally clamped into the window, so on
      a window narrower than the menu it sits against the edge and lets the
      overflow show rather than moving somewhere unexpected.
*/
void ContextMenu::show(float preferredLeft, float flipRightEdge, float top,
                       bool flipVertically) {
    if (!m_uiloRef) return;

    const Vec2f size = measure();
    const auto  win  = m_uiloRef->getWindowSize();
    const float winW = static_cast<float>(win.x);
    const float winH = static_cast<float>(win.y);

    float x = preferredLeft;
    if (x + size.x > winW) x = flipRightEdge - size.x;

    float y = top;
    if (flipVertically && y + size.y > winH) y = top - size.y;

    x = std::clamp(x, 0.f, std::max(0.f, winW - size.x));
    y = std::clamp(y, 0.f, std::max(0.f, winH - size.y));

    m_origin = { x, y };
    m_open   = true;
    m_highlight = -1;

    Rectf bounds{ m_origin, size };
    tick(bounds, 0.f);

    m_uiloRef->registerOverlay(this, [this] { close(); });
}


/*
    openAt(const Vec2f& cursor):
    - Params:   const Vec2f& cursor
    - Returns:  void
    - Desc:     Shows the menu with its top-left at the cursor, flipping about the
                cursor on either axis when it would not fit below and to the
                right of it.
*/
void ContextMenu::openAt(const Vec2f& cursor) {
    show(cursor.x, cursor.x, cursor.y, true);
}


/*
    close():
    - Params:   none
    - Returns:  void
    - Desc:     Hides the menu and any submenu under it, and takes it out of the
                overlay layer. The rows are kept, so reopening allocates nothing.
*/
void ContextMenu::close() {
    if (m_openSubmenu >= 0) closeSubmenu();
    m_open      = false;
    m_highlight = -1;
    if (m_uiloRef) m_uiloRef->unregisterOverlay(this);
}


/*
    closeChain():
    - Params:   none
    - Returns:  void
    - Desc:     Closes this menu and every menu that opened it, so picking an
                item anywhere in a nested chain puts the whole thing away.
*/
void ContextMenu::closeChain() {
    ContextMenu* root = this;
    while (root->m_parent) root = root->m_parent;
    root->close();
}


/*
    update(Rectf& parentBounds, float dt):
    - Params:   Rectf& parentBounds, float dt
    - Returns:  void
    - Desc:     Lays the rows out as an ordinary Column would, then repaints the
                highlight. The paint runs after layout because it reads each
                row's hovered state, which the dispatch sets from the bounds this
                pass resolves.
*/
void ContextMenu::update(Rectf& parentBounds, float dt) {
    Column::update(parentBounds, dt);
    if (m_open) paintRow(0, false);
}

} // namespace uilo
