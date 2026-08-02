#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "../containers/Column.hpp"
#include "../containers/Row.hpp"
#include "../decoration/Icon.hpp"
#include "../decoration/Spacer.hpp"
#include "../decoration/Text.hpp"
#include "../interactible/Button.hpp"
#include "../../utils/DateAndTime.hpp"
#include "../../utils/Theme.hpp"

namespace uilo {

class UILO;

/*
    DatePickerMode:
    - Desc:     Single takes one date at a time, so every click replaces the
                selection. Range brackets a span with two clicks -- the first
                starts it, the second closes it, and a third begins a new one --
                and while a range is half made the cells under the cursor
                preview where it would land.
*/
enum class DatePickerMode { Single, Range };


/*
    WeekdayLabelStyle:
    - Desc:     How the column headings above the grid are written. Initial
                ("S") suits a narrow picker, Short ("Sun") a wide one.
*/
enum class WeekdayLabelStyle { Initial, Short, Full };


/*
    DatePickerOptions:
    - Desc:     Everything the widget draws, in the same shape as the rest of
                the library: literal + role colour pairs where the role wins if
                it resolves against the active Palette, and plain pixel metrics
                in virtual units.
    - Sizes are all "before scale": UILO multiplies by getScale() at layout
      time, so a picker configured once is the right physical size on any
      display.
*/
class DatePickerOptions {
public:
    DatePickerOptions() = default;

    // Behaviour ---------------------------------------------------------
    DatePickerOptions& setMode(DatePickerMode m)          { m_mode = m; return *this; }
    DatePickerOptions& setFirstDayOfWeek(Weekday d)       { m_firstDayOfWeek = d; return *this; }
    // Days from the neighbouring months that fill out the first and last
    // week. Off leaves those cells blank rather than reflowing the grid.
    DatePickerOptions& setShowAdjacentMonths(bool v)      { m_showAdjacent = v; return *this; }
    // Clicking a greyed neighbour-month day. On (the default) it selects the
    // day and pages the view to its month.
    DatePickerOptions& setSelectAdjacentMonths(bool v)    { m_selectAdjacent = v; return *this; }
    // Always draw six week rows. On by default so the card keeps one height
    // as you page through months instead of jumping by a row.
    DatePickerOptions& setFixedWeekRows(bool v)           { m_fixedWeekRows = v; return *this; }
    // Earliest / latest selectable day. Days outside the range are drawn
    // disabled and ignore clicks; navigation still stops at their month.
    DatePickerOptions& setMinDate(const Date& d)          { m_minDate = d; return *this; }
    DatePickerOptions& setMaxDate(const Date& d)          { m_maxDate = d; return *this; }
    DatePickerOptions& clearMinDate()                     { m_minDate.reset(); return *this; }
    DatePickerOptions& clearMaxDate()                     { m_maxDate.reset(); return *this; }
    // Return false to grey out and lock an individual day -- weekends,
    // holidays, dates already taken. Called for every visible cell on every
    // rebuild, so keep it cheap.
    DatePickerOptions& setDateEnabled(std::function<bool(const Date&)> f) { m_dateEnabled = std::move(f); return *this; }
    // Dismiss the popup as soon as a date is picked. In Range mode this waits
    // for the second click. Ignored when the picker is embedded rather than
    // opened as a popup.
    DatePickerOptions& setCloseOnSelect(bool v)           { m_closeOnSelect = v; return *this; }

    // Popup presentation ------------------------------------------------
    // Card size. Height 0 (the default) derives it from the metrics below,
    // which is almost always what you want.
    DatePickerOptions& setPopupWidth(float px)            { m_popupWidth = px; return *this; }
    DatePickerOptions& setPopupHeight(float px)           { m_popupHeight = px; return *this; }
    // The dimmed sheet behind a centred popup. Alpha 0 leaves the page
    // visible and unblocked-looking, though it still swallows clicks.
    DatePickerOptions& setScrimColor(const Color& c)          { m_scrimColor = c; return *this; }
    DatePickerOptions& setScrimColorRole(const std::string& r){ m_scrimColorRole = r; return *this; }
    DatePickerOptions& setDismissOnScrimClick(bool v)     { m_dismissOnScrim = v; return *this; }

    // Card --------------------------------------------------------------
    DatePickerOptions& setBackgroundColor(const Color& c)          { m_bgColor = c; return *this; }
    DatePickerOptions& setBackgroundColorRole(const std::string& r){ m_bgColorRole = r; return *this; }
    DatePickerOptions& setRounding(float r)               { m_rounding = r; return *this; }
    DatePickerOptions& setPadding(float px)               { m_padding = px; return *this; }

    // Outline -------------------------------------------------------------
    // A border around the card, drawn inside its bounds. Follows
    // Theme::setOutlineThickness()/setOutlineColorRole() when left unset.
    DatePickerOptions& setOutlineColor(const Color& c)           { m_outlineColor = c; return *this; }
    DatePickerOptions& setOutlineColorRole(const std::string& r) { m_outlineColorRole = r; return *this; }
    DatePickerOptions& setOutlineThickness(float px)             { m_outlineThickness = px; return *this; }
    DatePickerOptions& setFont(const std::string& path)   { m_fontPath = path; return *this; }

    // Header (month title + navigation) ---------------------------------
    DatePickerOptions& setShowHeader(bool v)              { m_showHeader = v; return *this; }
    DatePickerOptions& setHeaderHeight(float px)          { m_headerHeight = px; return *this; }
    DatePickerOptions& setHeaderSpacing(float px)         { m_headerSpacing = px; return *this; }
    // A DateAndTime pattern against the first of the shown month:
    // "MMMM YYYY" -> "July 2026", "MMM YY" -> "Jul 26".
    DatePickerOptions& setTitleFormat(const std::string& s) { m_titleFormat = s; return *this; }
    DatePickerOptions& setHeaderCharSize(unsigned int n)  { m_headerCharSize = n; return *this; }
    DatePickerOptions& setHeaderBold(bool v)              { m_headerBold = v; return *this; }
    DatePickerOptions& setHeaderTextColor(const Color& c)          { m_headerTextColor = c; return *this; }
    DatePickerOptions& setHeaderTextColorRole(const std::string& r){ m_headerTextColorRole = r; return *this; }
    // Clicking the title jumps back to the month holding today.
    DatePickerOptions& setTitleClickGoesToToday(bool v)   { m_titleGoesToToday = v; return *this; }

    // A second pair of arrows that step a whole year.
    DatePickerOptions& setShowYearNavigation(bool v)      { m_showYearNav = v; return *this; }
    DatePickerOptions& setNavButtonSize(float px)         { m_navButtonSize = px; return *this; }
    DatePickerOptions& setNavIconSize(float px)           { m_navIconSize = px; return *this; }
    DatePickerOptions& setNavRounding(float r)            { m_navRounding = r; return *this; }
    DatePickerOptions& setPrevMonthIcon(std::string_view n) { m_prevIcon = std::string(n); return *this; }
    DatePickerOptions& setNextMonthIcon(std::string_view n) { m_nextIcon = std::string(n); return *this; }
    DatePickerOptions& setPrevYearIcon(std::string_view n)  { m_prevYearIcon = std::string(n); return *this; }
    DatePickerOptions& setNextYearIcon(std::string_view n)  { m_nextYearIcon = std::string(n); return *this; }
    DatePickerOptions& setNavIconStrokeWidth(float w)     { m_navStroke = w; m_hasNavStroke = true; return *this; }
    DatePickerOptions& setNavColor(const Color& c)              { m_navColor = c; return *this; }
    DatePickerOptions& setNavColorRole(const std::string& r)    { m_navColorRole = r; return *this; }
    DatePickerOptions& setNavHoverColor(const Color& c)         { m_navHoverColor = c; return *this; }
    DatePickerOptions& setNavHoverColorRole(const std::string& r){ m_navHoverColorRole = r; return *this; }
    DatePickerOptions& setNavIconColor(const Color& c)          { m_navIconColor = c; return *this; }
    DatePickerOptions& setNavIconColorRole(const std::string& r){ m_navIconColorRole = r; return *this; }

    // Weekday headings --------------------------------------------------
    DatePickerOptions& setShowWeekdayHeader(bool v)       { m_showWeekdayHeader = v; return *this; }
    DatePickerOptions& setWeekdayHeight(float px)         { m_weekdayHeight = px; return *this; }
    DatePickerOptions& setWeekdayLabelStyle(WeekdayLabelStyle s) { m_weekdayStyle = s; return *this; }
    DatePickerOptions& setWeekdayCharSize(unsigned int n) { m_weekdayCharSize = n; return *this; }
    DatePickerOptions& setWeekdayBold(bool v)             { m_weekdayBold = v; return *this; }
    DatePickerOptions& setWeekdayColor(const Color& c)          { m_weekdayColor = c; return *this; }
    DatePickerOptions& setWeekdayColorRole(const std::string& r){ m_weekdayColorRole = r; return *this; }

    // Day cells ---------------------------------------------------------
    DatePickerOptions& setCellHeight(float px)            { m_cellHeight = px; return *this; }
    // Inset applied inside each cell's slot, which is what puts air between
    // neighbouring days without disturbing the seven-column split.
    DatePickerOptions& setCellSpacing(float px)           { m_cellSpacing = px; return *this; }
    DatePickerOptions& setCellRounding(float r)           { m_cellRounding = r; return *this; }
    DatePickerOptions& setCharSize(unsigned int n)        { m_charSize = n; return *this; }
    DatePickerOptions& setCellColor(const Color& c)             { m_cellColor = c; return *this; }
    DatePickerOptions& setCellColorRole(const std::string& r)   { m_cellColorRole = r; return *this; }
    DatePickerOptions& setCellHoverColor(const Color& c)        { m_cellHoverColor = c; return *this; }
    DatePickerOptions& setCellHoverColorRole(const std::string& r) { m_cellHoverColorRole = r; return *this; }
    DatePickerOptions& setDayTextColor(const Color& c)          { m_dayTextColor = c; return *this; }
    DatePickerOptions& setDayTextColorRole(const std::string& r){ m_dayTextColorRole = r; return *this; }
    // Neighbour-month days, when shown.
    DatePickerOptions& setAdjacentTextColor(const Color& c)          { m_adjacentTextColor = c; return *this; }
    DatePickerOptions& setAdjacentTextColorRole(const std::string& r){ m_adjacentTextColorRole = r; return *this; }
    // Left unset, weekends read the same as any other day.
    DatePickerOptions& setWeekendTextColor(const Color& c)           { m_weekendTextColor = c; m_hasWeekendColor = true; return *this; }
    DatePickerOptions& setWeekendTextColorRole(const std::string& r) { m_weekendTextColorRole = r; m_hasWeekendColor = true; return *this; }
    DatePickerOptions& setDisabledTextColor(const Color& c)          { m_disabledTextColor = c; return *this; }
    DatePickerOptions& setDisabledTextColorRole(const std::string& r){ m_disabledTextColorRole = r; return *this; }
    DatePickerOptions& setSelectedColor(const Color& c)              { m_selectedColor = c; return *this; }
    DatePickerOptions& setSelectedColorRole(const std::string& r)    { m_selectedColorRole = r; return *this; }
    DatePickerOptions& setSelectedTextColor(const Color& c)          { m_selectedTextColor = c; return *this; }
    DatePickerOptions& setSelectedTextColorRole(const std::string& r){ m_selectedTextColorRole = r; return *this; }
    // The days between the two ends of a range.
    DatePickerOptions& setRangeFillColor(const Color& c)             { m_rangeFillColor = c; return *this; }
    DatePickerOptions& setRangeFillColorRole(const std::string& r)   { m_rangeFillColorRole = r; return *this; }
    DatePickerOptions& setRangeTextColor(const Color& c)             { m_rangeTextColor = c; return *this; }
    DatePickerOptions& setRangeTextColorRole(const std::string& r)   { m_rangeTextColorRole = r; return *this; }
    // Today gets its own fill so it stays findable while unselected. Alpha 0
    // turns the marker off.
    DatePickerOptions& setTodayColor(const Color& c)                { m_todayColor = c; return *this; }
    DatePickerOptions& setTodayColorRole(const std::string& r)      { m_todayColorRole = r; return *this; }
    DatePickerOptions& setTodayTextColor(const Color& c)            { m_todayTextColor = c; m_hasTodayTextColor = true; return *this; }
    DatePickerOptions& setTodayTextColorRole(const std::string& r)  { m_todayTextColorRole = r; m_hasTodayTextColor = true; return *this; }
    DatePickerOptions& setBoldToday(bool v)               { m_boldToday = v; return *this; }
    DatePickerOptions& setBoldSelected(bool v)            { m_boldSelected = v; return *this; }

    // Footer ------------------------------------------------------------
    // Any label left empty hides that button; all four empty hides the row
    // entirely, and nothing is reserved for it.
    DatePickerOptions& setShowFooter(bool v)              { m_showFooter = v; return *this; }
    // Height of the button row itself.
    DatePickerOptions& setFooterHeight(float px)          { m_footerHeight = px; return *this; }
    // Breathing room between the last week and the buttons. It sets how much
    // room is kept below the grid rather than being a gap on one side: the
    // buttons stay centred in whatever space ends up there, so raising this
    // moves them down and away from the grid together.
    DatePickerOptions& setFooterSpacing(float px)         { m_footerSpacing = px; return *this; }
    DatePickerOptions& setTodayButtonLabel(const std::string& s)   { m_todayLabel = s; return *this; }
    DatePickerOptions& setClearButtonLabel(const std::string& s)   { m_clearLabel = s; return *this; }
    DatePickerOptions& setCancelButtonLabel(const std::string& s)  { m_cancelLabel = s; return *this; }
    DatePickerOptions& setConfirmButtonLabel(const std::string& s) { m_confirmLabel = s; return *this; }
    DatePickerOptions& setFooterCharSize(unsigned int n)  { m_footerCharSize = n; return *this; }
    DatePickerOptions& setButtonWidth(float px)           { m_buttonWidth = px; return *this; }
    DatePickerOptions& setButtonRounding(float r)         { m_buttonRounding = r; return *this; }
    DatePickerOptions& setButtonColor(const Color& c)           { m_buttonColor = c; return *this; }
    DatePickerOptions& setButtonColorRole(const std::string& r) { m_buttonColorRole = r; return *this; }
    DatePickerOptions& setButtonTextColor(const Color& c)           { m_buttonTextColor = c; return *this; }
    DatePickerOptions& setButtonTextColorRole(const std::string& r) { m_buttonTextColorRole = r; return *this; }
    // The confirm button, styled apart from the rest as the primary action.
    DatePickerOptions& setConfirmButtonColor(const Color& c)           { m_confirmColor = c; return *this; }
    DatePickerOptions& setConfirmButtonColorRole(const std::string& r) { m_confirmColorRole = r; return *this; }
    DatePickerOptions& setConfirmButtonTextColor(const Color& c)           { m_confirmTextColor = c; return *this; }
    DatePickerOptions& setConfirmButtonTextColorRole(const std::string& r) { m_confirmTextColorRole = r; return *this; }

    // Callbacks ---------------------------------------------------------
    using DateCallback  = std::function<void(const Date&)>;
    using RangeCallback = std::function<void(const Date&, const Date&)>;
    using MonthCallback = std::function<void(int, unsigned)>;
    using VoidCallback  = std::function<void()>;

    // Every click that lands on a selectable day, Single mode or not.
    DatePickerOptions& setOnDateSelected(DateCallback f)  { m_onDateSelected = std::move(f); return *this; }
    // Both ends of a range, ordered earliest first, once the second is set.
    DatePickerOptions& setOnRangeSelected(RangeCallback f) { m_onRangeSelected = std::move(f); return *this; }
    DatePickerOptions& setOnMonthChanged(MonthCallback f)  { m_onMonthChanged = std::move(f); return *this; }
    DatePickerOptions& setOnCleared(VoidCallback f)        { m_onCleared = std::move(f); return *this; }
    // The confirm button. Fires with the current selection; in Range mode the
    // start is passed and getRangeEnd() carries the rest.
    DatePickerOptions& setOnConfirmed(DateCallback f)      { m_onConfirmed = std::move(f); return *this; }
    DatePickerOptions& setOnCancelled(VoidCallback f)      { m_onCancelled = std::move(f); return *this; }
    // Any dismissal: cancel, confirm, scrim click, or close().
    DatePickerOptions& setOnClosed(VoidCallback f)         { m_onClosed = std::move(f); return *this; }

    // Getters -----------------------------------------------------------
    DatePickerMode getMode()            const { return m_mode; }
    Weekday getFirstDayOfWeek()         const { return m_firstDayOfWeek; }
    bool getShowAdjacentMonths()        const { return m_showAdjacent; }
    bool getSelectAdjacentMonths()      const { return m_selectAdjacent; }
    bool getFixedWeekRows()             const { return m_fixedWeekRows; }
    const std::optional<Date>& getMinDate() const { return m_minDate; }
    const std::optional<Date>& getMaxDate() const { return m_maxDate; }
    const std::function<bool(const Date&)>& getDateEnabled() const { return m_dateEnabled; }
    bool getCloseOnSelect()             const { return m_closeOnSelect; }

    float getPopupWidth()               const { return m_popupWidth; }
    float getPopupHeight()              const { return m_popupHeight; }
    Color getScrimColor()               const { return m_scrimColor; }
    const std::string& getScrimColorRole() const { return m_scrimColorRole; }
    bool getDismissOnScrimClick()       const { return m_dismissOnScrim; }

    Color getBackgroundColor()          const { return m_bgColor; }
    const std::string& getBackgroundColorRole() const { return m_bgColorRole; }
    float getRounding()                 const;
    float getPadding()                  const { return m_padding; }
    Color              getOutlineColor()     const { return m_outlineColor; }
    const std::string& getOutlineColorRole() const { return m_outlineColorRole; }
    float              getOutlineThickness() const { return m_outlineThickness; }
    const std::string& getFontPath()    const;

    bool getShowHeader()                const { return m_showHeader; }
    float getHeaderHeight()             const { return m_headerHeight; }
    float getHeaderSpacing()            const { return m_headerSpacing; }
    const std::string& getTitleFormat() const { return m_titleFormat; }
    unsigned int getHeaderCharSize()    const { return m_headerCharSize; }
    bool getHeaderBold()                const { return m_headerBold; }
    Color getHeaderTextColor()          const { return m_headerTextColor; }
    const std::string& getHeaderTextColorRole() const { return m_headerTextColorRole; }
    bool getTitleClickGoesToToday()     const { return m_titleGoesToToday; }

    bool getShowYearNavigation()        const { return m_showYearNav; }
    float getNavButtonSize()            const { return m_navButtonSize; }
    float getNavIconSize()              const { return m_navIconSize; }
    float getNavRounding()              const;
    const std::string& getPrevMonthIcon() const { return m_prevIcon; }
    const std::string& getNextMonthIcon() const { return m_nextIcon; }
    const std::string& getPrevYearIcon()  const { return m_prevYearIcon; }
    const std::string& getNextYearIcon()  const { return m_nextYearIcon; }
    float getNavIconStrokeWidth()       const { return m_navStroke; }
    bool hasNavIconStrokeWidth()        const { return m_hasNavStroke; }
    Color getNavColor()                 const { return m_navColor; }
    const std::string& getNavColorRole() const { return m_navColorRole; }
    Color getNavHoverColor()            const { return m_navHoverColor; }
    const std::string& getNavHoverColorRole() const { return m_navHoverColorRole; }
    Color getNavIconColor()             const { return m_navIconColor; }
    const std::string& getNavIconColorRole() const { return m_navIconColorRole; }

    bool getShowWeekdayHeader()         const { return m_showWeekdayHeader; }
    float getWeekdayHeight()            const { return m_weekdayHeight; }
    WeekdayLabelStyle getWeekdayLabelStyle() const { return m_weekdayStyle; }
    unsigned int getWeekdayCharSize()   const { return m_weekdayCharSize; }
    bool getWeekdayBold()               const { return m_weekdayBold; }
    Color getWeekdayColor()             const { return m_weekdayColor; }
    const std::string& getWeekdayColorRole() const { return m_weekdayColorRole; }

    float getCellHeight()               const { return m_cellHeight; }
    float getCellSpacing()              const { return m_cellSpacing; }
    float getCellRounding()             const;
    unsigned int getCharSize()          const { return m_charSize; }
    Color getCellColor()                const { return m_cellColor; }
    const std::string& getCellColorRole() const { return m_cellColorRole; }
    Color getCellHoverColor()           const { return m_cellHoverColor; }
    const std::string& getCellHoverColorRole() const { return m_cellHoverColorRole; }
    Color getDayTextColor()             const { return m_dayTextColor; }
    const std::string& getDayTextColorRole() const { return m_dayTextColorRole; }
    Color getAdjacentTextColor()        const { return m_adjacentTextColor; }
    const std::string& getAdjacentTextColorRole() const { return m_adjacentTextColorRole; }
    Color getWeekendTextColor()         const { return m_weekendTextColor; }
    const std::string& getWeekendTextColorRole() const { return m_weekendTextColorRole; }
    bool hasWeekendTextColor()          const { return m_hasWeekendColor; }
    Color getDisabledTextColor()        const { return m_disabledTextColor; }
    const std::string& getDisabledTextColorRole() const { return m_disabledTextColorRole; }
    Color getSelectedColor()            const { return m_selectedColor; }
    const std::string& getSelectedColorRole() const { return m_selectedColorRole; }
    Color getSelectedTextColor()        const { return m_selectedTextColor; }
    const std::string& getSelectedTextColorRole() const { return m_selectedTextColorRole; }
    Color getRangeFillColor()           const { return m_rangeFillColor; }
    const std::string& getRangeFillColorRole() const { return m_rangeFillColorRole; }
    Color getRangeTextColor()           const { return m_rangeTextColor; }
    const std::string& getRangeTextColorRole() const { return m_rangeTextColorRole; }
    Color getTodayColor()               const { return m_todayColor; }
    const std::string& getTodayColorRole() const { return m_todayColorRole; }
    Color getTodayTextColor()           const { return m_todayTextColor; }
    const std::string& getTodayTextColorRole() const { return m_todayTextColorRole; }
    bool hasTodayTextColor()            const { return m_hasTodayTextColor; }
    bool getBoldToday()                 const { return m_boldToday; }
    bool getBoldSelected()              const { return m_boldSelected; }

    bool getShowFooter()                const { return m_showFooter; }
    float getFooterHeight()             const { return m_footerHeight; }
    float getFooterSpacing()            const { return m_footerSpacing; }
    const std::string& getTodayButtonLabel()   const { return m_todayLabel; }
    const std::string& getClearButtonLabel()   const { return m_clearLabel; }
    const std::string& getCancelButtonLabel()  const { return m_cancelLabel; }
    const std::string& getConfirmButtonLabel() const { return m_confirmLabel; }
    unsigned int getFooterCharSize()    const { return m_footerCharSize; }
    float getButtonWidth()              const { return m_buttonWidth; }
    float getButtonRounding()           const;
    Color getButtonColor()              const { return m_buttonColor; }
    const std::string& getButtonColorRole() const { return m_buttonColorRole; }
    Color getButtonTextColor()          const { return m_buttonTextColor; }
    const std::string& getButtonTextColorRole() const { return m_buttonTextColorRole; }
    Color getConfirmButtonColor()       const { return m_confirmColor; }
    const std::string& getConfirmButtonColorRole() const { return m_confirmColorRole; }
    Color getConfirmButtonTextColor()   const { return m_confirmTextColor; }
    const std::string& getConfirmButtonTextColorRole() const { return m_confirmTextColorRole; }

    const DateCallback&  getOnDateSelected()  const { return m_onDateSelected; }
    const RangeCallback& getOnRangeSelected() const { return m_onRangeSelected; }
    const MonthCallback& getOnMonthChanged()  const { return m_onMonthChanged; }
    const VoidCallback&  getOnCleared()       const { return m_onCleared; }
    const DateCallback&  getOnConfirmed()     const { return m_onConfirmed; }
    const VoidCallback&  getOnCancelled()     const { return m_onCancelled; }
    const VoidCallback&  getOnClosed()        const { return m_onClosed; }


    // Raw, for handing to a child element that should keep following
    // the theme rather than being pinned to a resolved number.
    const std::optional<float>& getRoundingOpt() const { return m_rounding; }
    static constexpr float getRoundingOptFallback() { return 10.f; }
    // Raw, for handing to a child element that should keep following
    // the theme rather than being pinned to a resolved number.
    const std::optional<float>& getNavRoundingOpt() const { return m_navRounding; }
    static constexpr float getNavRoundingOptFallback() { return 6.f; }
    // Raw, for handing to a child element that should keep following
    // the theme rather than being pinned to a resolved number.
    const std::optional<float>& getCellRoundingOpt() const { return m_cellRounding; }
    static constexpr float getCellRoundingOptFallback() { return 6.f; }
    // Raw, for handing to a child element that should keep following
    // the theme rather than being pinned to a resolved number.
    const std::optional<float>& getButtonRoundingOpt() const { return m_buttonRounding; }
    static constexpr float getButtonRoundingOptFallback() { return 6.f; }

private:
    DatePickerMode m_mode          = DatePickerMode::Single;
    Weekday m_firstDayOfWeek       = Weekday::Sunday;
    bool m_showAdjacent            = true;
    bool m_selectAdjacent          = true;
    bool m_fixedWeekRows           = true;
    std::optional<Date> m_minDate;
    std::optional<Date> m_maxDate;
    std::function<bool(const Date&)> m_dateEnabled;
    bool m_closeOnSelect           = false;

    float m_popupWidth             = 320.f;
    float m_popupHeight            = 0.f;   /* 0 = derive from the metrics */
    Color m_scrimColor             = Color{0, 0, 0, 140};
    std::string m_scrimColorRole;
    bool m_dismissOnScrim          = true;

    Color m_bgColor                = Color{0, 0, 0, 0};
    std::string m_bgColorRole      = "panel";
    std::optional<float> m_rounding;
    float m_padding                = 12.f;
    Color                m_outlineColor = Color::Transparent;
    std::string          m_outlineColorRole;
    float m_outlineThickness = 0.f;
    std::string m_fontPath;

    bool m_showHeader              = true;
    float m_headerHeight           = 32.f;
    float m_headerSpacing          = 4.f;
    std::string m_titleFormat      = "MMMM YYYY";
    unsigned int m_headerCharSize  = 17;
    bool m_headerBold              = true;
    Color m_headerTextColor        = Color::White;
    std::string m_headerTextColorRole = "text";
    bool m_titleGoesToToday        = true;

    bool m_showYearNav             = false;
    float m_navButtonSize          = 28.f;
    float m_navIconSize            = 16.f;
    std::optional<float> m_navRounding;
    std::string m_prevIcon         = "chevron-left";
    std::string m_nextIcon         = "chevron-right";
    std::string m_prevYearIcon     = "chevrons-left";
    std::string m_nextYearIcon     = "chevrons-right";
    float m_navStroke              = 0.f;
    bool m_hasNavStroke            = false;
    Color m_navColor               = Color{0, 0, 0, 0};
    std::string m_navColorRole;
    Color m_navHoverColor          = Color{255, 255, 255, 24};
    std::string m_navHoverColorRole = "panelAlt";
    Color m_navIconColor           = Color::White;
    std::string m_navIconColorRole = "text";

    bool m_showWeekdayHeader       = true;
    float m_weekdayHeight          = 22.f;
    WeekdayLabelStyle m_weekdayStyle = WeekdayLabelStyle::Initial;
    unsigned int m_weekdayCharSize = 13;
    bool m_weekdayBold             = false;
    Color m_weekdayColor           = Color{255, 255, 255, 130};
    std::string m_weekdayColorRole = "textDim";

    float m_cellHeight             = 32.f;
    float m_cellSpacing            = 2.f;
    std::optional<float> m_cellRounding;
    unsigned int m_charSize        = 15;
    Color m_cellColor              = Color{0, 0, 0, 0};
    std::string m_cellColorRole;
    Color m_cellHoverColor         = Color{255, 255, 255, 24};
    std::string m_cellHoverColorRole = "outline";
    Color m_dayTextColor           = Color::White;
    std::string m_dayTextColorRole = "text";
    Color m_adjacentTextColor      = Color{255, 255, 255, 70};
    std::string m_adjacentTextColorRole = "textDim";
    Color m_weekendTextColor       = Color::White;
    std::string m_weekendTextColorRole;
    bool m_hasWeekendColor         = false;
    Color m_disabledTextColor      = Color{255, 255, 255, 45};
    std::string m_disabledTextColorRole = "textDim";
    Color m_selectedColor          = Color{120, 110, 220, 255};
    std::string m_selectedColorRole = "accent";
    Color m_selectedTextColor      = Color::White;
    std::string m_selectedTextColorRole = "onAccent";
    Color m_rangeFillColor         = Color{120, 110, 220, 70};
    std::string m_rangeFillColorRole;
    Color m_rangeTextColor         = Color::White;
    std::string m_rangeTextColorRole = "text";
    Color m_todayColor             = Color{255, 255, 255, 28};
    std::string m_todayColorRole = "panelAlt";
    Color m_todayTextColor         = Color::White;
    std::string m_todayTextColorRole;
    bool m_hasTodayTextColor       = false;
    bool m_boldToday               = true;
    bool m_boldSelected            = true;

    bool m_showFooter              = true;
    float m_footerHeight           = 30.f;
    float m_footerSpacing          = 8.f;
    std::string m_todayLabel       = "Today";
    std::string m_clearLabel;
    std::string m_cancelLabel      = "Cancel";
    std::string m_confirmLabel     = "OK";
    unsigned int m_footerCharSize  = 14;
    float m_buttonWidth            = 68.f;
    std::optional<float> m_buttonRounding;
    Color m_buttonColor            = Color{255, 255, 255, 20};
    std::string m_buttonColorRole  = "panelAlt";
    Color m_buttonTextColor        = Color::White;
    std::string m_buttonTextColorRole = "text";
    Color m_confirmColor           = Color{120, 110, 220, 255};
    std::string m_confirmColorRole = "accent";
    Color m_confirmTextColor       = Color::White;
    std::string m_confirmTextColorRole = "onAccent";

    DateCallback  m_onDateSelected;
    RangeCallback m_onRangeSelected;
    MonthCallback m_onMonthChanged;
    VoidCallback  m_onCleared;
    DateCallback  m_onConfirmed;
    VoidCallback  m_onCancelled;
    VoidCallback  m_onClosed;
};


/*
    getRounding():
    - Params:   none
    - Returns:  float
    - Desc:     Corner radius of the picker card, resolved in three steps: the
                value this picker was given, then the active Theme's, then 10.
*/
inline float DatePickerOptions::getRounding() const {
    return Theme::resolveRounding(m_rounding, 10.f);
}


/*
    getFontPath():
    - Params:   none
    - Returns:  const std::string&
    - Desc:     Font for every label in the picker, falling back to the active
                Theme's.
*/
inline const std::string& DatePickerOptions::getFontPath() const {
    return Theme::resolveFont(m_fontPath);
}


/*
    getNavRounding():
    - Params:   none
    - Returns:  float
    - Desc:     Corner radius of the month and year navigation buttons, resolved
                the same three ways with a fallback of 6.
*/
inline float DatePickerOptions::getNavRounding() const {
    return Theme::resolveRounding(m_navRounding, 6.f);
}


/*
    getCellRounding():
    - Params:   none
    - Returns:  float
    - Desc:     Corner radius of a day cell, resolved the same three ways with a
                fallback of 6.
*/
inline float DatePickerOptions::getCellRounding() const {
    return Theme::resolveRounding(m_cellRounding, 6.f);
}


/*
    getButtonRounding():
    - Params:   none
    - Returns:  float
    - Desc:     Corner radius of the footer buttons, resolved the same three
                ways with a fallback of 6.
*/
inline float DatePickerOptions::getButtonRounding() const {
    return Theme::resolveRounding(m_buttonRounding, 6.f);
}


/*
    DatePicker:
    - Desc:     A month grid for choosing a date, built on Column. Header with
                month title and arrows, weekday headings, six rows of day cells,
                and an optional button row.
    - There are two ways to use one.

        Embedded -- add it to a layout like any other element and read the
        selection from the callbacks.

            column(Modifier(), {}, contains{
                datepicker(Modifier().setHeight(320_px),
                           DatePickerOptions().setOnDateSelected(...))
            })

        Centred popup -- keep the pointer, and open it when something asks
        for a date. The picker builds its own full-window backdrop, centres
        itself on it, and blocks the page beneath until dismissed.

            auto* picker = datepicker({}, DatePickerOptions()
                .setCloseOnSelect(true)
                .setOnDateSelected([](const Date& d) { ... }));
            ...
            picker->open(ui);

      A popup picker must not also be a child of the page tree; open() makes
      it a child of its own backdrop, and an element cannot be in two places.
    - Paging months rebuilds the grid. Selecting, hovering, and previewing a
      range only recolour the existing cells, so dragging across the grid
      allocates nothing.
*/
class DatePicker : public Column {
public:
    explicit DatePicker(Modifier modifier, DatePickerOptions options = {}, const std::string& name = "");

    const DatePickerOptions& getOptions() const { return m_dpOptions; }
    DatePickerOptions&       getOptions()       { return m_dpOptions; }
    // Re-applies the card settings and rebuilds the grid.
    void setOptions(const DatePickerOptions& opts);

    void update(Rectf& parentBounds, float dt) override;

    // ---- Selection -------------------------------------------------------
    // Empty until something is picked. In Range mode this is the start.
    const std::optional<Date>& getSelectedDate() const { return m_selected; }
    const std::optional<Date>& getRangeStart()   const { return m_rangeStart; }
    const std::optional<Date>& getRangeEnd()     const { return m_rangeEnd; }
    bool hasSelection() const { return m_selected.has_value(); }
    // Selects the date and pages the view to its month. Silent: no callback
    // fires, since this is the application setting state, not the user.
    void setSelectedDate(const Date& date);
    void setRange(const Date& first, const Date& last);
    void clearSelection();

    // ---- Navigation ------------------------------------------------------
    int      getDisplayYear()  const { return m_displayYear; }
    unsigned getDisplayMonth() const { return m_displayMonth; }
    void showMonth(int year, unsigned month);
    void showDate(const Date& date) { showMonth(date.year, date.month); }
    void nextMonth();
    void previousMonth();
    void nextYear();
    void previousYear();
    // Pages to the current month. Does not change the selection.
    void goToToday();

    // ---- Popup presentation ---------------------------------------------
    // Presents the picker centred over the whole window, dimmed behind. The
    // UILO reference is needed because a popup picker is not part of the page
    // tree and so has no parent to inherit it from.
    void open(UILO& uiloRef);
    // For a picker already known to a UILO (embedded, or opened before).
    void open();
    void close();
    bool isOpen() const { return m_isOpen; }
    void toggle(UILO& uiloRef) { m_isOpen ? close() : open(uiloRef); }

    // Card height the current metrics add up to, which is what open() uses
    // when the options leave the popup height at 0. Nothing is counted for a
    // footer with no buttons in it.
    float preferredHeight() const;
    // Pins the height to preferredHeight() so an embedded picker occupies
    // exactly its content and no more. Returns this so it can be chained where
    // the element is declared: datepicker(...)->sizeToContent().
    DatePicker* sizeToContent();

private:
    bool hasFooterButtons() const;
    void applyCardOptions();
    void rebuild();
    void clearRows();
    // Pages the existing grid to a new month without touching its structure:
    // rewrites each cell's day, adjacency, click target, and the title, then
    // recolours. Only valid when every month lays out identically (fixed six
    // rows, adjacent days drawn as cells); returns false otherwise so the
    // caller can fall back to a rebuild. This is the path that keeps paging
    // working under the Wt bridge, whose sync only re-applies properties to the
    // elements translated once at build time -- a rebuild swaps those elements
    // out and would go unseen.
    bool updateGridInPlace(int year, unsigned month);

    Element* buildHeader();
    Element* buildWeekdayHeader();
    Element* buildGrid();
    Element* buildFooter();
    // One day cell, recorded in m_cells so applyCellColors() can restyle it
    // without a rebuild.
    Element* buildDayCell(const Date& date, bool adjacent);
    // A square icon button carrying a navigation action.
    Element* buildNavButton(const std::string& iconName, std::function<void()> action);
    Element* buildFooterButton(
        const std::string& label,
        bool primary,
        std::function<void()> action
    );

    // Re-derives every visible cell's fill and text colour from the current
    // selection, hover, and range-in-progress. The whole reason paging is the
    // only thing that rebuilds.
    void applyCellColors();
    bool isSelectable(const Date& date) const;
    // Inside the committed range, or the one being previewed under the cursor.
    bool isInRange(const Date& date) const;
    bool isRangeEndpoint(const Date& date) const;
    void handleDayClicked(const Date& date, bool adjacent);
    void setHoverDate(const std::optional<Date>& date);
    void notifyMonthChanged();

    DatePickerOptions m_dpOptions;

    int      m_displayYear  = 1970;
    unsigned m_displayMonth = 1;

    std::optional<Date> m_selected;
    std::optional<Date> m_rangeStart;
    std::optional<Date> m_rangeEnd;
    // True between the two clicks of a range, when the cell under the cursor
    // stands in for the missing end.
    bool m_awaitingRangeEnd = false;
    std::optional<Date> m_hoverDate;

    // Parallel arrays over the visible grid: cell backgrounds, their labels,
    // and which day each one is.
    std::vector<Row*>  m_cells;
    std::vector<Text*> m_cellTexts;
    std::vector<Date>  m_cellDates;
    std::vector<bool>  m_cellAdjacent;

    Text* m_titleText = nullptr;

    // Popup state. The backdrop is built once and kept, so reopening does not
    // churn the element pool.
    Column* m_backdrop = nullptr;
    bool    m_isOpen   = false;
    // The modifier the picker was constructed with, restored on close after
    // open() swaps in the centred popup geometry.
    Modifier m_embeddedModifier;

    bool m_needsRebuild = false;
};

} // namespace uilo
