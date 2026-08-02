#include "Waveform.hpp"
#include "../../UILO.hpp"
#include "../../renderer/Shapes.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace uilo {

/*
    Waveform(Modifier modifier, WaveformOptions options, const std::string& name):
    - Params:   Modifier modifier, WaveformOptions options, const std::string&
                name
    - Returns:  Waveform
    - Desc:     Constructs an empty waveform from a modifier and its options,
                and tags it as a Waveform. Sample data arrives separately
                through setSamples.
*/
Waveform::Waveform(
    Modifier modifier,
    WaveformOptions options,
    const std::string& name
) : m_options(options) {
    m_modifier = modifier;
    m_name     = name;
    m_type     = ElementType::Waveform;
}


/*
    setSamples(...):
    - Params:   const float* const* channels, std::size_t numChannels,
                std::size_t numFrames
    - Returns:  void
    - Desc:     Copies numFrames samples from each of numChannels planar
                buffers, which matches miniaudio's deinterleaved float** layout.
                The data is copied into the element, so the caller's buffers may
                be freed immediately afterwards. A null pointer or a zero count
                clears the display. A null individual channel is filled with
                silence rather than skipped, so the channel count stays honest.
                Any existing visible range is clamped into the new buffer
                instead of being reset, so re-supplying data does not throw away
                where the user was looking.
*/
void Waveform::setSamples(
    const float* const* channels,
    std::size_t numChannels,
    std::size_t numFrames
) {
    if (!channels || numChannels == 0 || numFrames == 0) {
        m_samples.clear();
        m_numChannels = 0;
        m_numFrames   = 0;
        m_rangeStart  = 0;
        m_rangeCount  = 0;
        m_peaksDirty  = true;
        m_dirty       = true;
        return;
    }

    m_samples.resize(numChannels * numFrames);
    for (std::size_t c = 0; c < numChannels; ++c) {
        if (channels[c])
            std::memcpy(m_samples.data() + c * numFrames,
                        channels[c], numFrames * sizeof(float));
        else
            std::memset(m_samples.data() + c * numFrames, 0,
                        numFrames * sizeof(float));
    }

    m_numChannels = numChannels;
    m_numFrames   = numFrames;
    if (m_rangeStart >= numFrames) m_rangeStart = 0;
    if (m_rangeCount > 0 && m_rangeStart + m_rangeCount > numFrames)
        m_rangeCount = numFrames - m_rangeStart;
    m_peaksDirty = true;
    m_dirty      = true;
}


/*
    setRange(std::size_t firstFrame, std::size_t frameCount):
    - Params:   std::size_t firstFrame, std::size_t frameCount
    - Returns:  void
    - Desc:     Restricts the rendered region to frameCount frames from
                firstFrame, clamped into the buffer. A frameCount of 0 shows the
                whole buffer. Also resets the high-precision shadow of the
                range, so a later zoom starts from exactly what was asked for
                here rather than from where a previous gesture had drifted to.
*/
void Waveform::setRange(std::size_t firstFrame, std::size_t frameCount) {
    if (m_numFrames == 0) {
        m_rangeStart = 0;
        m_rangeCount = 0;
    } else {
        m_rangeStart = std::min(firstFrame, m_numFrames - 1);
        m_rangeCount = (frameCount == 0)
            ? 0
            : std::min(frameCount, m_numFrames - m_rangeStart);
    }

    m_rangeStartD = (double)m_rangeStart;
    m_rangeCountD = (double)m_rangeCount;
    m_peaksDirty = true;
    m_dirty      = true;
}


/*
    zoomAt(float anchorNorm, float factor):
    - Params:   float anchorNorm -- 0 at the left edge, 1 at the right, float
                factor -- above 1 zooms in, below 1 zooms out
    - Returns:  void
    - Desc:     Zooms the visible range about a normalised position across the
                widget, keeping the sample under that position under it
                afterwards, clamped at the buffer ends. The range is held as
                doubles and only rounded when the integer fields the peak
                sampler reads are populated: a trackpad delivers many sub-frame
                momentum ticks, and rounding each one to whole frames makes the
                visible window wobble. The window will not shrink below a few
                frames, so zooming in without limit cannot collapse it to
                nothing.
*/
void Waveform::zoomAt(float anchorNorm, float factor) {
    if (m_numFrames == 0 || factor <= 0.f) return;
    anchorNorm = std::clamp(anchorNorm, 0.f, 1.f);

    const double total   = (double)m_numFrames;
    const double curCnt  = (m_rangeCountD > 0.0) ? m_rangeCountD : total;
    const double anchorF = m_rangeStartD + (double)anchorNorm * curCnt;

    double newCnt = curCnt / (double)factor;
    constexpr double kMinFrames = 8.0;
    newCnt = std::clamp(newCnt, kMinFrames, total);

    double newStart = anchorF - (double)anchorNorm * newCnt;
    if (newStart < 0.0)            newStart = 0.0;
    if (newStart + newCnt > total) newStart = total - newCnt;

    /* Precise state stays in the doubles; the integer fields are derived. */
    m_rangeStartD = newStart;
    m_rangeCountD = (newCnt >= total) ? 0.0 : newCnt;
    const std::size_t start = (std::size_t)std::llround(newStart);
    const std::size_t cnt   = (std::size_t)std::llround(newCnt);
    const std::size_t count = (cnt >= m_numFrames) ? 0 : cnt;
    m_rangeStart = std::min(start, m_numFrames - 1);
    m_rangeCount = (count == 0) ? 0 : std::min(count, m_numFrames - m_rangeStart);
    m_peaksDirty = true;
    m_dirty      = true;
}


/*
    update(Rectf& parentBounds, float dt):
    - Params:   Rectf& parentBounds, float dt
    - Returns:  void
    - Desc:     Resolves the element's bounds and invalidates the cached peaks
                when the size changed, since the column count is derived from
                the width.
*/
void Waveform::update(Rectf& parentBounds, float dt) {
    (void)dt;
    Vec2f oldSize = m_bounds.size;
    resize(parentBounds);
    if (m_bounds.size != oldSize) {
        m_peaksDirty = true;
        m_dirty      = true;
    }
}


/*
    rebuildPeaks():
    - Params:   none
    - Returns:  void
    - Desc:     Bins the visible range of samples into per-column min/max pairs,
                which is what makes drawing independent of how long the
                recording is. The column count comes from the options, or from
                the widget's pixel width times the resolution, and is capped at
                the number of frames available so a short selection is never
                over-sampled. Each column takes a slice of the range computed
                from its fractional position, which distributes the remainder
                evenly instead of piling it on the last column, and every slice
                is forced to at least one frame so no column is left empty.
                Under SumMono the channels are averaged into a single set of
                peaks.
*/
void Waveform::rebuildPeaks() {
    m_peaks.clear();
    m_peakChannels   = 0;
    m_peakColumns    = 0;
    m_peakBoundsSize = m_bounds.size;
    m_peaksDirty     = false;

    if (m_numChannels == 0 || m_numFrames == 0) return;
    if (m_bounds.size.x <= 0.f || m_bounds.size.y <= 0.f) return;

    const std::size_t first = m_rangeStart;
    const std::size_t total = (m_rangeCount > 0) ? m_rangeCount
                                                 : (m_numFrames - first);
    if (total == 0) return;

    int cols = m_options.getColumns();
    if (cols <= 0) {
        const float res = std::max(0.0001f, m_options.getResolution());
        cols = std::max(1, (int)std::floor(m_bounds.size.x * res));
    }
    cols = std::min<int>(cols, (int)total);
    if (cols <= 0) return;

    const bool sum = (m_options.getLayout() == WaveformLayout::SumMono);
    const std::size_t outChannels = sum ? 1 : m_numChannels;

    m_peaks.assign(outChannels * (std::size_t)cols * 2, 0.f);
    m_peakChannels = outChannels;
    m_peakColumns  = cols;

    for (int col = 0; col < cols; ++col) {
        std::size_t s0 = first + (std::size_t)((double)total * (double)col       / (double)cols);
        std::size_t s1 = first + (std::size_t)((double)total * (double)(col + 1) / (double)cols);
        if (s1 <= s0) s1 = s0 + 1;
        if (s1 > first + total) s1 = first + total;

        if (sum) {
            float mn = 0.f, mx = 0.f;
            for (std::size_t s = s0; s < s1; ++s) {
                float sum = 0.f;
                for (std::size_t c = 0; c < m_numChannels; ++c)
                    sum += m_samples[c * m_numFrames + s];
                sum /= (float)m_numChannels;
                if (sum < mn) mn = sum;
                if (sum > mx) mx = sum;
            }
            m_peaks[(std::size_t)col * 2 + 0] = mn;
            m_peaks[(std::size_t)col * 2 + 1] = mx;
        } else {
            for (std::size_t c = 0; c < m_numChannels; ++c) {
                const float* src = m_samples.data() + c * m_numFrames;
                float mn = 0.f, mx = 0.f;
                for (std::size_t s = s0; s < s1; ++s) {
                    float v = src[s];
                    if (v < mn) mn = v;
                    if (v > mx) mx = v;
                }
                std::size_t base = c * (std::size_t)cols * 2 + (std::size_t)col * 2;
                m_peaks[base + 0] = mn;
                m_peaks[base + 1] = mx;
            }
        }
    }
}


/*
    renderChannelStrip(std::size_t ch, Rectf strip):
    - Params:   std::size_t ch, Rectf strip
    - Returns:  void
    - Desc:     Draws one channel's cached peaks into a rectangle, in whichever
                of the three styles the options ask for, batched into a single
                call. Bars draws a vertical min-to-max line per column and is
                widened to a visible minimum so silence still reads as a centre
                line. Filled draws from the baseline to the larger of the two
                peaks with a bar wide enough to touch its neighbours, which is
                what makes a solid envelope out of lines. Line joins one signed
                value per column into a polyline. A per-channel colour override
                applies under Stacked and Overlay; SumMono has collapsed the
                channels and keeps the base colour.
*/
void Waveform::renderChannelStrip(std::size_t ch, Rectf strip) {
    if (m_peakColumns <= 0) return;
    auto& renderer = m_uiloRef->getRenderer();

    const float gain   = m_options.getGain();
    const float midY   = strip.position.y + strip.size.y * 0.5f;
    const float halfH  = strip.size.y * 0.5f;
    const float thick  = std::max(0.5f, m_options.getLineThickness());

    Color color = resolveColor(m_options.getColorRole(), m_options.getColor());
    if (m_options.getLayout() != WaveformLayout::SumMono) {
        if (ch == 0 && m_options.getLeftChannelColor().a  != 0)
            color = resolveColor(m_options.getLeftChannelColorRole(), m_options.getLeftChannelColor());
        else if (ch == 1 && m_options.getRightChannelColor().a != 0)
            color = resolveColor(m_options.getRightChannelColorRole(), m_options.getRightChannelColor());
    }
    const auto  style  = m_options.getStyle();

    const float colW = strip.size.x / (float)m_peakColumns;

    std::vector<Line> lines;

    const std::size_t base = ch * (std::size_t)m_peakColumns * 2;

    if (style == WaveformStyle::Bars) {
        lines.reserve((std::size_t)m_peakColumns);
        for (int col = 0; col < m_peakColumns; ++col) {
            float mn = m_peaks[base + (std::size_t)col * 2 + 0] * gain;
            float mx = m_peaks[base + (std::size_t)col * 2 + 1] * gain;
            mn = std::clamp(mn, -1.f, 1.f);
            mx = std::clamp(mx, -1.f, 1.f);
            float x  = strip.position.x + colW * ((float)col + 0.5f);
            float y0 = midY - mx * halfH;
            float y1 = midY - mn * halfH;
            if (std::abs(y1 - y0) < 1.f) { y0 = midY - 0.5f; y1 = midY + 0.5f; }
            lines.push_back(Line{{x, y0}, {x, y1}, thick, color});
        }
    } else if (style == WaveformStyle::Filled) {
        lines.reserve((std::size_t)m_peakColumns);
        const float barThick = std::max(thick, colW + 1.f);
        for (int col = 0; col < m_peakColumns; ++col) {
            float mn = m_peaks[base + (std::size_t)col * 2 + 0] * gain;
            float mx = m_peaks[base + (std::size_t)col * 2 + 1] * gain;
            mn = std::clamp(mn, -1.f, 1.f);
            mx = std::clamp(mx, -1.f, 1.f);
            float v  = (std::abs(mx) >= std::abs(mn)) ? mx : mn;
            float x  = strip.position.x + colW * ((float)col + 0.5f);
            float y  = midY - v * halfH;
            if (std::abs(y - midY) < 0.5f) y = midY + (v >= 0.f ? -0.5f : 0.5f);
            lines.push_back(Line{{x, midY}, {x, y}, barThick, color});
        }
    } else {
        lines.reserve((std::size_t)std::max(0, m_peakColumns - 1));
        auto pointAt = [&](int col) {
            float mn = m_peaks[base + (std::size_t)col * 2 + 0] * gain;
            float mx = m_peaks[base + (std::size_t)col * 2 + 1] * gain;
            mn = std::clamp(mn, -1.f, 1.f);
            mx = std::clamp(mx, -1.f, 1.f);
            float v  = (std::abs(mx) >= std::abs(mn)) ? mx : mn;
            return Vec2f{strip.position.x + colW * ((float)col + 0.5f),
                         midY - v * halfH};
        };
        Vec2f prev = pointAt(0);
        for (int col = 1; col < m_peakColumns; ++col) {
            Vec2f cur = pointAt(col);
            lines.push_back(Line{prev, cur, thick, color});
            prev = cur;
        }
    }

    renderer.drawLines(lines.data(), lines.size());
}


/*
    render():
    - Params:   none
    - Returns:  void
    - Desc:     Draws the background, then the channels, clipped to the widget's
                own rounded bounds so peaks never leak across the corner radius.
                The peaks are rebuilt here when stale, which is also where a
                size change first takes effect. Under Stacked each channel gets
                an equal horizontal slice of the height; Overlay draws them all
                at full height; SumMono draws the one averaged set.
*/
void Waveform::render() {
    if (!m_modifier.getVisible()) { m_dirty = false; return; }
    if (!m_uiloRef)               { m_dirty = false; return; }
    if (m_bounds.size.x <= 0.f || m_bounds.size.y <= 0.f) {
        m_dirty = false;
        return;
    }

    auto& renderer = m_uiloRef->getRenderer();
    const float scale = m_uiloRef->getScale();

    const Color bg = resolveColor(m_options.getBackgroundColorRole(), m_options.getBackgroundColor());
    if (bg.a > 0) {
        const float r = m_options.getRounding() * scale;
        if (r <= 0.f)
            renderer.draw(Rect{m_bounds.position, m_bounds.size, bg});
        else
            renderer.draw(RoundedRect{m_bounds.position, m_bounds.size, r, 8u, bg});
    }

    const float r = m_options.getRounding() * scale;
    renderer.pushRoundClip(m_bounds, r);

    if (m_peaksDirty || m_peakBoundsSize != m_bounds.size) rebuildPeaks();

    if (m_peakChannels > 0 && m_peakColumns > 0) {
        switch (m_options.getLayout()) {
            case WaveformLayout::SumMono:
                renderChannelStrip(0, m_bounds);
                break;
            case WaveformLayout::Overlay:
                for (std::size_t c = 0; c < m_peakChannels; ++c)
                    renderChannelStrip(c, m_bounds);
                break;
            case WaveformLayout::Stacked: {
                const float h = m_bounds.size.y / (float)m_peakChannels;
                for (std::size_t c = 0; c < m_peakChannels; ++c) {
                    Rectf strip{
                        {m_bounds.position.x,
                         m_bounds.position.y + (float)c * h},
                        {m_bounds.size.x, h}
                    };
                    renderChannelStrip(c, strip);
                }
                break;
            }
        }
    }

    renderer.popRoundClip();
    m_dirty = false;
}

} // namespace uilo
