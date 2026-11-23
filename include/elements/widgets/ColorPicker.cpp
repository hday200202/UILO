#include "ColorPicker.hpp"
#include "../../UILO.hpp"
#include "../../renderer/Shapes.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace uilo {
namespace {

Color hsv(float hue, float saturation, float value, float alpha = 1.f) {
    const float h = std::fmod(hue, 1.f) * 6.f;
    const float c = value * saturation;
    const float x = c * (1.f - std::abs(std::fmod(h, 2.f) - 1.f));
    float r = 0, g = 0, b = 0;
    switch (int(h)) {
        case 0: r = c; g = x; break;
        case 1: r = x; g = c; break;
        case 2: g = c; b = x; break;
        case 3: g = x; b = c; break;
        case 4: r = x; b = c; break;
        default: r = c; b = x; break;
    }
    const float m = value - c;
    return {uint8_t(std::lround((r + m) * 255)), uint8_t(std::lround((g + m) * 255)),
            uint8_t(std::lround((b + m) * 255)), uint8_t(std::lround(alpha * 255))};
}

void checker(Renderer& renderer, Rectf bounds, float scale) {
    const float cell = 6.f * scale;
    for (int y = 0; y * cell < bounds.size.y; ++y) {
        for (int x = 0; x * cell < bounds.size.x; ++x) {
            renderer.draw(Rect{bounds.position + Vec2f{x * cell, y * cell},
                {std::min(cell, bounds.size.x - x * cell),
                 std::min(cell, bounds.size.y - y * cell)},
                (x + y) % 2 ? Color{120, 120, 120} : Color{190, 190, 190}});
        }
    }
}

void gradient(Renderer& renderer, Rectf bounds, Color tl, Color tr, Color bl, Color br) {
    Rect rect{bounds.position, bounds.size};
    const Color colors[]{tl, tr, bl, br};
    rect.setGradientColors(colors);
    renderer.draw(rect);
}

} // namespace

/*
    Preview:
    - Desc:     Shows the current RGBA value over a checkerboard, including full transparency.
*/
class ColorPicker::Preview : public Element {
public:
    explicit Preview(ColorPicker& picker) : m_picker(picker) {
        m_modifier.setWidth({28.f, false});
    }
    void update(Rectf& area, float) override { resize(area); }
    bool isDirty() const override { return true; }
    void render() override {
        if (!m_uiloRef || !m_modifier.getVisible()) return;
        auto& renderer = m_uiloRef->getRenderer();
        checker(renderer, m_bounds, m_uiloRef->getScale());
        renderer.draw(Rect{m_bounds.position, m_bounds.size, m_picker.m_color,
            resolveColor(m_picker.m_pickerOptions.getOutlineColorRole(), Color::White),
            m_uiloRef->getScale()});
    }
private:
    ColorPicker& m_picker;
};

/*
    Field:
    - Desc:     Absolute-position editing keeps each gesture on its original rail, clamps
                out-of-bounds drags, and never warps or hides the pointer.
*/
class ColorPicker::Field : public Interactible {
public:
    explicit Field(ColorPicker& picker) : m_picker(picker) {}
    void update(Rectf& area, float) override {
        resize(area);
        if (m_drag == Part::None || !m_uiloRef) return;
        edit(m_uiloRef->getMousePosition());
        if (!m_uiloRef->isMouseButtonDown(MouseButton::Left)) m_drag = Part::None;
    }
    void onDeactivate() override { m_drag = Part::None; }
    bool isDirty() const override { return true; }
    bool checkLeftClick(const Vec2f& mouse) override {
        if (!m_uiloRef || !m_modifier.getVisible()) return false;
        m_drag = partAt(mouse);
        if (m_drag == Part::None) return false;
        m_uiloRef->setCurrInteractible(this);
        edit(mouse);
        return true;
    }
    bool checkHover(const Vec2f& mouse) override {
        const bool hit = m_modifier.getVisible() && partAt(mouse) != Part::None;
        if (hit && m_uiloRef) m_uiloRef->requestCursor(CursorType::Crosshair, 1);
        return hit;
    }
    void render() override {
        if (!m_uiloRef || !m_modifier.getVisible()) return;
        auto& renderer = m_uiloRef->getRenderer();
        const float scale = m_uiloRef->getScale();
        const auto sv = bounds(Part::SV), hue = bounds(Part::Hue);
        if (sv.size.x <= 0 || sv.size.y <= 0) return;
        renderer.pushScissor(m_bounds);
        gradient(renderer, sv, Color::White, hsv(m_picker.m_hue, 1, 1),
                 Color::Black, Color::Black);
        for (int i = 0; i < 6; ++i) {
            const Rectf segment{hue.position + Vec2f{0, hue.size.y * i / 6.f},
                {hue.size.x, hue.size.y / 6.f}};
            const auto top = hsv(i / 6.f, 1, 1), bottom = hsv((i + 1) / 6.f, 1, 1);
            gradient(renderer, segment, top, top, bottom, bottom);
        }
        auto marker = [&](Rectf rail, float position) {
            const float inset = std::min(2 * scale, rail.size.y * .5f);
            const float y = std::clamp(rail.position.y + position * rail.size.y,
                rail.position.y + inset, rail.position.y + rail.size.y - inset);
            renderer.draw(Rect{{rail.position.x - scale, y - 2 * scale},
                {rail.size.x + 2 * scale, 4 * scale}, Color::Transparent, Color::Black, scale});
            renderer.draw(Line{{rail.position.x, y},
                {rail.position.x + rail.size.x, y}, 2 * scale, Color::White});
        };
        marker(hue, m_picker.m_hue);
        if (m_picker.m_pickerOptions.getAlphaEnabled()) {
            const auto alpha = bounds(Part::Alpha);
            checker(renderer, alpha, scale);
            Color opaque = m_picker.m_color; opaque.a = 255;
            Color transparent = opaque; transparent.a = 0;
            gradient(renderer, alpha, opaque, opaque, transparent, transparent);
            marker(alpha, 1.f - m_picker.m_color.af());
        }
        const float radius = std::min(5.f * scale, std::min(sv.size.x, sv.size.y) * .5f);
        const Vec2f center{
            std::clamp(sv.position.x + m_picker.m_saturation * sv.size.x,
                sv.position.x + radius, sv.position.x + sv.size.x - radius),
            std::clamp(sv.position.y + (1.f - m_picker.m_value) * sv.size.y,
                sv.position.y + radius, sv.position.y + sv.size.y - radius)};
        renderer.drawArc(center, std::max(0.f, radius - 2 * scale), radius, 0, 360,
                         Color::Black, 40);
        renderer.drawArc(center, std::max(0.f, radius - 1.5f * scale),
                         std::max(0.f, radius - .5f * scale), 0, 360, Color::White, 40);
        renderer.popScissor();
    }
private:
    enum class Part { None, SV, Hue, Alpha };
    Rectf bounds(Part part) const {
        const float scale = m_uiloRef ? m_uiloRef->getScale() : 1.f;
        const float rail = std::min(16.f * scale, m_bounds.size.x / 8.f);
        const float gap = std::min(10.f * scale, m_bounds.size.x / 16.f);
        Rectf result = m_bounds;
        result.position += Vec2f{scale, scale};
        result.size.x = std::max(0.f, result.size.x - 2 * scale);
        result.size.y = std::max(0.f, result.size.y - 2 * scale);
        const int rails = m_picker.m_pickerOptions.getAlphaEnabled() ? 2 : 1;
        if (part == Part::SV) result.size.x = std::max(0.f, result.size.x - rails * (rail + gap));
        else {
            result.position.x += result.size.x - rail;
            if (part == Part::Alpha) result.position.x -= rail + gap;
            result.size.x = rail;
        }
        return result;
    }
    Part partAt(Vec2f point) const {
        if (bounds(Part::SV).contains(point)) return Part::SV;
        if (bounds(Part::Hue).contains(point)) return Part::Hue;
        if (m_picker.m_pickerOptions.getAlphaEnabled() && bounds(Part::Alpha).contains(point))
            return Part::Alpha;
        return Part::None;
    }
    void edit(Vec2f mouse) {
        const auto area = bounds(m_drag);
        if (area.size.x <= 0 || area.size.y <= 0) return;
        const float x = std::clamp((mouse.x - area.position.x) / area.size.x, 0.f, 1.f);
        const float y = std::clamp((mouse.y - area.position.y) / area.size.y, 0.f, 1.f);
        m_picker.changeHSV(m_drag == Part::Hue ? y : m_picker.m_hue,
            m_drag == Part::SV ? x : m_picker.m_saturation,
            m_drag == Part::SV ? 1.f - y : m_picker.m_value,
            m_drag == Part::Alpha ? 1.f - y : m_picker.m_color.af());
    }
    ColorPicker& m_picker;
    Part m_drag = Part::None;
};

/*
    ColorPicker:
    - Desc:     Builds a stable child tree so changing values never recreates focused controls.
*/
ColorPicker::ColorPicker(Modifier modifier, ColorPickerOptions options, const std::string& name)
    : Column(modifier, ColumnOptions().setOuterPadding(0).setInnerPadding(0)
        .setColorRole(options.getBackgroundColorRole()), {}, name), m_pickerOptions(options) {
    m_hex = new Textbox(Modifier(), TextboxOptions().setCharSize(12).setMaxLength(9)
        .setOuterPadding(0).setInnerPadding(5).setBackgroundColorRole(options.getFieldColorRole())
        .setTextColorRole(options.getTextColorRole()).setOutlineColorRole(options.getOutlineColorRole())
        .setOutlineThickness(1).setRounding(3).setOnStringChanged([this](const std::string& text) {
            // Keep partial input and the caret intact while previewing complete hex values.
            const size_t digits = text.size() - (!text.empty() && text.front() == '#' ? 1 : 0);
            if (digits != 6 && digits != 8) return;
            m_editingHex = true;
            setHex(text);
            m_editingHex = false;
        }).setOnEnterPressed([this](const std::string& text) {
            if (!setHex(text)) refreshText();
        }));
    m_header = new Row(Modifier().setHeight({28.f, false}),
        RowOptions().setOuterPadding(0).setInnerPadding(0)
            .setColorRole(options.getBackgroundColorRole()),
        {new Preview(*this), new Spacer(Modifier().setWidth({8.f, false})), m_hex});
    addElement(m_header);
    addElement(new Spacer(Modifier().setHeight({8.f, false})));
    addElement(new Field(*this));
    refreshText();
}

void ColorPicker::applyTheme(const Theme& theme) {
    m_pickerOptions.inheritFrom(theme.cascade<ColorPickerOptions>(m_pickerOptions.getThemeRole()));
    getOptions().setColorRole(m_pickerOptions.getBackgroundColorRole());
    if (m_header) m_header->getOptions().setColorRole(m_pickerOptions.getBackgroundColorRole());
    if (m_hex) m_hex->getOptions().setBackgroundColorRole(m_pickerOptions.getFieldColorRole())
        .setTextColorRole(m_pickerOptions.getTextColorRole())
        .setOutlineColorRole(m_pickerOptions.getOutlineColorRole());
    Column::applyTheme(theme);
}

/*
    setColor:
    - Desc:     Silently adopts an RGBA value, retaining hue for gray and hue/saturation for black.
*/
void ColorPicker::setColor(Color color) {
    if (!m_pickerOptions.getAlphaEnabled()) color.a = 255;
    if (color == m_color) return;
    m_color = color;
    const float high = std::max({color.rf(), color.gf(), color.bf()});
    const float low = std::min({color.rf(), color.gf(), color.bf()});
    const float delta = high - low;
    m_value = high;
    if (high > 0) m_saturation = delta / high;
    if (delta > 0) {
        float h = high == color.rf() ? (color.gf() - color.bf()) / delta
            : high == color.gf() ? 2.f + (color.bf() - color.rf()) / delta
                                : 4.f + (color.rf() - color.gf()) / delta;
        m_hue = std::fmod(h / 6.f + 1.f, 1.f);
    }
    refreshText();
    m_dirty = true;
}

void ColorPicker::setAlphaEnabled(bool enabled) {
    m_pickerOptions.setAlphaEnabled(enabled);
    if (!enabled) m_color.a = 255;
    refreshText();
    m_dirty = true;
}

/*
    parseHex:
    - Desc:     Strictly accepts RGB/RGBA shorthand and full hex, with an optional leading #.
*/
std::optional<Color> ColorPicker::parseHex(std::string_view text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos) return {};
    text = text.substr(first, text.find_last_not_of(" \t\r\n") - first + 1);
    if (text.front() == '#') text.remove_prefix(1);
    if (text.size() != 3 && text.size() != 4 && text.size() != 6 && text.size() != 8) return {};
    uint32_t value = 0;
    for (char c : text) {
        const int digit = c >= '0' && c <= '9' ? c - '0' :
            c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1;
        if (digit < 0) return {};
        value = text.size() <= 4 ? (value << 8) | uint32_t(digit * 17) :
                                  (value << 4) | uint32_t(digit);
    }
    if (text.size() == 3 || text.size() == 6) value = (value << 8) | 255;
    return Color::fromRGBA(value);
}

bool ColorPicker::setHex(const std::string& text) {
    const auto parsed = parseHex(text);
    if (!parsed) return false;
    const Color previous = m_color;
    setColor(*parsed);
    refreshText();
    notifyChanged(previous);
    return true;
}

void ColorPicker::refreshText() {
    if (m_editingHex) return;
    char text[10];
    if (m_pickerOptions.getAlphaEnabled())
        std::snprintf(text, sizeof(text), "#%02X%02X%02X%02X", m_color.r, m_color.g, m_color.b, m_color.a);
    else std::snprintf(text, sizeof(text), "#%02X%02X%02X", m_color.r, m_color.g, m_color.b);
    m_hex->setString(text);
}

void ColorPicker::changeHSV(float hue, float saturation, float value, float alpha) {
    const Color previous = m_color;
    m_hue = hue; m_saturation = saturation; m_value = value;
    m_color = hsv(hue, saturation, value, m_pickerOptions.getAlphaEnabled() ? alpha : 1.f);
    refreshText();
    m_dirty = true;
    notifyChanged(previous);
}

void ColorPicker::notifyChanged(Color previous) {
    if (previous != m_color && m_pickerOptions.getOnColorChanged())
        m_pickerOptions.getOnColorChanged()(m_color);
}

} // namespace uilo
