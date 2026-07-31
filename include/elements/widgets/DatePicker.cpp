#include "DatePicker.hpp"

#include "../../UILO.hpp"

#include <algorithm>

namespace uilo {

namespace {

using DT = DateAndTime;

// The card itself is a plain Column; DatePickerOptions exposes the subset of
// ColumnOptions that makes sense here so the widget keeps one Options type.
ColumnOptions cardOptionsFrom(const DatePickerOptions& o) {
    return ColumnOptions()
        .setColor(o.getBackgroundColor())
        .setColorRole(o.getBackgroundColorRole())
        .setRounding(o.getRounding());
}

// Marks an element and everything below it for deletion. Erasing only the row
// would leave its cells in UILO's element pool for the lifetime of the
// program, since the pool sweep is driven by the flag alone.
void eraseSubtree(Element* element) {
    if (!element) return;

    const ElementType type = element->getType();
    if (type == ElementType::Row || type == ElementType::Column) {
        auto* container = static_cast<Container*>(element);
        for (auto* child : container->getChildren()) eraseSubtree(child);
    }

    if (element->getUILO()) element->erase();
    else                    delete element;
}

std::string weekdayLabel(Weekday day, WeekdayLabelStyle style) {
    switch (style) {
        case WeekdayLabelStyle::Full:  return DT::weekdayName(day, false);
        case WeekdayLabelStyle::Short: return DT::weekdayName(day, true);
        case WeekdayLabelStyle::Initial:
        default:                       return DT::weekdayInitial(day);
    }
}

} // namespace


DatePicker::DatePicker(Modifier modifier, DatePickerOptions options, const std::string& name)
    : Column(modifier, cardOptionsFrom(options), {}, name),
      m_dpOptions(std::move(options)),
      m_embeddedModifier(modifier)
{
    const Date today = DT::today();
    m_displayYear  = today.year;
    m_displayMonth = today.month;
    rebuild();
}


/*
    setOptions(const DatePickerOptions& opts):
    - Params:   const DatePickerOptions& opts
    - Returns:  void
    - Desc:     Swaps in a new configuration, re-applies the card surface, and
                queues a grid rebuild for the next update.
*/
void DatePicker::setOptions(const DatePickerOptions& opts) {
    m_dpOptions = opts;
    applyCardOptions();
    m_needsRebuild = true;
}


/*
    applyCardOptions():
    - Params:   none
    - Returns:  void
    - Desc:     Pushes the background and rounding down onto the Column the
                card is built from.
*/
void DatePicker::applyCardOptions() {
    Column::setOptions(cardOptionsFrom(m_dpOptions));
}


/*
    update(Rectf& parentBounds, float dt):
    - Params:   Rectf& parentBounds, float dt
    - Returns:  void
    - Desc:     Rebuilds the grid first if something queued one, then lays out
                as a Column. Rebuilds are deferred to here because clicks are
                dispatched while the parent walks its child list, and mutating
                that list from a click handler would invalidate the walk in
                progress.
*/
void DatePicker::update(Rectf& parentBounds, float dt) {
    if (m_needsRebuild) rebuild();
    Column::update(parentBounds, dt);
}


// ---------------------------------------------------------------------------
// Selection
// ---------------------------------------------------------------------------

/*
    setSelectedDate(const Date& date):
    - Params:   const Date& date
    - Returns:  void
    - Desc:     Selects a date and pages the grid to its month. Fires no
                callback: this is the application driving the widget, and an
                application that wanted to hear about it already knows.
*/
void DatePicker::setSelectedDate(const Date& date) {
    if (!DT::isValid(date)) return;

    m_selected   = date;
    m_rangeStart = date;
    m_rangeEnd.reset();
    m_awaitingRangeEnd = false;

    showMonth(date.year, date.month);
    applyCellColors();
}


/*
    setRange(const Date& first, const Date& last):
    - Params:   const Date& first, const Date& last
    - Returns:  void
    - Desc:     Sets both ends of a range, in either order, and pages to the
                start. Fires no callback, as with setSelectedDate().
*/
void DatePicker::setRange(const Date& first, const Date& last) {
    if (!DT::isValid(first) || !DT::isValid(last)) return;

    Date lo = first, hi = last;
    DT::order(lo, hi);

    m_selected   = lo;
    m_rangeStart = lo;
    m_rangeEnd   = hi;
    m_awaitingRangeEnd = false;

    showMonth(lo.year, lo.month);
    applyCellColors();
}


/*
    clearSelection():
    - Params:   none
    - Returns:  void
    - Desc:     Drops the selection and any half-made range, and fires
                onCleared.
*/
void DatePicker::clearSelection() {
    const bool had = m_selected.has_value() || m_rangeStart.has_value();

    m_selected.reset();
    m_rangeStart.reset();
    m_rangeEnd.reset();
    m_awaitingRangeEnd = false;

    applyCellColors();
    if (had && m_dpOptions.getOnCleared()) m_dpOptions.getOnCleared()();
}


// ---------------------------------------------------------------------------
// Navigation
// ---------------------------------------------------------------------------

/*
    showMonth(int year, unsigned month):
    - Params:   int year, unsigned month
    - Returns:  void
    - Desc:     Pages the grid to a month, ignoring one outside 1-12. Queues a
                rebuild rather than doing it here, so this is safe to call from
                a click handler.
*/
void DatePicker::showMonth(int year, unsigned month) {
    if (month < 1 || month > 12) return;
    if (m_displayYear == year && m_displayMonth == month) return;

    m_displayYear  = year;
    m_displayMonth = month;

    // Prefer refreshing the existing cells in place over a structural rebuild.
    // It allocates nothing, and -- unlike a rebuild, which replaces the cell
    // elements -- it is visible to the Wt bridge, whose sync only re-applies
    // properties to the elements translated once at build time. A rebuild is
    // only needed when a month can lay out to a different grid shape (variable
    // week rows, or adjacent days drawn as gaps), which updateGridInPlace()
    // rejects; the queued rebuild then runs from update() as before.
    if (!updateGridInPlace(m_displayYear, m_displayMonth))
        m_needsRebuild = true;

    notifyMonthChanged();
}


/*
    updateGridInPlace(int year, unsigned month):
    - Params:   int year, unsigned month
    - Returns:  bool -- true if the grid was paged in place, false if the shape
                differs between months and a rebuild is required instead.
    - Desc:     Rewrites each existing day cell to the day it now shows -- its
                number, whether it belongs to the month, and the date its click
                and hover handlers carry -- then refreshes the title and
                recolours. No element is created or destroyed, so the Wt bridge
                picks the changes up through its ordinary property sync.
*/
bool DatePicker::updateGridInPlace(int year, unsigned month) {
    const DatePickerOptions& o = m_dpOptions;

    // The in-place path holds only when every month draws the same grid: six
    // fixed rows, and neighbour-month days as real cells rather than gaps.
    // Otherwise the cell count or the cells' positions shift between months and
    // only a rebuild can express it.
    if (!o.getFixedWeekRows() || !o.getShowAdjacentMonths()) return false;
    if (m_cells.size() != static_cast<std::size_t>(6u * 7u)) return false;

    Date cursor = DT::gridStart(year, month, o.getFirstDayOfWeek());

    for (std::size_t i = 0; i < m_cells.size(); ++i) {
        const bool adjacent = (cursor.month != month || cursor.year != year);

        m_cellDates[i]    = cursor;
        m_cellAdjacent[i] = adjacent;
        m_cellTexts[i]->setString(std::to_string(cursor.day));

        // The cell's handlers captured its old date by value, so rebind them to
        // the day it now shows. The bridge reads handlers at click time, so
        // swapping the modifier's callbacks is enough -- no re-translation.
        m_cells[i]->getModifier()
            .setOnLeftClick([this, cursor, adjacent](Element*) { handleDayClicked(cursor, adjacent); })
            .setOnHoverEnter([this, cursor](Element*) { setHoverDate(cursor); })
            .setOnHoverExit([this, cursor](Element*) {
                if (m_hoverDate && *m_hoverDate == cursor) setHoverDate(std::nullopt);
            });

        cursor = DT::addDays(cursor, 1);
    }

    if (m_titleText)
        m_titleText->setString(DT::format(Date{year, month, 1u}, o.getTitleFormat()));

    applyCellColors();
    return true;
}


/*
    nextMonth():
    - Params:   none
    - Returns:  void
    - Desc:     Pages forward one month, carrying into the next year.
*/
void DatePicker::nextMonth() {
    const Date next = DT::addMonths(Date{m_displayYear, m_displayMonth, 1}, 1);
    showMonth(next.year, next.month);
}


/*
    previousMonth():
    - Params:   none
    - Returns:  void
    - Desc:     Pages back one month, carrying into the previous year.
*/
void DatePicker::previousMonth() {
    const Date prev = DT::addMonths(Date{m_displayYear, m_displayMonth, 1}, -1);
    showMonth(prev.year, prev.month);
}


/*
    nextYear():
    - Params:   none
    - Returns:  void
    - Desc:     Pages forward twelve months.
*/
void DatePicker::nextYear() {
    showMonth(m_displayYear + 1, m_displayMonth);
}


/*
    previousYear():
    - Params:   none
    - Returns:  void
    - Desc:     Pages back twelve months.
*/
void DatePicker::previousYear() {
    showMonth(m_displayYear - 1, m_displayMonth);
}


/*
    goToToday():
    - Params:   none
    - Returns:  void
    - Desc:     Pages to the current month without touching the selection.
*/
void DatePicker::goToToday() {
    const Date today = DT::today();
    showMonth(today.year, today.month);
}


/*
    notifyMonthChanged():
    - Params:   none
    - Returns:  void
    - Desc:     Fires onMonthChanged for the month now on show.
*/
void DatePicker::notifyMonthChanged() {
    if (m_dpOptions.getOnMonthChanged())
        m_dpOptions.getOnMonthChanged()(m_displayYear, m_displayMonth);
}


// ---------------------------------------------------------------------------
// Popup presentation
// ---------------------------------------------------------------------------

/*
    hasFooterButtons():
    - Params:   none
    - Returns:  bool
    - Desc:     Whether the footer would draw anything: showFooter on and at
                least one label non-empty. Both preferredHeight() and
                buildFooter() go through this so the height reserved and the
                row actually built can never disagree.
*/
bool DatePicker::hasFooterButtons() const {
    const DatePickerOptions& o = m_dpOptions;
    if (!o.getShowFooter()) return false;
    return !o.getTodayButtonLabel().empty()
        || !o.getClearButtonLabel().empty()
        || !o.getCancelButtonLabel().empty()
        || !o.getConfirmButtonLabel().empty();
}


/*
    preferredHeight():
    - Params:   none
    - Returns:  float
    - Desc:     The height the configured metrics add up to, in virtual
                pixels: padding, header, weekday strip, six week rows (or as
                many as the month needs when fixed rows are off), and the
                footer. What open() uses when no explicit popup height is set,
                and what sizeToContent() writes into the modifier.
*/
float DatePicker::preferredHeight() const {
    const DatePickerOptions& o = m_dpOptions;

    float height = o.getPadding() * 2.f;

    if (o.getShowHeader())
        height += o.getHeaderHeight() + o.getHeaderSpacing();

    if (o.getShowWeekdayHeader())
        height += o.getWeekdayHeight();

    const unsigned rows = o.getFixedWeekRows()
        ? 6u
        : DT::weeksInMonthGrid(m_displayYear, m_displayMonth, o.getFirstDayOfWeek());
    height += static_cast<float>(rows) * o.getCellHeight();

    // Nothing is reserved for a footer that has no buttons in it -- an
    // embedded picker with the footer off ends exactly at its last week.
    if (hasFooterButtons())
        height += o.getFooterHeight() + o.getFooterSpacing();

    return height;
}


/*
    sizeToContent():
    - Params:   none
    - Returns:  DatePicker*
    - Desc:     Fixes the picker's height at preferredHeight() so an embedded
                one takes exactly the room it needs instead of stretching to
                its slot. Returns this, to be used inline where the element is
                declared:

                    datepicker(Modifier(), opts)->sizeToContent()

                With fixed week rows (the default) one call holds for every
                month. With them off the height follows the month on show, so
                call it again from onMonthChanged to keep it tight.
*/
DatePicker* DatePicker::sizeToContent() {
    m_modifier.setHeight(Dimension{preferredHeight(), false});
    m_embeddedModifier = m_modifier;
    m_dirty = true;
    return this;
}


/*
    open(UILO& uiloRef):
    - Params:   UILO& uiloRef
    - Returns:  void
    - Desc:     Presents the picker centred over the window on a dimmed
                backdrop. The backdrop goes in as a floating element rather
                than an overlay because UILO ticks and renders floating
                elements every frame but leaves overlays to whoever owns them
                -- and a popup picker, being outside the page tree, has no
                owner to do that. Clicks land on the backdrop first, which is
                what makes the popup modal.
*/
void DatePicker::open(UILO& uiloRef) {
    if (m_isOpen) return;

    const DatePickerOptions& o = m_dpOptions;

    if (!m_backdrop) {
        // Full-window, so it catches every click; the card is its only child
        // and centres itself on both axes.
        m_backdrop = new Column(
            Modifier().setWidth(100_pct).setHeight(100_pct),
            ColumnOptions()
                .setColor(o.getScrimColor())
                .setColorRole(o.getScrimColorRole()),
            contains{}, "");
        m_backdrop->addElement(this);
    }

    // A click that reaches the backdrop itself missed the card, since
    // Container::checkLeftClick offers children first and only falls through
    // when none of them took it.
    m_backdrop->getModifier().setOnLeftClick([this](Element*) {
        if (m_dpOptions.getDismissOnScrimClick()) close();
    });
    m_backdrop->getOptions()
        .setColor(o.getScrimColor())
        .setColorRole(o.getScrimColorRole());

    const float height = o.getPopupHeight() > 0.f ? o.getPopupHeight() : preferredHeight();
    m_modifier
        .setWidth(Dimension{o.getPopupWidth(), false})
        .setHeight(Dimension{height, false})
        .setAlign(Align::CenterX | Align::CenterY);

    m_isOpen = true;
    m_dirty  = true;
    uiloRef.addFloating(FreeElement{m_backdrop});
}


/*
    open():
    - Params:   none
    - Returns:  void
    - Desc:     Opens against the UILO the picker already knows -- one it was
                opened with before, or inherited from a parent. No-op before
                either has happened, since there is nothing to present on.
*/
void DatePicker::open() {
    if (m_uiloRef) open(*m_uiloRef);
}


/*
    close():
    - Params:   none
    - Returns:  void
    - Desc:     Takes the backdrop back out of the floating list and restores
                the modifier the picker was constructed with, so a picker that
                is also used embedded goes back to its layout geometry. Fires
                onClosed.
*/
void DatePicker::close() {
    if (!m_isOpen) return;

    m_isOpen = false;
    if (m_uiloRef && m_backdrop) m_uiloRef->removeFloating(m_backdrop);

    m_modifier = m_embeddedModifier;
    m_dirty    = true;

    if (m_dpOptions.getOnClosed()) m_dpOptions.getOnClosed()();
}


// ---------------------------------------------------------------------------
// Building
// ---------------------------------------------------------------------------

/*
    clearRows():
    - Params:   none
    - Returns:  void
    - Desc:     Tears down the whole grid and forgets the per-cell tracking
                arrays.
*/
void DatePicker::clearRows() {
    for (auto* child : m_children) eraseSubtree(child);
    m_children.clear();

    m_cells.clear();
    m_cellTexts.clear();
    m_cellDates.clear();
    m_cellAdjacent.clear();
    m_titleText = nullptr;

    m_dirty = true;
}


/*
    rebuild():
    - Params:   none
    - Returns:  void
    - Desc:     Builds the card contents for the month on show: header,
                weekday strip, day grid, footer. Only paging months and
                changing options come through here -- selecting and hovering
                go through applyCellColors(), which touches no allocation.
*/
void DatePicker::rebuild() {
    m_needsRebuild = false;
    clearRows();

    const DatePickerOptions& o = m_dpOptions;
    const float pad = o.getPadding();

    if (pad > 0.f) addElement(new Spacer(Modifier().setHeight(Dimension{pad, false})));

    if (o.getShowHeader()) {
        addElement(buildHeader());
        if (o.getHeaderSpacing() > 0.f)
            addElement(new Spacer(Modifier().setHeight(Dimension{o.getHeaderSpacing(), false})));
    }

    if (o.getShowWeekdayHeader()) addElement(buildWeekdayHeader());

    addElement(buildGrid());

    Element* footer = o.getShowFooter() ? buildFooter() : nullptr;

    if (footer) {
        // Two equal percent spacers split whatever room is left below the grid,
        // which centres the button row between the last week and the bottom
        // edge however tall the card ends up. The bottom padding is part of
        // that slack rather than a separate gap underneath.
        addElement(new Spacer(Modifier().setHeight(50_pct)));
        addElement(footer);
        addElement(new Spacer(Modifier().setHeight(50_pct)));
    } else if (pad > 0.f) {
        addElement(new Spacer(Modifier().setHeight(Dimension{pad, false})));
    }

    applyCellColors();
}


/*
    buildHeader():
    - Params:   none
    - Returns:  Element*
    - Desc:     The title strip: optional year arrows, month arrows, and the
                month name filling the space between them.
*/
Element* DatePicker::buildHeader() {
    const DatePickerOptions& o = m_dpOptions;
    const float pad = o.getPadding();

    auto* header = new Row(
        Modifier().setHeight(Dimension{o.getHeaderHeight(), false}),
        RowOptions(), contains{});

    if (pad > 0.f) header->addElement(new Spacer(Modifier().setWidth(Dimension{pad, false})));

    if (o.getShowYearNavigation())
        header->addElement(buildNavButton(o.getPrevYearIcon(), [this]() { previousYear(); }));

    header->addElement(buildNavButton(o.getPrevMonthIcon(), [this]() { previousMonth(); }));

    const Date first{m_displayYear, m_displayMonth, 1};
    m_titleText = new Text(
        Modifier().setWidth(100_pct).setAlign(Align::CenterX | Align::CenterY),
        TextOptions()
            .setFont(o.getFontPath())
            .setContent(DT::format(first, o.getTitleFormat()))
            .setCharSize(o.getHeaderCharSize())
            .setColor(o.getHeaderTextColor())
            .setColorRole(o.getHeaderTextColorRole())
            .setTextAlignX(Align::CenterX)
            .setTextAlignY(Align::CenterY)
            .setBold(o.getHeaderBold()));

    if (o.getTitleClickGoesToToday()) {
        // The Text has no handlers of its own, so wrapping it in a Row is what
        // gives the title something clickable.
        auto* titleWrap = new Row(
            Modifier()
                .setWidth(100_pct)
                .setOnLeftClick([this](Element*) { goToToday(); }),
            RowOptions(), contains{});
        titleWrap->addElement(m_titleText);
        header->addElement(titleWrap);
    } else {
        header->addElement(m_titleText);
    }

    header->addElement(buildNavButton(o.getNextMonthIcon(), [this]() { nextMonth(); }));

    if (o.getShowYearNavigation())
        header->addElement(buildNavButton(o.getNextYearIcon(), [this]() { nextYear(); }));

    if (pad > 0.f) header->addElement(new Spacer(Modifier().setWidth(Dimension{pad, false})));

    return header;
}


/*
    buildNavButton(const std::string& iconName, std::function<void()> action):
    - Params:   const std::string& iconName, std::function<void()> action
    - Returns:  Element*
    - Desc:     A square icon button. A Row rather than a Button because
                ButtonOptions only takes a Text label, and these carry icons.
*/
Element* DatePicker::buildNavButton(const std::string& iconName, std::function<void()> action) {
    const DatePickerOptions& o = m_dpOptions;

    const Color       baseColor  = o.getNavColor();
    const std::string baseRole   = o.getNavColorRole();
    const Color       hoverColor = o.getNavHoverColor();
    const std::string hoverRole  = o.getNavHoverColorRole();

    Modifier mod = Modifier()
        .setWidth(Dimension{o.getNavButtonSize(), false})
        .setHeight(Dimension{o.getNavButtonSize(), false})
        .setAlign(Align::CenterY)
        .setOnLeftClick([action = std::move(action)](Element*) { action(); })
        .setOnHoverEnter([hoverColor, hoverRole](Row* r) {
            r->getOptions().setColor(hoverColor).setColorRole(hoverRole);
        })
        .setOnHoverExit([baseColor, baseRole](Row* r) {
            r->getOptions().setColor(baseColor).setColorRole(baseRole);
        });

    auto* button = new Row(
        mod,
        RowOptions()
            .setColor(baseColor)
            .setColorRole(baseRole)
            .setRounding(o.getNavRounding()),
        contains{});

    if (!iconName.empty()) {
        IconOptions iconOpts = IconOptions()
            .setIcon(iconName)
            .setColor(o.getNavIconColor())
            .setColorRole(o.getNavIconColorRole());
        if (o.hasNavIconStrokeWidth()) iconOpts.setStrokeWidth(o.getNavIconStrokeWidth());

        button->addElement(new Icon(
            Modifier()
                .setWidth(Dimension{o.getNavIconSize(), false})
                .setHeight(Dimension{o.getNavIconSize(), false})
                .setAlign(Align::CenterX | Align::CenterY),
            iconOpts));
    }

    return button;
}


/*
    buildWeekdayHeader():
    - Params:   none
    - Returns:  Element*
    - Desc:     Seven column headings, starting from the configured first day
                of the week. Split the same way as the grid below so the
                letters line up over their columns.
*/
Element* DatePicker::buildWeekdayHeader() {
    const DatePickerOptions& o = m_dpOptions;
    const float pad = o.getPadding();

    auto* row = new Row(
        Modifier().setHeight(Dimension{o.getWeekdayHeight(), false}),
        RowOptions(), contains{});

    if (pad > 0.f) row->addElement(new Spacer(Modifier().setWidth(Dimension{pad, false})));

    for (unsigned column = 0; column < 7; ++column) {
        const Weekday day = DT::weekdayInColumn(column, o.getFirstDayOfWeek());
        row->addElement(new Text(
            Modifier()
                .setWidth(Dimension{100.f / 7.f, true})
                .setAlign(Align::CenterX | Align::CenterY),
            TextOptions()
                .setFont(o.getFontPath())
                .setContent(weekdayLabel(day, o.getWeekdayLabelStyle()))
                .setCharSize(o.getWeekdayCharSize())
                .setColor(o.getWeekdayColor())
                .setColorRole(o.getWeekdayColorRole())
                .setTextAlignX(Align::CenterX)
                .setTextAlignY(Align::CenterY)
                .setBold(o.getWeekdayBold())));
    }

    if (pad > 0.f) row->addElement(new Spacer(Modifier().setWidth(Dimension{pad, false})));

    return row;
}


/*
    buildGrid():
    - Params:   none
    - Returns:  Element*
    - Desc:     The day cells, as a Column of week Rows. Every row holds seven
                cells whatever the month, walking whole weeks from gridStart()
                so the columns stay aligned to their weekdays; cells outside
                the month are marked adjacent and styled apart.
*/
Element* DatePicker::buildGrid() {
    const DatePickerOptions& o = m_dpOptions;
    const float pad = o.getPadding();

    const unsigned rows = o.getFixedWeekRows()
        ? 6u
        : DT::weeksInMonthGrid(m_displayYear, m_displayMonth, o.getFirstDayOfWeek());

    auto* grid = new Column(
        Modifier().setHeight(Dimension{static_cast<float>(rows) * o.getCellHeight(), false}),
        ColumnOptions(), contains{});

    Date cursor = DT::gridStart(m_displayYear, m_displayMonth, o.getFirstDayOfWeek());

    m_cells.reserve(static_cast<std::size_t>(rows) * 7u);
    m_cellTexts.reserve(static_cast<std::size_t>(rows) * 7u);
    m_cellDates.reserve(static_cast<std::size_t>(rows) * 7u);
    m_cellAdjacent.reserve(static_cast<std::size_t>(rows) * 7u);

    for (unsigned week = 0; week < rows; ++week) {
        auto* weekRow = new Row(
            Modifier().setHeight(Dimension{o.getCellHeight(), false}),
            RowOptions(), contains{});

        if (pad > 0.f) weekRow->addElement(new Spacer(Modifier().setWidth(Dimension{pad, false})));

        for (unsigned column = 0; column < 7; ++column) {
            const bool adjacent = (cursor.month != m_displayMonth || cursor.year != m_displayYear);

            if (adjacent && !o.getShowAdjacentMonths()) {
                // Hold the column open so the rest of the week stays put.
                weekRow->addElement(new Spacer(
                    Modifier().setWidth(Dimension{100.f / 7.f, true})));
            } else {
                weekRow->addElement(buildDayCell(cursor, adjacent));
            }

            cursor = DT::addDays(cursor, 1);
        }

        if (pad > 0.f) weekRow->addElement(new Spacer(Modifier().setWidth(Dimension{pad, false})));

        grid->addElement(weekRow);
    }

    return grid;
}


/*
    buildDayCell(const Date& date, bool adjacent):
    - Params:   const Date& date, bool adjacent
    - Returns:  Element*
    - Desc:     One day: a Row carrying the fill and the click, with the day
                number centred in it. Colours are left to applyCellColors(),
                which runs immediately after the grid is assembled and again
                on every selection or hover change.
*/
Element* DatePicker::buildDayCell(const Date& date, bool adjacent) {
    const DatePickerOptions& o = m_dpOptions;

    auto* label = new Text(
        Modifier().setAlign(Align::CenterX | Align::CenterY),
        TextOptions()
            .setFont(o.getFontPath())
            .setContent(std::to_string(date.day))
            .setCharSize(o.getCharSize())
            .setColor(o.getDayTextColor())
            .setColorRole(o.getDayTextColorRole())
            .setTextAlignX(Align::CenterX)
            .setTextAlignY(Align::CenterY));

    Modifier mod = Modifier()
        .setWidth(Dimension{100.f / 7.f, true})
        .setOuterPadding(o.getCellSpacing())
        .setOnLeftClick([this, date, adjacent](Element*) { handleDayClicked(date, adjacent); })
        .setOnHoverEnter([this, date](Element*) { setHoverDate(date); })
        .setOnHoverExit([this, date](Element*) {
            // Only clear if the cursor left *this* cell: the enter for the
            // next one may already have run.
            if (m_hoverDate && *m_hoverDate == date) setHoverDate(std::nullopt);
        });

    auto* cell = new Row(
        mod,
        RowOptions().setRounding(o.getCellRounding()),
        contains{});
    cell->addElement(label);

    m_cells.push_back(cell);
    m_cellTexts.push_back(label);
    m_cellDates.push_back(date);
    m_cellAdjacent.push_back(adjacent);

    return cell;
}


/*
    buildFooter():
    - Params:   none
    - Returns:  Element*
    - Desc:     The action row: Today and Clear on the left, Cancel and the
                confirm button on the right. Returns nothing when every label
                is empty, so an all-empty footer takes up no space.
*/
Element* DatePicker::buildFooter() {
    const DatePickerOptions& o = m_dpOptions;
    const float pad = o.getPadding();

    if (!hasFooterButtons()) return nullptr;

    auto* footer = new Row(
        Modifier().setHeight(Dimension{o.getFooterHeight(), false}),
        RowOptions(), contains{});

    if (pad > 0.f) footer->addElement(new Spacer(Modifier().setWidth(Dimension{pad, false})));

    if (!o.getTodayButtonLabel().empty())
        footer->addElement(buildFooterButton(o.getTodayButtonLabel(), false, [this]() {
            const Date today = DT::today();
            goToToday();
            if (isSelectable(today)) handleDayClicked(today, false);
        }));

    if (!o.getClearButtonLabel().empty())
        footer->addElement(buildFooterButton(o.getClearButtonLabel(), false, [this]() {
            clearSelection();
        }));

    // Cancel sits midway between the left-hand buttons and the confirm button
    // at the right end: two equal percent spacers divide the free width, so
    // whatever is left of it goes half to each side.
    footer->addElement(new Spacer(Modifier().setWidth(50_pct)));

    if (!o.getCancelButtonLabel().empty())
        footer->addElement(buildFooterButton(o.getCancelButtonLabel(), false, [this]() {
            if (m_dpOptions.getOnCancelled()) m_dpOptions.getOnCancelled()();
            close();
        }));

    footer->addElement(new Spacer(Modifier().setWidth(50_pct)));

    if (!o.getConfirmButtonLabel().empty())
        footer->addElement(buildFooterButton(o.getConfirmButtonLabel(), true, [this]() {
            if (m_dpOptions.getOnConfirmed() && m_selected)
                m_dpOptions.getOnConfirmed()(*m_selected);
            close();
        }));

    if (pad > 0.f) footer->addElement(new Spacer(Modifier().setWidth(Dimension{pad, false})));

    return footer;
}


/*
    buildFooterButton(const std::string& label, bool primary, std::function<void()> action):
    - Params:   const std::string& label, bool primary, std::function<void()> action
    - Returns:  Element*
    - Desc:     One footer button. The primary one takes the confirm colours,
                which are the accent by default so the committing action reads
                as the committing action.
*/
Element* DatePicker::buildFooterButton(const std::string& label, bool primary,
                                       std::function<void()> action) {
    const DatePickerOptions& o = m_dpOptions;

    const Color       fill     = primary ? o.getConfirmButtonColor()     : o.getButtonColor();
    const std::string fillRole = primary ? o.getConfirmButtonColorRole() : o.getButtonColorRole();
    const Color       text     = primary ? o.getConfirmButtonTextColor()     : o.getButtonTextColor();
    const std::string textRole = primary ? o.getConfirmButtonTextColorRole() : o.getButtonTextColorRole();

    auto* labelText = new Text(
        Modifier().setWidth(100_pct).setHeight(100_pct),
        TextOptions()
            .setFont(o.getFontPath())
            .setContent(label)
            .setCharSize(o.getFooterCharSize())
            .setColor(text)
            .setColorRole(textRole)
            .setTextAlignX(Align::CenterX)
            .setTextAlignY(Align::CenterY));

    return new Button(
        Modifier()
            .setWidth(Dimension{o.getButtonWidth(), false})
            .setHeight(100_pct)
            .setAlign(Align::CenterY)
            .setOuterPadding(2.f)
            .setOnLeftClick([action = std::move(action)](Element*) { action(); }),
        ButtonOptions()
            .setColor(fill)
            .setColorRole(fillRole)
            .setRounding(o.getButtonRounding())
            .setLabel(labelText),
        "");
}


// ---------------------------------------------------------------------------
// Cell state
// ---------------------------------------------------------------------------

/*
    isSelectable(const Date& date):
    - Params:   const Date& date
    - Returns:  bool
    - Desc:     Whether a day can be picked: a real date, inside any min/max
                bounds, and passed by the dateEnabled predicate.
*/
bool DatePicker::isSelectable(const Date& date) const {
    if (!DT::isValid(date)) return false;

    const auto& min = m_dpOptions.getMinDate();
    const auto& max = m_dpOptions.getMaxDate();
    if (min && date < *min) return false;
    if (max && date > *max) return false;

    const auto& predicate = m_dpOptions.getDateEnabled();
    if (predicate && !predicate(date)) return false;

    return true;
}


/*
    isInRange(const Date& date):
    - Params:   const Date& date
    - Returns:  bool
    - Desc:     True for a day strictly between the ends of a range. While the
                second end is still missing the cell under the cursor stands
                in for it, which is what previews the span as the pointer
                moves.
*/
bool DatePicker::isInRange(const Date& date) const {
    if (m_dpOptions.getMode() != DatePickerMode::Range) return false;
    if (!m_rangeStart) return false;

    const std::optional<Date> other = m_rangeEnd
        ? m_rangeEnd
        : (m_awaitingRangeEnd ? m_hoverDate : std::nullopt);
    if (!other) return false;

    Date lo = *m_rangeStart, hi = *other;
    DT::order(lo, hi);
    return date > lo && date < hi;
}


/*
    isRangeEndpoint(const Date& date):
    - Params:   const Date& date
    - Returns:  bool
    - Desc:     True at either end of a range, the previewed end included.
*/
bool DatePicker::isRangeEndpoint(const Date& date) const {
    if (m_dpOptions.getMode() != DatePickerMode::Range) return false;
    if (m_rangeStart && date == *m_rangeStart) return true;
    if (m_rangeEnd   && date == *m_rangeEnd)   return true;
    if (m_awaitingRangeEnd && m_hoverDate && date == *m_hoverDate) return true;
    return false;
}


/*
    applyCellColors():
    - Params:   none
    - Returns:  void
    - Desc:     Re-derives every visible cell's fill, text colour, and weight
                from the current state, in priority order: disabled, then
                selected or a range endpoint, then inside a range, then today,
                then hover, then a plain day. Mutating the existing options
                rather than rebuilding is what lets selection and hover cost
                nothing.
*/
void DatePicker::applyCellColors() {
    const DatePickerOptions& o = m_dpOptions;
    const Date today = DT::today();

    for (std::size_t i = 0; i < m_cells.size(); ++i) {
        Row*  cell     = m_cells[i];
        Text* label    = m_cellTexts[i];
        const Date& d  = m_cellDates[i];
        const bool adjacent = m_cellAdjacent[i];

        Color       fill     = o.getCellColor();
        std::string fillRole = o.getCellColorRole();
        Color       text     = adjacent ? o.getAdjacentTextColor()     : o.getDayTextColor();
        std::string textRole = adjacent ? o.getAdjacentTextColorRole() : o.getDayTextColorRole();
        bool        bold     = false;

        if (!adjacent && o.hasWeekendTextColor() && DT::isWeekend(d)) {
            text     = o.getWeekendTextColor();
            textRole = o.getWeekendTextColorRole();
        }

        const bool enabled  = isSelectable(d);
        const bool selected = (m_selected && *m_selected == d) || isRangeEndpoint(d);
        const bool inRange  = isInRange(d);
        const bool isToday  = (d == today);

        if (!enabled) {
            text     = o.getDisabledTextColor();
            textRole = o.getDisabledTextColorRole();
        } else if (selected) {
            fill     = o.getSelectedColor();
            fillRole = o.getSelectedColorRole();
            text     = o.getSelectedTextColor();
            textRole = o.getSelectedTextColorRole();
            bold     = o.getBoldSelected();
        } else if (inRange) {
            fill     = o.getRangeFillColor();
            fillRole = o.getRangeFillColorRole();
            text     = o.getRangeTextColor();
            textRole = o.getRangeTextColorRole();
        } else if (isToday) {
            fill     = o.getTodayColor();
            fillRole = o.getTodayColorRole();
            if (o.hasTodayTextColor()) {
                text     = o.getTodayTextColor();
                textRole = o.getTodayTextColorRole();
            }
            bold = o.getBoldToday();
        } else if (m_hoverDate && *m_hoverDate == d) {
            fill     = o.getCellHoverColor();
            fillRole = o.getCellHoverColorRole();
        }

        // Mutating the live options is enough: a Row resolves its colour every
        // frame, and Text notices a changed colour against its own cache.
        cell->getOptions().setColor(fill).setColorRole(fillRole).setRounding(o.getCellRounding());
        label->getOptions().setColor(text).setColorRole(textRole).setBold(bold);
    }
    m_dirty = true;
}


/*
    setHoverDate(const std::optional<Date>& date):
    - Params:   const std::optional<Date>& date
    - Returns:  void
    - Desc:     Tracks which day the cursor is over and restyles the grid when
                that changes. The hover fill could be done per cell, but a
                half-made range has to repaint the whole previewed span, which
                only the grid as a whole can do.
*/
void DatePicker::setHoverDate(const std::optional<Date>& date) {
    if (m_hoverDate == date) return;
    m_hoverDate = date;
    applyCellColors();
}


/*
    handleDayClicked(const Date& date, bool adjacent):
    - Params:   const Date& date, bool adjacent
    - Returns:  void
    - Desc:     Applies a click on a day cell. Single mode replaces the
                selection outright; Range mode opens a span on the first click
                and closes it on the second. A click on a neighbour-month day
                pages the view to that month, so the picked date is never left
                off screen.
*/
void DatePicker::handleDayClicked(const Date& date, bool adjacent) {
    const DatePickerOptions& o = m_dpOptions;

    if (adjacent && !o.getSelectAdjacentMonths()) return;
    if (!isSelectable(date)) return;

    bool rangeCompleted = false;

    if (o.getMode() == DatePickerMode::Range) {
        if (!m_rangeStart || !m_awaitingRangeEnd) {
            // First click of a new span, or the start of a fresh one after the
            // last was completed.
            m_rangeStart = date;
            m_rangeEnd.reset();
            m_awaitingRangeEnd = true;
        } else {
            Date lo = *m_rangeStart, hi = date;
            DT::order(lo, hi);
            m_rangeStart = lo;
            m_rangeEnd   = hi;
            m_awaitingRangeEnd = false;
            rangeCompleted = true;
        }
        m_selected = m_rangeStart;
    } else {
        m_selected   = date;
        m_rangeStart = date;
        m_rangeEnd.reset();
        m_awaitingRangeEnd = false;
    }

    if (adjacent) showMonth(date.year, date.month);

    applyCellColors();

    if (o.getOnDateSelected()) o.getOnDateSelected()(date);
    if (rangeCompleted && o.getOnRangeSelected())
        o.getOnRangeSelected()(*m_rangeStart, *m_rangeEnd);

    // In Range mode there is nothing to close on until the span is complete.
    const bool selectionSettled = o.getMode() == DatePickerMode::Range ? rangeCompleted : true;
    if (m_isOpen && o.getCloseOnSelect() && selectionSettled) close();
}

} // namespace uilo
