# Waveform.cpp

`include/elements/decoration/Waveform.cpp`

[← index](../../README.md)

## Functions

- [`Waveform(Modifier modifier, WaveformOptions options, const std::string& name)`](#waveform)
- [`setSamples(...)`](#setsamples)
- [`setRange(std::size_t firstFrame, std::size_t frameCount)`](#setrange)
- [`zoomAt(float anchorNorm, float factor)`](#zoomat)
- [`update(Rectf& parentBounds, float dt)`](#update)
- [`rebuildPeaks()`](#rebuildpeaks)
- [`renderChannelStrip(std::size_t ch, Rectf strip)`](#renderchannelstrip)
- [`render()`](#render)

---

### Waveform

```cpp
Waveform(Modifier modifier, WaveformOptions options, const std::string& name)
```

**Parameters**

- `Modifier modifier`
- `WaveformOptions options`
- `const std::string& name`

**Returns** — [Waveform](Waveform.hpp.md#waveform)

Constructs an empty waveform from a modifier and its options, and tags it as a [Waveform](Waveform.hpp.md#waveform). Sample data arrives separately through setSamples.

---

### setSamples

```cpp
setSamples(...)
```

**Parameters**

- `const float* const* channels`
- `std::size_t numChannels`
- `std::size_t numFrames`

**Returns** — void

Copies numFrames samples from each of numChannels planar buffers, which matches miniaudio's deinterleaved float** layout. The data is copied into the element, so the caller's buffers may be freed immediately afterwards. A null pointer or a zero count clears the display. A null individual channel is filled with silence rather than skipped, so the channel count stays honest. Any existing visible range is clamped into the new buffer instead of being reset, so re-supplying data does not throw away where the user was looking.

---

### setRange

```cpp
setRange(std::size_t firstFrame, std::size_t frameCount)
```

**Parameters**

- `std::size_t firstFrame`
- `std::size_t frameCount`

**Returns** — void

Restricts the rendered region to frameCount frames from firstFrame, clamped into the buffer. A frameCount of 0 shows the whole buffer. Also resets the high-precision shadow of the range, so a later zoom starts from exactly what was asked for here rather than from where a previous gesture had drifted to.

---

### zoomAt

```cpp
zoomAt(float anchorNorm, float factor)
```

**Parameters**

- `float anchorNorm -- 0 at the left edge`
- `1 at the right`
- `float factor -- above 1 zooms in`
- `below 1 zooms out`

**Returns** — void

Zooms the visible range about a normalised position across the widget, keeping the sample under that position under it afterwards, clamped at the buffer ends. The range is held as doubles and only rounded when the integer fields the peak sampler reads are populated: a trackpad delivers many sub-frame momentum ticks, and rounding each one to whole frames makes the visible window wobble. The window will not shrink below a few frames, so zooming in without limit cannot collapse it to nothing.

---

### update

```cpp
update(Rectf& parentBounds, float dt)
```

**Parameters**

- `Rectf& parentBounds`
- `float dt`

**Returns** — void

Resolves the element's bounds and invalidates the cached peaks when the size changed, since the column count is derived from the width.

---

### rebuildPeaks

```cpp
rebuildPeaks()
```

**Returns** — void

Bins the visible range of samples into per-column min/max pairs, which is what makes drawing independent of how long the recording is. The column count comes from the options, or from the widget's pixel width times the resolution, and is capped at the number of frames available so a short selection is never over-sampled. Each column takes a slice of the range computed from its fractional position, which distributes the remainder evenly instead of piling it on the last column, and every slice is forced to at least one frame so no column is left empty. Under SumMono the channels are averaged into a single set of peaks.

---

### renderChannelStrip

```cpp
renderChannelStrip(std::size_t ch, Rectf strip)
```

**Parameters**

- `std::size_t ch`
- `Rectf strip`

**Returns** — void

Draws one channel's cached peaks into a rectangle, in whichever of the three styles the options ask for, batched into a single call. Bars draws a vertical min-to-max line per column and is widened to a visible minimum so silence still reads as a centre line. Filled draws from the baseline to the larger of the two peaks with a bar wide enough to touch its neighbours, which is what makes a solid envelope out of lines. Line joins one signed value per column into a polyline. A per-channel colour override applies under Stacked and Overlay; SumMono has collapsed the channels and keeps the base colour.

---

### render

```cpp
render()
```

**Returns** — void

Draws the background, then the channels, clipped to the widget's own rounded bounds so peaks never leak across the corner radius. The peaks are rebuilt here when stale, which is also where a size change first takes effect. Under Stacked each channel gets an equal horizontal slice of the height; Overlay draws them all at full height; SumMono draws the one averaged set.
