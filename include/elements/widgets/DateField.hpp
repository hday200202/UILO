#pragma once

#include <functional>
#include <optional>
#include <string>

#include "../containers/Row.hpp"
#include "../decoration/Icon.hpp"
#include "../decoration/Spacer.hpp"
#include "../decoration/Text.hpp"
#include "DatePicker.hpp"
#include "../../utils/Theme.hpp"

namespace uilo {

class UILO;

/*
    DateFieldLayout:
    - Desc:     What the field puts inside itself. Auto decides from the field's
                own shape -- given room for the date text it shows the icon, the
                value and the chevron, and squarer than that it drops to the
                icon alone, with DateFieldOptions::setLabelAspectThreshold
                deciding where the line falls. The three explicit modes skip the
                measuring and always draw the same thing.
*/
enum class DateFieldLayout { Auto, IconOnly, LabelOnly, IconAndLabel };


/*
    DateFieldOptions:
    - Desc:     The field's own appearance, plus the DatePickerOptions the popup
                is built from. Colours come in literal + role pairs like the
                rest of the library, and metrics are in virtual pixels.
*/
class DateFieldOptions {
public:
    DateFieldOptions() = default;

    // Value -------------------------------------------------------------
    // DateAndTime pattern for the value once one is picked.
    DateFieldOptions& setFormat(const std::string& pattern)  { m_format = pattern; return *this; }
    // Shown while the field is empty.
    DateFieldOptions& setPlaceholder(const std::string& s)   { m_placeholder = s; return *this; }
    // Starts the field with a value already in it, and the popup on that month.
    DateFieldOptions& setInitialDate(const Date& d)          { m_initialDate = d; return *this; }
    // In Range mode: the pattern each end is written with, and what goes
    // between them.
    DateFieldOptions& setRangeFormat(const std::string& pattern) { m_rangeFormat = pattern; return *this; }
    DateFieldOptions& setRangeSeparator(const std::string& s)    { m_rangeSeparator = s; return *this; }

    // Shape -------------------------------------------------------------
    DateFieldOptions& setLayout(DateFieldLayout l)            { m_layout = l; return *this; }
    // Width divided by height at or above which Auto shows the text. The
    // default 3.0 comes from what the row has to fit: side padding, icon, gap,
    // chevron and a date string is a little over three times a field's height
    // at the default metrics. Lower it if your format is short, raise it if you
    // spell the month out.
    DateFieldOptions& setLabelAspectThreshold(float ratio)    { m_labelAspect = ratio; return *this; }

    // Surface -----------------------------------------------------------
    DateFieldOptions& setBackgroundColor(const Color& c)           { m_bgColor = c; return *this; }
    DateFieldOptions& setBackgroundColorRole(const std::string& r) { m_bgColorRole = r; return *this; }
    DateFieldOptions& setHoverColor(const Color& c)                { m_hoverColor = c; return *this; }
    DateFieldOptions& setHoverColorRole(const std::string& r)      { m_hoverColorRole = r; return *this; }
    DateFieldOptions& setRounding(float r)                    { m_rounding = r; return *this; }
    // Inset at each end of the row, ahead of the icon and after the chevron.
    DateFieldOptions& setPadding(float px)                    { m_padding = px; return *this; }

    // Outline -------------------------------------------------------------
    // A border around the field, drawn inside its bounds. Follows
    // Theme::setOutlineThickness()/setOutlineColorRole() when left unset.
    DateFieldOptions& setOutlineColor(const Color& c)           { m_outlineColor = c; return *this; }
    DateFieldOptions& setOutlineColorRole(const std::string& r) { m_outlineColorRole = r; return *this; }
    DateFieldOptions& setOutlineThickness(float px)             { m_outlineThickness = px; return *this; }

    // Text --------------------------------------------------------------
    DateFieldOptions& setFont(const std::string& path)        { m_fontPath = path; return *this; }
    DateFieldOptions& setCharSize(unsigned int n)             { m_charSize = n; return *this; }
    DateFieldOptions& setTextColor(const Color& c)                 { m_textColor = c; return *this; }
    DateFieldOptions& setTextColorRole(const std::string& r)       { m_textColorRole = r; return *this; }
    // Dimmer than the value by default, the way a placeholder should read.
    DateFieldOptions& setPlaceholderColor(const Color& c)           { m_placeholderColor = c; return *this; }
    DateFieldOptions& setPlaceholderColorRole(const std::string& r) { m_placeholderColorRole = r; return *this; }

    // Icons -------------------------------------------------------------
    DateFieldOptions& setShowIcon(bool v)                     { m_showIcon = v; return *this; }
    DateFieldOptions& setIcon(std::string_view n)             { m_icon = std::string(n); return *this; }
    DateFieldOptions& setIconSize(float px)                   { m_iconSize = px; return *this; }
    // Gap between the icon and the value.
    DateFieldOptions& setIconSpacing(float px)                { m_iconSpacing = px; return *this; }
    DateFieldOptions& setIconStrokeWidth(float w)             { m_iconStroke = w; m_hasIconStroke = true; return *this; }
    DateFieldOptions& setIconColor(const Color& c)                 { m_iconColor = c; return *this; }
    DateFieldOptions& setIconColorRole(const std::string& r)       { m_iconColorRole = r; return *this; }
    // The dropdown-style marker at the right end. Only drawn when the text is.
    DateFieldOptions& setShowChevron(bool v)                  { m_showChevron = v; return *this; }
    DateFieldOptions& setChevronIcon(std::string_view n)      { m_chevronIcon = std::string(n); return *this; }
    DateFieldOptions& setChevronSize(float px)                { m_chevronSize = px; return *this; }
    DateFieldOptions& setChevronColor(const Color& c)              { m_chevronColor = c; return *this; }
    DateFieldOptions& setChevronColorRole(const std::string& r)    { m_chevronColorRole = r; return *this; }

    // Popup -------------------------------------------------------------
    // How the calendar itself is configured. The field overrides only what it
    // has to -- the selection callbacks it needs to track the value, and
    // closeOnSelect, which it turns on so one click settles the field the way
    // choosing a dropdown item does. Anything set here otherwise stands, so
    // range mode, bounds and year navigation all come through this.
    DateFieldOptions& setPickerOptions(const DatePickerOptions& o) { m_picker = o; return *this; }
    DatePickerOptions&       getPickerOptions()       { return m_picker; }
    const DatePickerOptions& getPickerOptions() const { return m_picker; }
    // Leave the popup open after a pick instead of dismissing it.
    DateFieldOptions& setCloseOnSelect(bool v)                { m_closeOnSelect = v; return *this; }

    // Callbacks ---------------------------------------------------------
    using DateCallback  = std::function<void(const Date&)>;
    using RangeCallback = std::function<void(const Date&, const Date&)>;
    using VoidCallback  = std::function<void()>;

    // The field's value changed because someone picked a date. Setting the
    // value from code does not fire it -- see DateField::setDate.
    DateFieldOptions& setOnDateChanged(DateCallback f)   { m_onDateChanged = std::move(f); return *this; }
    // Both ends of a range, once the second is picked, in Range mode.
    DateFieldOptions& setOnRangeChanged(RangeCallback f) { m_onRangeChanged = std::move(f); return *this; }
    DateFieldOptions& setOnOpened(VoidCallback f)        { m_onOpened = std::move(f); return *this; }
    DateFieldOptions& setOnClosed(VoidCallback f)        { m_onClosed = std::move(f); return *this; }
    DateFieldOptions& setOnCleared(VoidCallback f)       { m_onCleared = std::move(f); return *this; }

    // Getters -----------------------------------------------------------
    const std::string& getFormat()          const { return m_format; }
    const std::string& getPlaceholder()     const { return m_placeholder; }
    const std::optional<Date>& getInitialDate() const { return m_initialDate; }
    const std::string& getRangeFormat()     const { return m_rangeFormat; }
    const std::string& getRangeSeparator()  const { return m_rangeSeparator; }

    DateFieldLayout getLayout()             const { return m_layout; }
    float getLabelAspectThreshold()         const { return m_labelAspect; }

    Color getBackgroundColor()              const { return m_bgColor; }
    const std::string& getBackgroundColorRole() const { return m_bgColorRole; }
    Color getHoverColor()                   const { return m_hoverColor; }
    const std::string& getHoverColorRole()  const { return m_hoverColorRole; }
    float getRounding()                     const;
    float getPadding()                      const { return m_padding; }
    Color              getOutlineColor()     const { return m_outlineColor; }
    const std::string& getOutlineColorRole() const { return m_outlineColorRole; }
    float              getOutlineThickness() const { return m_outlineThickness; }

    const std::string& getFontPath()        const;
    unsigned int getCharSize()              const { return m_charSize; }
    Color getTextColor()                    const { return m_textColor; }
    const std::string& getTextColorRole()   const { return m_textColorRole; }
    Color getPlaceholderColor()             const { return m_placeholderColor; }
    const std::string& getPlaceholderColorRole() const { return m_placeholderColorRole; }

    bool  getShowIcon()                     const { return m_showIcon; }
    const std::string& getIcon()            const { return m_icon; }
    float getIconSize()                     const { return m_iconSize; }
    float getIconSpacing()                  const { return m_iconSpacing; }
    float getIconStrokeWidth()              const { return m_iconStroke; }
    bool  hasIconStrokeWidth()              const { return m_hasIconStroke; }
    Color getIconColor()                    const { return m_iconColor; }
    const std::string& getIconColorRole()   const { return m_iconColorRole; }
    bool  getShowChevron()                  const { return m_showChevron; }
    const std::string& getChevronIcon()     const { return m_chevronIcon; }
    float getChevronSize()                  const { return m_chevronSize; }
    Color getChevronColor()                 const { return m_chevronColor; }
    const std::string& getChevronColorRole() const { return m_chevronColorRole; }

    bool getCloseOnSelect()                 const { return m_closeOnSelect; }

    const DateCallback&  getOnDateChanged()  const { return m_onDateChanged; }
    const RangeCallback& getOnRangeChanged() const { return m_onRangeChanged; }
    const VoidCallback&  getOnOpened()       const { return m_onOpened; }
    const VoidCallback&  getOnClosed()       const { return m_onClosed; }
    const VoidCallback&  getOnCleared()      const { return m_onCleared; }


    // Raw, for handing to a child element that should keep following
    // the theme rather than being pinned to a resolved number.
    const std::optional<float>& getRoundingOpt() const { return m_rounding; }
    static constexpr float getRoundingOptFallback() { return 8.f; }

private:
    std::string m_format      = "MMM D, YYYY";
    std::string m_placeholder = "Pick a date";
    std::optional<Date> m_initialDate;
    std::string m_rangeFormat    = "MMM D";
    std::string m_rangeSeparator = " - ";

    DateFieldLayout m_layout = DateFieldLayout::Auto;
    float m_labelAspect      = 3.0f;

    Color       m_bgColor        = Color{0, 0, 0, 0};
    std::string m_bgColorRole    = "panelAlt";
    Color       m_hoverColor     = Color{255, 255, 255, 20};
    std::string m_hoverColorRole = "panel";
    std::optional<float>       m_rounding;
    float       m_padding        = 14.f;
    Color                m_outlineColor = Color::Transparent;
    std::string          m_outlineColorRole;
    float m_outlineThickness = 0.f;

    std::string  m_fontPath;
    unsigned int m_charSize      = 16;
    Color        m_textColor     = Color::White;
    std::string  m_textColorRole = "text";
    Color        m_placeholderColor     = Color{255, 255, 255, 140};
    std::string  m_placeholderColorRole = "textDim";

    bool        m_showIcon      = true;
    std::string m_icon          = "calendar";
    float       m_iconSize      = 20.f;
    float       m_iconSpacing   = 12.f;
    float       m_iconStroke    = 0.f;
    bool        m_hasIconStroke = false;
    Color       m_iconColor     = Color::White;
    std::string m_iconColorRole = "textDim";
    bool        m_showChevron   = true;
    std::string m_chevronIcon   = "chevron-down";
    float       m_chevronSize   = 20.f;
    Color       m_chevronColor  = Color::White;
    std::string m_chevronColorRole = "textDim";

    DatePickerOptions m_picker;
    bool              m_closeOnSelect = true;

    DateCallback  m_onDateChanged;
    RangeCallback m_onRangeChanged;
    VoidCallback  m_onOpened;
    VoidCallback  m_onClosed;
    VoidCallback  m_onCleared;
};


/*
    getRounding():
    - Params:   none
    - Returns:  float
    - Desc:     Corner radius of the field, resolved in three steps: the value
                this field was given, then the active Theme's, then 8.
*/
inline float DateFieldOptions::getRounding() const {
    return Theme::resolveRounding(m_rounding, 8.f);
}


/*
    getFontPath():
    - Params:   none
    - Returns:  const std::string&
    - Desc:     Font for the label, falling back to the active Theme's when this
                field was not given one.
*/
inline const std::string& DateFieldOptions::getFontPath() const {
    return Theme::resolveFont(m_fontPath);
}


/*
    DateField:
    - Desc:     A date input in the shape of a dropdown: a clickable strip
                showing the current value, which opens a DatePicker popup and
                writes back whatever is chosen. Everything the hand-wired
                version needs -- the popup, the label write-back, the hover, the
                placeholder -- is inside, so putting a date input on screen is
                one element:

            datefield(Modifier().setWidth(280_px).setHeight(48_px),
                      DateFieldOptions().setOnDateChanged([](const Date& d) {
                          save(d);
                      }))

      No UILO reference and no held pointers: the field opens the popup through
      the UILO it is already bound to as part of the page.
    - It adapts to the size it is given. With room for the date text it draws
      icon, value and chevron; squarer than setLabelAspectThreshold() it drops
      to just the icon, which is what makes a 40x40 field a sensible icon button
      and a 280x48 one a labelled field, with nothing to configure either way.
    - Configure the popup through DateFieldOptions::getPickerOptions(), so range
      mode, min/max bounds and year navigation are all reachable. In Range mode
      the label shows both ends.
*/
class DateField : public Row {
public:
    explicit DateField(Modifier modifier, DateFieldOptions options = {}, const std::string& name = "");

    const DateFieldOptions& getOptions() const { return m_dfOptions; }
    DateFieldOptions&       getOptions()       { return m_dfOptions; }
    // Re-applies the surface and the label. Does not rebuild the popup: reach
    // it through getPicker() to change the calendar after construction.
    void setOptions(const DateFieldOptions& opts);

    void update(Rectf& parentBounds, float dt) override;

    // ---- Value -----------------------------------------------------------
    // Empty until a date is picked. In Range mode this is the start.
    const std::optional<Date>& getDate()       const { return m_date; }
    const std::optional<Date>& getRangeEnd()   const { return m_rangeEnd; }
    bool hasDate() const { return m_date.has_value(); }
    // Sets the value and pages the popup to it, without firing onDateChanged:
    // this is the application writing the field, and it already knows.
    void setDate(const Date& date);
    void setRange(const Date& first, const Date& last);
    void clear();
    // The value as the field draws it, or the placeholder when empty.
    std::string getDisplayText() const;

    // ---- Popup -----------------------------------------------------------
    // The calendar behind the field. Live: changing its options after
    // construction is how you reconfigure the popup.
    DatePicker* getPicker() const { return m_picker; }
    void open();
    void close();
    bool isOpen() const;

protected:
    // The field is the click target, so the press is always consumed whether or
    // not the application attached a handler of its own. Native this stops the
    // parent seeing it; on the web it is what stops the DOM click bubbling.
    bool claimsPointerEvents() const override { return true; }

private:
    void applySurface();
    // Sets the label's string and colour from the current value.
    void refreshLabel();
    // Shows and hides the row's parts for the mode in force. Only visibility is
    // touched -- never the child list -- because the web bridge translates the
    // tree once and thereafter only re-applies properties, so a structural
    // change would go unseen while a hidden child is plain CSS.
    void applyLayout(DateFieldLayout layout);
    // Which mode Auto resolves to right now.
    DateFieldLayout resolveLayout() const;
    // Width over height, from the laid-out box when there is one and from the
    // declared dimensions when there is not. Returns 0 when neither can say.
    float resolveAspect() const;
    // Called from the popup when the user picks; updates the value, the label,
    // and fires the field's own callbacks.
    void handlePicked(const Date& date);
    void handleRangePicked(const Date& first, const Date& last);

    DateFieldOptions m_dfOptions;

    std::optional<Date> m_date;
    std::optional<Date> m_rangeEnd;

    // The row's parts, all present for the field's whole life; applyLayout()
    // decides which are visible.
    Spacer* m_padLeft     = nullptr;
    Spacer* m_centerLeft  = nullptr;   /* percent: centres the icon when alone */
    Icon*   m_icon        = nullptr;
    Spacer* m_iconGap     = nullptr;
    Text*   m_label       = nullptr;
    Spacer* m_flexRight   = nullptr;   /* percent: pushes the chevron to the end */
    Icon*   m_chevron     = nullptr;
    Spacer* m_padRight    = nullptr;

    DatePicker* m_picker = nullptr;

    // What applyLayout() last applied, so the per-frame check is a comparison
    // rather than a walk.
    DateFieldLayout m_appliedLayout = DateFieldLayout::Auto;
    bool            m_layoutApplied = false;
};

} // namespace uilo
