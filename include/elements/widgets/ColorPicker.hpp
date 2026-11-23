#pragma once

#include "../containers/Column.hpp"
#include "../containers/Row.hpp"
#include <string_view>
#include "../interactible/Textbox.hpp"
#include "../../utils/Themed.hpp"

#include <functional>
#include <optional>

namespace uilo {

/*
    ColorPickerOptions:
    - Desc:     Theme roles and change notification for an inline HSV/alpha picker.
                The caller owns popup placement and apply/cancel semantics.
*/
class ColorPickerOptions {
public:
    UILO_THEMED(ColorPickerOptions)

    void inheritFrom(const ColorPickerOptions& prototype) {
        m_backgroundRole.inherit(prototype.m_backgroundRole);
        m_fieldRole.inherit(prototype.m_fieldRole);
        m_textRole.inherit(prototype.m_textRole);
        m_outlineRole.inherit(prototype.m_outlineRole);
    }

    ColorPickerOptions& setBackgroundColorRole(const std::string& role) {
        m_backgroundRole.set(role); return *this;
    }
    ColorPickerOptions& setFieldColorRole(const std::string& role) {
        m_fieldRole.set(role); return *this;
    }
    ColorPickerOptions& setTextColorRole(const std::string& role) {
        m_textRole.set(role); return *this;
    }
    ColorPickerOptions& setOutlineColorRole(const std::string& role) {
        m_outlineRole.set(role); return *this;
    }
    ColorPickerOptions& setAlphaEnabled(bool enabled) { m_alpha = enabled; return *this; }
    ColorPickerOptions& setOnColorChanged(std::function<void(Color)> callback) {
        m_changed = std::move(callback); return *this;
    }
    const std::string& getBackgroundColorRole() const { return m_backgroundRole.get(); }
    const std::string& getFieldColorRole() const { return m_fieldRole.get(); }
    const std::string& getTextColorRole() const { return m_textRole.get(); }
    const std::string& getOutlineColorRole() const { return m_outlineRole.get(); }
    bool getAlphaEnabled() const { return m_alpha; }
    const std::function<void(Color)>& getOnColorChanged() const { return m_changed; }

private:
    Themed<std::string> m_backgroundRole{"panel"}, m_fieldRole{"panelAlt"};
    Themed<std::string> m_textRole{"text"}, m_outlineRole{"outline"};
    bool m_alpha = true;
    std::function<void(Color)> m_changed;
    std::string m_themeRole;
};

/*
    ColorPicker:
    - Desc:     Saturation/value field, vertical hue and opacity rails, preview swatch
                and editable hex text. Programmatic changes are silent; user edits
                notify once. Achromatic colors retain the last chosen hue.
*/
class ColorPicker : public Column {
public:
    explicit ColorPicker(Modifier modifier = {}, ColorPickerOptions options = {},
                         const std::string& name = "");
    void applyTheme(const Theme& theme) override;
    void setColor(Color color);
    Color getColor() const { return m_color; }
    void setAlphaEnabled(bool enabled);
    bool setHex(const std::string& text);
    static std::optional<Color> parseHex(std::string_view text);
    ColorPickerOptions& getPickerOptions() { return m_pickerOptions; }
    const ColorPickerOptions& getPickerOptions() const { return m_pickerOptions; }

private:
    class Field;
    class Preview;
    void refreshText();
    void changeHSV(float hue, float saturation, float value, float alpha);
    void notifyChanged(Color previous);
    ColorPickerOptions m_pickerOptions;
    Color m_color = Color::White;
    float m_hue = 0.f, m_saturation = 0.f, m_value = 1.f;
    Textbox* m_hex = nullptr;
    Row* m_header = nullptr;
    bool m_editingHex = false;
};

} // namespace uilo
