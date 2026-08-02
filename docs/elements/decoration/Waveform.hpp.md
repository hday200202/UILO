# Waveform.hpp

`include/elements/decoration/Waveform.hpp`

[← index](../../README.md)

## Types

- [WaveformLayout](#waveformlayout)
- [WaveformStyle](#waveformstyle)
- [WaveformOptions](#waveformoptions)
- [Waveform](#waveform)

## Functions

- [`getRounding()`](#getrounding)
- [`setOptions(const WaveformOptions& o)`](#setoptions)
- [`getRangeCount()`](#getrangecount)

---

### WaveformLayout

How multiple audio channels are arranged inside the widget bounds. Stacked gives each channel its own horizontal strip, Overlay draws them over each other at full height, and SumMono averages them into a single full-height waveform.

---

### WaveformStyle

How one channel's samples are drawn. Bars is a vertical min/max line per column, the oscilloscope look; Line is a continuous polyline through the per-column samples; Filled mirrors that polyline to form a filled envelope.

---

### WaveformOptions

Everything a [Waveform](#waveform) draws: the trace colour with optional per- channel overrides, the background, corner rounding, line thickness, the channel layout and trace style, how many columns the samples are binned into, and a vertical gain. Colors come as a literal plus a role, where the role wins when it resolves against the active [Palette](../../Palette.hpp.md#palette) and the literal is the fallback.

---

### Waveform

An audio waveform display. The element owns a copy of the sample data, so the caller's buffers can be freed as soon as setSamples returns, and bins it into per-column min/max peaks that are cached and only rebuilt when the data, the visible range, the column count or the widget size changes. A sub-range of the buffer can be shown and zoomed about a point, which is what makes it usable as a scrubbable view of a long recording rather than only a whole-file thumbnail.

---

### getRounding

```cpp
getRounding()
```

**Returns** — float

Corner radius, resolved in three steps: the value this element was given, then the active Theme's, then 0. Resolved on every read rather than cached, so changing the Theme restyles a waveform already on screen.

---

### setOptions

```cpp
setOptions(const WaveformOptions& o)
```

**Parameters**

- `const WaveformOptions& o`

**Returns** — void

Replaces the options and invalidates the cached peaks, since the column count and resolution both feed the binning.

---

### getRangeCount

```cpp
getRangeCount()
```

**Returns** — std::size_t

How many frames are visible. A stored count of 0 means the whole buffer, so that is reported as the frame count rather than as zero.
