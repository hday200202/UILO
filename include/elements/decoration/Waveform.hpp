#pragma once

#include "../Element.hpp"
#include <vector>
#include <cstddef>

namespace uilo {

// How multiple audio channels are arranged inside the widget bounds.
enum class WaveformLayout {
    Stacked, // each channel gets its own horizontal strip
    Overlay, // all channels drawn over each other, full height
    SumMono, // average all channels into a single full-height waveform
};

// How a single channel's samples are drawn.
enum class WaveformStyle {
    Bars, // vertical min/max line per column (default, oscilloscope look)
    Line, // continuous polyline through per-column samples
    Filled, // continuous polyline mirrored to form a filled envelope
};

class WaveformOptions {
public:
    WaveformOptions() = default;

    WaveformOptions& setColor(Color c)              { m_color = c;       return *this; }
    WaveformOptions& setColorRole(const std::string& r)            { m_colorRole = r;       return *this; }
    // Per-channel overrides. Take effect only when layout is Stacked or
    // Overlay (SumMono collapses channels and always uses the base color).
    // An unset channel color (alpha == 0) falls back to setColor().
    WaveformOptions& setLeftChannelColor(Color c)   { m_leftColor  = c;  return *this; }
    WaveformOptions& setLeftChannelColorRole(const std::string& r) { m_leftColorRole = r;  return *this; }
    WaveformOptions& setRightChannelColor(Color c)  { m_rightColor = c;  return *this; }
    WaveformOptions& setRightChannelColorRole(const std::string& r){ m_rightColorRole = r; return *this; }
    WaveformOptions& setBackgroundColor(Color c)    { m_bgColor = c;     return *this; }
    WaveformOptions& setBackgroundColorRole(const std::string& r)  { m_bgColorRole = r;     return *this; }
    WaveformOptions& setRounding(float r)           { m_rounding = r;    return *this; }
    WaveformOptions& setLineThickness(float t)      { m_lineThickness = t; return *this; }
    WaveformOptions& setLayout(WaveformLayout l)    { m_layout = l;      return *this; }
    WaveformOptions& setStyle(WaveformStyle s)      { m_style = s;       return *this; }
    // 0 == "use widget pixel width" (1 column per pixel). Otherwise a
    // fixed bin count regardless of widget size.
    WaveformOptions& setColumns(int c)              { m_columns = c;     return *this; }
    // Multiplier on the per-pixel column count. 1.0 == 1 column per
    // pixel (default), 0.25 == 1 column per 4 pixels (smoother / less
    // aliasing), 2.0 == 2 columns per pixel (supersampled). Ignored
    // when an explicit setColumns(>0) is used.
    WaveformOptions& setResolution(float r)         { m_resolution = r;  return *this; }
    // Vertical scaling factor applied to the normalized peak before
    // mapping to pixels. 1.0 == fill the strip; values > 1 over-drive.
    WaveformOptions& setGain(float g)               { m_gain = g;        return *this; }

    Color           getColor()          const { return m_color; }
    const std::string& getColorRole()           const { return m_colorRole; }
    Color           getLeftChannelColor()  const { return m_leftColor; }
    const std::string& getLeftChannelColorRole()  const { return m_leftColorRole; }
    Color           getRightChannelColor() const { return m_rightColor; }
    const std::string& getRightChannelColorRole() const { return m_rightColorRole; }
    Color           getBackgroundColor() const { return m_bgColor; }
    const std::string& getBackgroundColorRole() const { return m_bgColorRole; }
    float           getRounding()       const { return m_rounding; }
    float           getLineThickness()  const { return m_lineThickness; }
    WaveformLayout  getLayout()         const { return m_layout; }
    WaveformStyle   getStyle()          const { return m_style; }
    int             getColumns()        const { return m_columns; }
    float           getResolution()     const { return m_resolution; }
    float           getGain()           const { return m_gain; }

private:
    Color          m_color         = Color{255, 255, 255, 255};
    std::string    m_colorRole;
    Color          m_leftColor     = Color{0, 0, 0, 0};   // a=0 -> use m_color
    std::string    m_leftColorRole;
    Color          m_rightColor    = Color{0, 0, 0, 0};   // a=0 -> use m_color
    std::string    m_rightColorRole;
    Color          m_bgColor       = Color{0, 0, 0, 0};
    std::string    m_bgColorRole;
    float          m_rounding      = 0.f;
    float          m_lineThickness = 1.f;
    WaveformLayout m_layout        = WaveformLayout::Stacked;
    WaveformStyle  m_style         = WaveformStyle::Bars;
    int            m_columns       = 0;
    float          m_resolution    = 1.f;
    float          m_gain          = 1.f;
};

class Waveform : public Element {
public:
    explicit Waveform(Modifier modifier, WaveformOptions options = {},
                      const std::string& name = "");

    const WaveformOptions& getOptions() const { return m_options; }
    WaveformOptions&       getOptions()       { return m_options; }
    void setOptions(const WaveformOptions& o) { m_options = o; m_peaksDirty = true; m_dirty = true; }

    // Copy `numFrames` samples from each of `numChannels` planar buffers
    // (matches miniaudio's `float**` deinterleaved layout). Pass nullptr
    // or numFrames==0 to clear. Samples are copied into the widget so the
    // caller's buffers may be freed immediately afterwards.
    void setSamples(const float* const* channels,
                    std::size_t numChannels,
                    std::size_t numFrames);

    // Restrict rendered region to [firstFrame, firstFrame+frameCount).
    // Pass frameCount == 0 to show the full buffer.
    void setRange(std::size_t firstFrame, std::size_t frameCount);

    // Zoom around a normalized x-position within the widget
    // (`anchorNorm` in [0,1], 0 == left edge, 1 == right edge).
    // `factor` > 1 zooms in (visible range shrinks), `factor` < 1
    // zooms out. The sample sitting under `anchorNorm` stays under
    // `anchorNorm` after the zoom (clamped at the buffer ends).
    void zoomAt(float anchorNorm, float factor);

    std::size_t getRangeStart() const { return m_rangeStart; }
    std::size_t getRangeCount() const {
        return m_rangeCount ? m_rangeCount : m_numFrames;
    }

    std::size_t getNumChannels() const { return m_numChannels; }
    std::size_t getNumFrames()   const { return m_numFrames; }

    void update(Rectf& parentBounds, float dt) override;
    void render() override;

private:
    void rebuildPeaks();
    void renderChannelStrip(std::size_t ch, Rectf strip);

    WaveformOptions m_options;

    // Owning copy of the audio data.
    std::vector<float> m_samples;        // numChannels * numFrames, planar
    std::size_t m_numChannels = 0;
    std::size_t m_numFrames   = 0;

    std::size_t m_rangeStart  = 0;
    std::size_t m_rangeCount  = 0; // 0 == full buffer

    // Cached peaks: m_peaks[ch * numColumns * 2 + col * 2 + {0,1}] = {min,max}.
    std::vector<float> m_peaks;
    std::size_t        m_peakChannels = 0;
    int                m_peakColumns  = 0;
    Vec2f              m_peakBoundsSize = {0.f, 0.f};
    bool               m_peaksDirty   = true;
};

} // namespace uilo
