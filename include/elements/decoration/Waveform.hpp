#pragma once

#include <optional>

#include "../Element.hpp"
#include <vector>
#include <cstddef>
#include "../../utils/Theme.hpp"

#include "../../utils/Themed.hpp"

namespace uilo {

/*
    WaveformLayout:
    - Desc:     How multiple audio channels are arranged inside the widget
                bounds. Stacked gives each channel its own horizontal strip,
                Overlay draws them over each other at full height, and SumMono
                averages them into a single full-height waveform.
*/
enum class WaveformLayout {
    Stacked,
    Overlay,
    SumMono,
};


/*
    WaveformStyle:
    - Desc:     How one channel's samples are drawn. Bars is a vertical min/max
                line per column, the oscilloscope look; Line is a continuous
                polyline through the per-column samples; Filled mirrors that
                polyline to form a filled envelope.
*/
enum class WaveformStyle {
    Bars,
    Line,
    Filled,
};


/*
    WaveformOptions:
    - Desc:     Everything a Waveform draws: the trace colour with optional per-
                channel overrides, the background, corner rounding, line
                thickness, the channel layout and trace style, how many columns
                the samples are binned into, and a vertical gain. Colors come as
                a literal plus a role, where the role wins when it resolves
                against the active Palette and the literal is the fallback.
*/
class WaveformOptions {
public:
    UILO_THEMED(WaveformOptions)

    /*
        inheritFrom(const WaveformOptions& prototype):
        - Params:   const WaveformOptions& prototype
        - Returns:  void
        - Desc:     Fills in every field the call site left alone from the
                    theme's prototype, and leaves the rest exactly as it was
                    set. Run when the element binds to its UILO, and again
                    whenever that UILO's theme changes.
    */
    void inheritFrom(const WaveformOptions& prototype) {
        m_lineThickness.inherit(prototype.m_lineThickness);
        m_outerPadding.inherit(prototype.m_outerPadding);
        m_rounding.inherit(prototype.m_rounding);
        m_colorRole.inherit(prototype.m_colorRole);
        m_leftColorRole.inherit(prototype.m_leftColorRole);
        m_rightColorRole.inherit(prototype.m_rightColorRole);
        m_bgColorRole.inherit(prototype.m_bgColorRole);
    }

    // Space kept outside this element, inside the slot its parent gave it. It
    // shrinks the element rather than displacing a sibling. Unset follows the theme's default for this type.
    WaveformOptions& setOuterPadding(float px)   { m_outerPadding.set(px); return *this; }
    WaveformOptions& clearOuterPadding()         { m_outerPadding.clear(); return *this; }
    float  getOuterPadding()     const { return m_outerPadding.get().value_or(0.f); }


    WaveformOptions& setColor(Color c)                  { m_color = c; return *this; }
    WaveformOptions& setColorRole(const std::string& r) { m_colorRole.set(r); return *this; }

    // Per-channel overrides. These take effect only under the Stacked and Overlay
    // layouts, since SumMono collapses the channels and always uses the base
    // colour. An unset channel colour -- alpha 0 -- falls back to setColor().
    WaveformOptions& setLeftChannelColor(Color c)                    { m_leftColor = c; return *this; }
    WaveformOptions& setLeftChannelColorRole(const std::string& r)   { m_leftColorRole.set(r); return *this; }
    WaveformOptions& setRightChannelColor(Color c)                   { m_rightColor = c; return *this; }
    WaveformOptions& setRightChannelColorRole(const std::string& r)  { m_rightColorRole.set(r); return *this; }

    WaveformOptions& setBackgroundColor(Color c)                    { m_bgColor = c; return *this; }
    WaveformOptions& setBackgroundColorRole(const std::string& r)   { m_bgColorRole.set(r); return *this; }
    WaveformOptions& setRounding(float r)                           { m_rounding.set(r); return *this; }
    WaveformOptions& setLineThickness(float t)                      { m_lineThickness.set(t); return *this; }
    WaveformOptions& setLayout(WaveformLayout l)                    { m_layout = l; return *this; }
    WaveformOptions& setStyle(WaveformStyle s)                      { m_style = s; return *this; }

    // 0 means "use the widget's pixel width", one column per pixel. Any other
    // value is a fixed bin count regardless of widget size.
    WaveformOptions& setColumns(int c)         { m_columns = c; return *this; }
    // Multiplier on the per-pixel column count: 1.0 is one column per pixel,
    // 0.25 one per four pixels (smoother, less aliasing), 2.0 supersampled.
    // Ignored when an explicit setColumns above 0 is used.
    WaveformOptions& setResolution(float r)   { m_resolution = r; return *this; }
    // Vertical scaling applied to the normalised peak before it is mapped to
    // pixels. 1.0 fills the strip; above that over-drives it.
    WaveformOptions& setGain(float g)         { m_gain = g; return *this; }

    Color              getColor()                   const { return m_color; }
    const std::string& getColorRole()               const { return m_colorRole.get(); }
    Color              getLeftChannelColor()        const { return m_leftColor; }
    const std::string& getLeftChannelColorRole()    const { return m_leftColorRole.get(); }
    Color              getRightChannelColor()       const { return m_rightColor; }
    const std::string& getRightChannelColorRole()   const { return m_rightColorRole.get(); }
    Color              getBackgroundColor()         const { return m_bgColor; }
    const std::string& getBackgroundColorRole()     const { return m_bgColorRole.get(); }
    float              getRounding()                const;
    float              getLineThickness()           const { return m_lineThickness.get(); }
    WaveformLayout     getLayout()                  const { return m_layout; }
    WaveformStyle      getStyle()                   const { return m_style; }
    int                getColumns()                 const { return m_columns; }
    float              getResolution()              const { return m_resolution; }
    float              getGain()                    const { return m_gain; }

private:
    Themed<std::optional<float>> m_outerPadding;
    Color       m_color = Color{255, 255, 255, 255};
    Themed<std::string> m_colorRole;
    Color       m_leftColor = Color{0, 0, 0, 0};   /* a == 0 -> use m_color */
    Themed<std::string> m_leftColorRole;
    Color       m_rightColor = Color{0, 0, 0, 0};   /* a == 0 -> use m_color */
    Themed<std::string> m_rightColorRole;
    Color       m_bgColor = Color{0, 0, 0, 0};
    Themed<std::string> m_bgColorRole;

    Themed<std::optional<float>> m_rounding;
    Themed<float> m_lineThickness {1.f};
    WaveformLayout       m_layout        = WaveformLayout::Stacked;
    WaveformStyle        m_style         = WaveformStyle::Bars;
    int                  m_columns       = 0;
    float                m_resolution    = 1.f;
    float                m_gain          = 1.f;
    // The theme role this was constructed with; resolved at bind time.
    std::string m_themeRole;
};


/*
    getRounding():
    - Params:   none
    - Returns:  float
    - Desc:     Corner radius, resolved in three steps: the value this element
                was given, then the active Theme's, then 0. Resolved on every
                read rather than cached, so changing the Theme restyles a
                waveform already on screen.
*/
inline float WaveformOptions::getRounding() const {
    return m_rounding.get().value_or(0.f);
}


/*
    Waveform:
    - Desc:     An audio waveform display. The element owns a copy of the sample
                data, so the caller's buffers can be freed as soon as setSamples
                returns, and bins it into per-column min/max peaks that are
                cached and only rebuilt when the data, the visible range, the
                column count or the widget size changes. A sub-range of the
                buffer can be shown and zoomed about a point, which is what
                makes it usable as a scrubbable view of a long recording rather
                than only a whole-file thumbnail.
*/
class Waveform : public Element {
public:
    void applyTheme(const Theme& theme) override {
        m_options.inheritFrom(theme.cascade<WaveformOptions>(m_options.getThemeRole()));
        Element::applyTheme(theme);
    }

    float getOuterPadding() const override { return m_options.getOuterPadding(); }

    explicit Waveform(
        Modifier modifier,
        WaveformOptions options = {},
        const std::string& name = ""
    );

    const WaveformOptions& getOptions() const { return m_options; }
    WaveformOptions&       getOptions()       { return m_options; }
    void                   setOptions(const WaveformOptions& o);

    void setSamples(
        const float* const* channels,
        std::size_t numChannels,
        std::size_t numFrames
    );

    void setRange(std::size_t firstFrame, std::size_t frameCount);
    void zoomAt(float anchorNorm, float factor);

    std::size_t getRangeStart() const { return m_rangeStart; }
    std::size_t getRangeCount() const;

    std::size_t getNumChannels() const { return m_numChannels; }
    std::size_t getNumFrames()   const { return m_numFrames; }

    void update(Rectf& parentBounds, float dt) override;
    void render() override;

private:
    void rebuildPeaks();
    void renderChannelStrip(std::size_t ch, Rectf strip);

    WaveformOptions m_options;

    // Owning copy of the audio data, planar: numChannels * numFrames.
    std::vector<float> m_samples;
    std::size_t        m_numChannels = 0;
    std::size_t        m_numFrames   = 0;

    std::size_t m_rangeStart = 0;
    std::size_t m_rangeCount = 0;   /* 0 == full buffer */
    // High-precision shadow of the visible range. zoomAt integrates many tiny
    // per-tick factors (trackpad momentum), and snapping to integer frame counts
    // on each call jitters the visible window. The doubles carry the real state;
    // the size_t fields above are derived from them, rounded, for sampling.
    double m_rangeStartD = 0.0;
    double m_rangeCountD = 0.0;   /* 0 == full buffer */

    // Cached peaks, indexed m_peaks[ch * numColumns * 2 + col * 2 + {0,1}] as
    // {min, max}.
    std::vector<float> m_peaks;
    std::size_t        m_peakChannels   = 0;
    int                m_peakColumns    = 0;
    Vec2f              m_peakBoundsSize = {0.f, 0.f};
    bool               m_peaksDirty     = true;
};


/*
    setOptions(const WaveformOptions& o):
    - Params:   const WaveformOptions& o
    - Returns:  void
    - Desc:     Replaces the options and invalidates the cached peaks, since the
                column count and resolution both feed the binning.
*/
inline void Waveform::setOptions(const WaveformOptions& o) {
    m_options    = o;
    m_peaksDirty = true;
    m_dirty      = true;
}


/*
    getRangeCount():
    - Params:   none
    - Returns:  std::size_t
    - Desc:     How many frames are visible. A stored count of 0 means the whole
                buffer, so that is reported as the frame count rather than as
                zero.
*/
inline std::size_t Waveform::getRangeCount() const {
    return m_rangeCount ? m_rangeCount : m_numFrames;
}

} // namespace uilo
