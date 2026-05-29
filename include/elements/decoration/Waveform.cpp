#include "Waveform.hpp"
#include "../../UILO.hpp"
#include "../../renderer/Shapes.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace uilo {

Waveform::Waveform(Modifier modifier, WaveformOptions options,
                   const std::string& name)
    : m_options(options)
{
    m_modifier = modifier;
    m_name     = name;
    m_type     = ElementType::Waveform;
}

void Waveform::setSamples(const float* const* channels,
                          std::size_t numChannels,
                          std::size_t numFrames) {
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

void Waveform::zoomAt(float anchorNorm, float factor) {
    if (m_numFrames == 0 || factor <= 0.f) return;
    anchorNorm = std::clamp(anchorNorm, 0.f, 1.f);

    const double total   = (double)m_numFrames;
    const double curCnt  = (m_rangeCountD > 0.0) ? m_rangeCountD : total;
    const double anchorF = m_rangeStartD + (double)anchorNorm * curCnt;

    // factor > 1 -> zoom in -> visible window shrinks.
    double newCnt = curCnt / (double)factor;
    constexpr double kMinFrames = 8.0;
    newCnt = std::clamp(newCnt, kMinFrames, total);

    double newStart = anchorF - (double)anchorNorm * newCnt;
    if (newStart < 0.0)                    newStart = 0.0;
    if (newStart + newCnt > total)         newStart = total - newCnt;

    // Keep precise state; only snap when populating the size_t fields that
    // the peak sampler reads. Without the doubles, sub-frame momentum ticks
    // each round to an int and the visible window wobbles.
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

void Waveform::update(Rectf& parentBounds, float dt) {
    (void)dt;
    Vec2f oldSize = m_bounds.size;
    resize(parentBounds);
    if (m_bounds.size != oldSize) {
        m_peaksDirty = true;
        m_dirty      = true;
    }
}

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

    // For each output column, scan its slice of input frames and record
    // (min,max). Slice size is total/cols, distributed with remainder.
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

void Waveform::renderChannelStrip(std::size_t ch, Rectf strip) {
    if (m_peakColumns <= 0) return;
    auto& renderer = m_uiloRef->getRenderer();

    const float gain   = m_options.getGain();
    const float midY   = strip.position.y + strip.size.y * 0.5f;
    const float halfH  = strip.size.y * 0.5f;
    const float thick  = std::max(0.5f, m_options.getLineThickness());
    // Per-channel color override (alpha==0 sentinel falls back to base).
    // Only honored for Stacked / Overlay layouts; SumMono uses base color.
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
        // Vertical bar from baseline to the signed peak, fat enough to
        // touch neighbouring columns — gives a solid filled envelope.
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
    } else { // WaveformStyle::Line
        // Continuous polyline through one signed value per column.
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

void Waveform::render() {
    if (!m_uiloRef) { m_dirty = false; return; }
    if (m_bounds.size.x <= 0.f || m_bounds.size.y <= 0.f) {
        m_dirty = false;
        return;
    }

    auto& renderer = m_uiloRef->getRenderer();
    const float scale = m_uiloRef->getScale();

    // Background
    const Color bg = resolveColor(m_options.getBackgroundColorRole(), m_options.getBackgroundColor());
    if (bg.a > 0) {
        const float r = m_options.getRounding() * scale;
        if (r <= 0.f)
            renderer.draw(Rect{m_bounds.position, m_bounds.size, bg});
        else
            renderer.draw(RoundedRect{m_bounds.position, m_bounds.size, r, 8u, bg});
    }

    // Clip drawing to the (optionally rounded) widget bounds so peaks
    // never leak across the corner radius.
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
