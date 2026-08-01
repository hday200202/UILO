#include "DateField.hpp"

#include "../../UILO.hpp"

namespace uilo {

namespace {

using DT = DateAndTime;

/* The strip itself is a plain Row; DateFieldOptions exposes the subset of
   RowOptions that makes sense here so the widget keeps a single Options type. */
RowOptions surfaceOptionsFrom(const DateFieldOptions& o) {
    return RowOptions()
        .setColor(o.getBackgroundColor())
        .setColorRole(o.getBackgroundColorRole())
        .inheritRounding(o.getRoundingOpt(), DateFieldOptions::getRoundingOptFallback())
        .setOutlineColor(o.getOutlineColor())
        .setOutlineColorRole(o.getOutlineColorRole())
        .setOutlineThickness(o.getOutlineThickness());
}

} // namespace


/*
    DateField(Modifier modifier, DateFieldOptions options, const std::string& name):
    - Params:   Modifier modifier, DateFieldOptions options,
                const std::string& name
    - Returns:  DateField
    - Desc:     Builds the field as a Row and creates every part it will ever
                need -- the insets, the icon, the gap, the label and the chevron
                -- whatever shape it is currently in. Parts are only ever shown
                or hidden from then on, never created or destroyed, because the
                web bridge translates structure once and would never see a
                rebuild.
*/
DateField::DateField(
    Modifier modifier,
    DateFieldOptions options,
    const std::string& name
) : Row(modifier, surfaceOptionsFrom(options), {}, name),
    m_dfOptions(std::move(options))
{
    const DateFieldOptions& o = m_dfOptions;

    /* The popup ----------------------------------------------------- Built
       from the caller's picker options with only the parts the field has to. */
    DatePickerOptions pickerOpts = o.getPickerOptions();

    auto userSelected = pickerOpts.getOnDateSelected();
    auto userRange    = pickerOpts.getOnRangeSelected();
    auto userClosed   = pickerOpts.getOnClosed();

    pickerOpts
        .setCloseOnSelect(o.getCloseOnSelect())
        .setOnDateSelected([this, userSelected](const Date& d) {
            handlePicked(d);
            if (userSelected) userSelected(d);
        })
        .setOnRangeSelected([this, userRange](const Date& a, const Date& b) {
            handleRangePicked(a, b);
            if (userRange) userRange(a, b);
        })
        .setOnClosed([this, userClosed]() {
            if (m_dfOptions.getOnClosed()) m_dfOptions.getOnClosed()();
            if (userClosed) userClosed();
        });

    m_picker = new DatePicker(Modifier(), pickerOpts);

    /* The click target ----------------------------------------------
       Installed here rather than later because the web bridge only connects a. */
    m_modifier
        .setOnLeftClick([this](Element*) { open(); })
        .setOnHoverEnter([this](Row* r) {
            r->getOptions()
                .setColor(m_dfOptions.getHoverColor())
                .setColorRole(m_dfOptions.getHoverColorRole());
        })
        .setOnHoverExit([this](Row* r) {
            r->getOptions()
                .setColor(m_dfOptions.getBackgroundColor())
                .setColorRole(m_dfOptions.getBackgroundColorRole());
        });

    /* Contents ------------------------------------------------------ Every
       part is built once and kept. */
    m_padLeft    = new Spacer(Modifier().setWidth(Dimension{o.getPadding(), false}));
    m_centerLeft = new Spacer(Modifier().setWidth(100_pct));

    IconOptions iconOpts = IconOptions()
        .setIcon(o.getIcon())
        .setColor(o.getIconColor())
        .setColorRole(o.getIconColorRole());
    if (o.hasIconStrokeWidth()) iconOpts.setStrokeWidth(o.getIconStrokeWidth());

    m_icon = new Icon(
        Modifier()
            .setWidth(Dimension{o.getIconSize(), false})
            .setHeight(Dimension{o.getIconSize(), false})
            .setAlign(Align::CenterY),
        iconOpts);

    m_iconGap = new Spacer(Modifier().setWidth(Dimension{o.getIconSpacing(), false}));

    m_label = new Text(
        Modifier().setAlign(Align::Left | Align::CenterY),
        TextOptions()
            .setFont(o.getFontPath())
            .setContent(o.getPlaceholder())
            .setCharSize(o.getCharSize())
            .setColor(o.getPlaceholderColor())
            .setColorRole(o.getPlaceholderColorRole())
            .setTextAlignX(Align::Left)
            .setTextAlignY(Align::CenterY));

    m_flexRight = new Spacer(Modifier().setWidth(100_pct));

    IconOptions chevronOpts = IconOptions()
        .setIcon(o.getChevronIcon())
        .setColor(o.getChevronColor())
        .setColorRole(o.getChevronColorRole());
    if (o.hasIconStrokeWidth()) chevronOpts.setStrokeWidth(o.getIconStrokeWidth());

    m_chevron = new Icon(
        Modifier()
            .setWidth(Dimension{o.getChevronSize(), false})
            .setHeight(Dimension{o.getChevronSize(), false})
            .setAlign(Align::CenterY),
        chevronOpts);

    m_padRight = new Spacer(Modifier().setWidth(Dimension{o.getPadding(), false}));

    /* Order matters: this is the row left to right. */
    addElement(m_padLeft);
    addElement(m_centerLeft);
    addElement(m_icon);
    addElement(m_iconGap);
    addElement(m_label);
    addElement(m_flexRight);
    addElement(m_chevron);
    addElement(m_padRight);

    if (o.getInitialDate()) {
        m_date = *o.getInitialDate();
        m_picker->setSelectedDate(*m_date);
    }
    refreshLabel();

    /* Resolved from the declared dimensions at this point, since nothing has
       been laid out yet. */
    applyLayout(resolveLayout());
}


/*
    setOptions(const DateFieldOptions& opts):
    - Params:   const DateFieldOptions& opts
    - Returns:  void
    - Desc:     Swaps in a new configuration and re-applies everything derived
                from it. The popup is left alone -- it was built from the
                options given at construction, and getPicker() is how to change
                it afterwards.
*/
void DateField::setOptions(const DateFieldOptions& opts) {
    m_dfOptions = opts;
    applySurface();
    refreshLabel();
    m_layoutApplied = false;
    applyLayout(resolveLayout());
}


/*
    applySurface():
    - Params:   none
    - Returns:  void
    - Desc:     Pushes the background and rounding down onto the Row, and the
                metrics onto the parts that carry them.
*/
void DateField::applySurface() {
    const DateFieldOptions& o = m_dfOptions;

    Row::setOptions(surfaceOptionsFrom(o));

    m_padLeft->getModifier().setWidth(Dimension{o.getPadding(), false});
    m_padRight->getModifier().setWidth(Dimension{o.getPadding(), false});
    m_iconGap->getModifier().setWidth(Dimension{o.getIconSpacing(), false});

    m_icon->getModifier()
        .setWidth(Dimension{o.getIconSize(), false})
        .setHeight(Dimension{o.getIconSize(), false});
    m_icon->getOptions()
        .setIcon(o.getIcon())
        .setColor(o.getIconColor())
        .setColorRole(o.getIconColorRole());

    m_chevron->getModifier()
        .setWidth(Dimension{o.getChevronSize(), false})
        .setHeight(Dimension{o.getChevronSize(), false});
    m_chevron->getOptions()
        .setIcon(o.getChevronIcon())
        .setColor(o.getChevronColor())
        .setColorRole(o.getChevronColorRole());

    m_label->getOptions()
        .setFont(o.getFontPath())
        .setCharSize(o.getCharSize());

    m_dirty = true;
}


/*
    update(Rectf& parentBounds, float dt):
    - Params:   Rectf& parentBounds, float dt
    - Returns:  void
    - Desc:     Resolves the field's own box first so the layout decision is
                made against this frame's size rather than last frame's, then
                lays out as a Row. Native only: the web bridge does not tick,
                and takes the decision made in the constructor.
*/
void DateField::update(Rectf& parentBounds, float dt) {
    resize(parentBounds);
    applyLayout(resolveLayout());
    Row::update(parentBounds, dt);
}


/*
    resolveAspect():
    - Params:   none
    - Returns:  float
    - Desc:     The field's width over its height. Prefers the laid-out box,
                which is exact and follows percent sizing, and falls back to the
                declared width and height when there is none -- either before
                the first frame or under the web bridge, which never lays the
                tree out. Returns 0 when the field is sized in percentages and
                has not been laid out, since nothing available can say.
*/
float DateField::resolveAspect() const {
    if (m_bounds.size.x > 0.f && m_bounds.size.y > 0.f)
        return m_bounds.size.x / m_bounds.size.y;

    const Dimension w = m_modifier.getWidth();
    const Dimension h = m_modifier.getHeight();
    if (!w.percent && !h.percent && w.value > 0.f && h.value > 0.f)
        return w.value / h.value;

    return 0.f;
}


/*
    resolveLayout():
    - Params:   none
    - Returns:  DateFieldLayout
    - Desc:     Turns Auto into a concrete mode. Wide enough for the text and it
                keeps the label; squarer than the threshold and it drops to the
                icon. An unknown aspect resolves to the labelled form, which
                degrades better -- a too-narrow label is clipped, where a
                needlessly bare icon hides the value outright.
*/
DateFieldLayout DateField::resolveLayout() const {
    const DateFieldOptions& o = m_dfOptions;
    if (o.getLayout() != DateFieldLayout::Auto) return o.getLayout();

    /* With no icon to fall back to there is nothing for a narrow field to
       shrink into, so the text stays whatever shape the field is. */
    if (!o.getShowIcon()) return DateFieldLayout::LabelOnly;

    const float aspect = resolveAspect();
    if (aspect <= 0.f) return DateFieldLayout::IconAndLabel;

    return aspect >= o.getLabelAspectThreshold() ? DateFieldLayout::IconAndLabel
                                                 : DateFieldLayout::IconOnly;
}


/*
    applyLayout(DateFieldLayout layout):
    - Params:   DateFieldLayout layout
    - Returns:  void
    - Desc:     Shows the parts the mode calls for and hides the rest. Nothing
                is created or destroyed: visibility is a style the web bridge
                re-applies on every sync, where a change to the child list would
                never be seen. Returns early when the mode has not changed, so
                this costs a comparison per frame.
*/
void DateField::applyLayout(DateFieldLayout layout) {
    if (m_layoutApplied && layout == m_appliedLayout) return;
    m_appliedLayout = layout;
    m_layoutApplied = true;

    const DateFieldOptions& o = m_dfOptions;
    const bool showIcon  = o.getShowIcon() && layout != DateFieldLayout::LabelOnly;
    const bool showLabel = layout != DateFieldLayout::IconOnly;
    const bool showChev  = o.getShowChevron() && showLabel;

    /* Icon alone is centred, by giving it a percent spacer on both sides. */
    const bool iconAlone = showIcon && !showLabel;

    m_padLeft->getModifier().setVisible(!iconAlone);
    m_centerLeft->getModifier().setVisible(iconAlone);
    m_icon->getModifier().setVisible(showIcon);
    m_iconGap->getModifier().setVisible(showIcon && showLabel);
    m_label->getModifier().setVisible(showLabel);
    m_flexRight->getModifier().setVisible(true);
    m_chevron->getModifier().setVisible(showChev);
    m_padRight->getModifier().setVisible(!iconAlone);

    m_dirty = true;
}


/* Value */

/*
    getDisplayText():
    - Params:   none
    - Returns:  std::string
    - Desc:     What the label reads: the formatted value, both ends joined when
                a range is set, or the placeholder while the field is empty.
*/
std::string DateField::getDisplayText() const {
    const DateFieldOptions& o = m_dfOptions;
    if (!m_date) return o.getPlaceholder();

    if (m_rangeEnd) {
        return DT::format(*m_date, o.getRangeFormat())
             + o.getRangeSeparator()
             + DT::format(*m_rangeEnd, o.getRangeFormat());
    }
    return DT::format(*m_date, o.getFormat());
}


/*
    refreshLabel():
    - Params:   none
    - Returns:  void
    - Desc:     Writes the current text into the label and colours it as a value
                or as a placeholder. Both are ordinary property changes, so this
                reaches the web through the bridge's usual sync.
*/
void DateField::refreshLabel() {
    const DateFieldOptions& o = m_dfOptions;

    m_label->setString(getDisplayText());
    if (m_date) {
        m_label->getOptions().setColor(o.getTextColor()).setColorRole(o.getTextColorRole());
    } else {
        m_label->getOptions()
            .setColor(o.getPlaceholderColor())
            .setColorRole(o.getPlaceholderColorRole());
    }
    m_dirty = true;
}


/*
    setDate(const Date& date):
    - Params:   const Date& date
    - Returns:  void
    - Desc:     Sets the value and pages the popup to it. Fires no callback:
                this is the application writing the field, not the user picking.
*/
void DateField::setDate(const Date& date) {
    if (!DT::isValid(date)) return;

    m_date = date;
    m_rangeEnd.reset();
    if (m_picker) m_picker->setSelectedDate(date);
    refreshLabel();
}


/*
    setRange(const Date& first, const Date& last):
    - Params:   const Date& first, const Date& last
    - Returns:  void
    - Desc:     Sets both ends of a range, in either order. Silent, as
                setDate() is.
*/
void DateField::setRange(const Date& first, const Date& last) {
    if (!DT::isValid(first) || !DT::isValid(last)) return;

    Date lo = first, hi = last;
    DT::order(lo, hi);

    m_date     = lo;
    m_rangeEnd = hi;
    if (m_picker) m_picker->setRange(lo, hi);
    refreshLabel();
}


/*
    clear():
    - Params:   none
    - Returns:  void
    - Desc:     Empties the field, returning the label to the placeholder, and
                fires onCleared.
*/
void DateField::clear() {
    const bool had = m_date.has_value();

    m_date.reset();
    m_rangeEnd.reset();
    if (m_picker) m_picker->clearSelection();
    refreshLabel();

    if (had && m_dfOptions.getOnCleared()) m_dfOptions.getOnCleared()();
}


/*
    handlePicked(const Date& date):
    - Params:   const Date& date
    - Returns:  void
    - Desc:     A date came back from the popup. In Range mode this is the first
                of the two clicks, so the end is dropped until the span closes
                and the label shows the start on its own meanwhile.
*/
void DateField::handlePicked(const Date& date) {
    const bool ranged = m_picker
                     && m_picker->getOptions().getMode() == DatePickerMode::Range;

    m_date = date;
    if (ranged) m_rangeEnd.reset();

    refreshLabel();
    if (m_dfOptions.getOnDateChanged()) m_dfOptions.getOnDateChanged()(date);
}


/*
    handleRangePicked(const Date& first, const Date& last):
    - Params:   const Date& first, const Date& last
    - Returns:  void
    - Desc:     Both ends of a range arrived; the label switches to the joined
                form and onRangeChanged fires.
*/
void DateField::handleRangePicked(const Date& first, const Date& last) {
    m_date     = first;
    m_rangeEnd = last;

    refreshLabel();
    if (m_dfOptions.getOnRangeChanged()) m_dfOptions.getOnRangeChanged()(first, last);
}


/* Popup */

/*
    open():
    - Params:   none
    - Returns:  void
    - Desc:     Opens the calendar over the window, through the UILO the field
                was bound to when it became part of a page. No-op before then,
                since there is nothing to present on.
*/
void DateField::open() {
    if (!m_picker || !m_uiloRef) return;

    m_picker->open(*m_uiloRef);
    if (m_dfOptions.getOnOpened()) m_dfOptions.getOnOpened()();
}


/*
    close():
    - Params:   none
    - Returns:  void
    - Desc:     Dismisses the calendar. onClosed fires from the picker, so it
                reports a dismissal however it came about.
*/
void DateField::close() {
    if (m_picker) m_picker->close();
}


/*
    isOpen():
    - Params:   none
    - Returns:  bool
    - Desc:     Whether the calendar is currently up.
*/
bool DateField::isOpen() const {
    return m_picker && m_picker->isOpen();
}

} // namespace uilo
