#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "../Element.hpp"
#include "../../utils/Resources.hpp"
#include "../../utils/Theme.hpp"

namespace uilo {

/*
    IconOptions:
    - Desc:     Styling for an Icon. The source is one of three things, checked
                in the order markup, then file, then registry name:
                setIcon("arrow-left")          -- a name in Resources
                setIcon(Resources::icons::x)   -- the same, spelled safely
                setFile("art/logo.svg")        -- an .svg on disk
                setMarkup("<svg>...</svg>")    -- markup already in memory The
                built-in set is monochrome line art authored with
                stroke="currentColor", which no SVG parser can resolve on its
                own, so the colour set here is applied to every stroke and fill
                in the icon. That is what makes tinting an icon a one-liner --
                IconOptions().setColorRole("accent") -- and
                setPreserveOriginalColors(true) opts out, keeping whatever
                colours the markup declares, for multicolour art.
    - Stroke width is in the icon's own authoring units, the numbers in the
      markup, not pixels. The built-ins are drawn on a 24x24 grid at width 1.5,
      so setStrokeWidth(2.f) is "a bit heavier than stock" at every size, and
      the stroke keeps its proportions as the icon scales.
*/
class IconOptions {
public:
    IconOptions() = default;

    // Source
    IconOptions& setIcon(std::string_view name)    { m_name = std::string(name); return *this; }
    IconOptions& setFile(const std::string& path)  { m_file = path; return *this; }
    IconOptions& setMarkup(const std::string& svg) { m_markup = svg; return *this; }

    // Appearance
    IconOptions& setColor(const Color& c)           { m_color = c; return *this; }
    IconOptions& setColorRole(const std::string& r) { m_colorRole = r; return *this; }
    // Unset (the default) keeps the width the markup declares.
    IconOptions& setStrokeWidth(float w)            { m_strokeWidth = w; return *this; }
    IconOptions& clearStrokeWidth()                 { m_strokeWidth.reset(); return *this; }
    IconOptions& setPreserveOriginalColors(bool v)  { m_preserveColors = v; return *this; }
    // Letterbox the icon inside its bounds instead of stretching it.
    IconOptions& setPreserveAspect(bool v)          { m_preserveAspect = v; return *this; }
    IconOptions& setOpacity(float o)                { m_opacity = o; return *this; }
    IconOptions& setFlipH(bool v)                   { m_flipH = v; return *this; }
    IconOptions& setFlipV(bool v)                   { m_flipV = v; return *this; }
    // Extra raster resolution, for icons that will be scaled up after drawing.
    IconOptions& setSupersample(float s)            { m_supersample = s; return *this; }

    const std::string& getIcon()      const { return m_name; }
    const std::string& getFile()      const { return m_file; }
    const std::string& getMarkup()    const { return m_markup; }
    Color              getColor()     const { return m_color; }
    const std::string& getColorRole() const { return m_colorRole; }
    float              getStrokeWidth() const;
    bool               hasStrokeWidth() const;
    bool  getPreserveOriginalColors() const { return m_preserveColors; }
    bool  getPreserveAspect()         const { return m_preserveAspect; }
    float getOpacity()                const { return m_opacity; }
    bool  getFlipH()                  const { return m_flipH; }
    bool  getFlipV()                  const { return m_flipV; }
    float getSupersample()            const { return m_supersample; }

private:
    std::string m_name;
    std::string m_file;
    std::string m_markup;

    Color       m_color          = Color::White;
    std::string m_colorRole      = "text";
    std::optional<float> m_strokeWidth;
    bool        m_preserveColors = false;
    bool        m_preserveAspect = true;
    float       m_opacity        = 1.f;
    bool        m_flipH          = false;
    bool        m_flipV          = false;
    float       m_supersample    = 1.f;
};


/*
    getStrokeWidth():
    - Params:   none
    - Returns:  float
    - Desc:     Stroke width in the icon's own authoring units, resolved in
                three steps: the value this icon was given, then the active
                Theme's, then 0. Only meaningful when hasStrokeWidth() is true,
                since 0 is also what an unset width reports.
*/
inline float IconOptions::getStrokeWidth() const {
    return Theme::resolveIconStrokeWidth(m_strokeWidth, 0.f);
}


/*
    hasStrokeWidth():
    - Params:   none
    - Returns:  bool
    - Desc:     Whether a stroke width was set on this icon or on the active
                Theme. Only then is the width the markup declares overridden, so
                a themed stroke reaches every icon while an unthemed one keeps
                whatever its art was authored with.
*/
inline bool IconOptions::hasStrokeWidth() const {
    return m_strokeWidth.has_value()
        || Theme::current().iconStrokeOpt().has_value();
}


/*
    Icon:
    - Desc:     Draws an SVG icon. The element holds the *source* markup: the
                desktop backend parses it with NanoSVG and rasterizes to a
                texture at the size the icon actually occupies on screen, so it
                stays crisp as the window scales, while the web backend can emit
                that same markup inline instead. Keeping source rather than
                pixels on the element is what lets one Icon serve both.
    - The raster is cached and rebuilt only when something it depends on has
      moved -- pixel size, resolved colour, stroke width, or the source itself.
      A palette switch therefore re-tints without the caller doing anything.
*/
class Icon : public Element {
public:
    explicit Icon(
        Modifier modifier,
        IconOptions options = {},
        const std::string& name = ""
    );
    ~Icon() override;

    const IconOptions& getOptions() const { return m_options; }
    IconOptions&       getOptions()       { return m_options; }
    void               setOptions(const IconOptions& opts);

    void update(Rectf& parentBounds, float dt) override;
    void render() override;

    std::string_view getMarkup() const;
    float            getSourceAspect() const;

    bool isLoaded() const { return m_textureValid; }

private:
    void releaseTexture();
    bool ensureRaster(uint32_t pxW, uint32_t pxH, Color tint);

    IconOptions m_options;

    // Cache keys -- a change in any of these invalidates the texture.
    uint32_t    m_rasterW      = 0;
    uint32_t    m_rasterH      = 0;
    Color       m_rasterColor  = Color{0, 0, 0, 0};
    float       m_rasterStroke = -1.f;
    bool        m_rasterPreserveColors = false;
    std::string m_rasterSource;

    uint16_t m_textureHandle = 0xFFFFu;
    bool     m_textureValid  = false;
    uint32_t m_rasterTexW    = 0;
    uint32_t m_rasterTexH    = 0;

    // setFile() contents, read once and held so getMarkup() can hand out a view
    // that stays valid. Keyed by the path it was read from.
    mutable std::string m_fileMarkup;
    mutable std::string m_fileMarkupPath;

    mutable float m_sourceAspect      = 1.f;
    mutable bool  m_sourceAspectKnown = false;
};

} // namespace uilo
