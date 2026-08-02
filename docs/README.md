# UILO API reference

Generated from the block comments in `include/` by `tools/gen_docs.py`.
Edit the comments, not these files.

80 files, 97 types, 825 functions.

## elements

- [Element.cpp](elements/Element.cpp.md)
- [Element.hpp](elements/Element.hpp.md) — `ElementType`, `Element`
- [Modifier.cpp](elements/Modifier.cpp.md)
- [Modifier.hpp](elements/Modifier.hpp.md) — `cb_traits`, `Modifier`

## elements/containers

- [Canvas.cpp](elements/containers/Canvas.cpp.md)
- [Canvas.hpp](elements/containers/Canvas.hpp.md) — `GridLineStyle`, `CanvasOptions`, `Canvas`
- [Column.cpp](elements/containers/Column.cpp.md)
- [Column.hpp](elements/containers/Column.hpp.md) — `ColumnOptions`, `Column`
- [Container.cpp](elements/containers/Container.cpp.md)
- [Container.hpp](elements/containers/Container.hpp.md) — `Container`
- [Row.cpp](elements/containers/Row.cpp.md)
- [Row.hpp](elements/containers/Row.hpp.md) — `RowOptions`, `Row`

## elements/decoration

- [Icon.cpp](elements/decoration/Icon.cpp.md)
- [Icon.hpp](elements/decoration/Icon.hpp.md) — `IconOptions`, `Icon`
- [Image.cpp](elements/decoration/Image.cpp.md)
- [Image.hpp](elements/decoration/Image.hpp.md) — `ImageOptions`, `Image`
- [Spacer.cpp](elements/decoration/Spacer.cpp.md)
- [Spacer.hpp](elements/decoration/Spacer.hpp.md) — `SpacerOptions`, `Spacer`
- [Text.cpp](elements/decoration/Text.cpp.md)
- [Text.hpp](elements/decoration/Text.hpp.md) — `TextOptions`, `Text`
- [Waveform.cpp](elements/decoration/Waveform.cpp.md)
- [Waveform.hpp](elements/decoration/Waveform.hpp.md) — `WaveformLayout`, `WaveformStyle`, `WaveformOptions`, `Waveform`

## elements/interactible

- [Button.cpp](elements/interactible/Button.cpp.md)
- [Button.hpp](elements/interactible/Button.hpp.md) — `ButtonOptions`, `Button`
- [Dropdown.cpp](elements/interactible/Dropdown.cpp.md)
- [Dropdown.hpp](elements/interactible/Dropdown.hpp.md) — `DropdownOptions`, `Dropdown`
- [Interactible.cpp](elements/interactible/Interactible.cpp.md)
- [Interactible.hpp](elements/interactible/Interactible.hpp.md) — `Interactible`
- [Knob.cpp](elements/interactible/Knob.cpp.md)
- [Knob.hpp](elements/interactible/Knob.hpp.md) — `KnobValueChangedFuncPtr`, `KnobArcDir`, `KnobOptions`, `Knob`
- [Resizer.cpp](elements/interactible/Resizer.cpp.md)
- [Resizer.hpp](elements/interactible/Resizer.hpp.md) — `ResizerDir`, `ResizerOptions`, `Resizer`
- [Slider.cpp](elements/interactible/Slider.cpp.md)
- [Slider.hpp](elements/interactible/Slider.hpp.md) — `ValueChangedFuncPtr`, `ThumbShape`, `SliderOrientation`, `SliderOptions`, …
- [Textbox.cpp](elements/interactible/Textbox.cpp.md)
- [Textbox.hpp](elements/interactible/Textbox.hpp.md) — `TextboxOptions`, `Textbox`

## elements/widgets

- [DateField.cpp](elements/widgets/DateField.cpp.md)
- [DateField.hpp](elements/widgets/DateField.hpp.md) — `DateFieldLayout`, `DateFieldOptions`, `DateField`
- [DatePicker.cpp](elements/widgets/DatePicker.cpp.md)
- [DatePicker.hpp](elements/widgets/DatePicker.hpp.md) — `DatePickerMode`, `WeekdayLabelStyle`, `DatePickerOptions`, `DatePicker`
- [Filebrowser.cpp](elements/widgets/Filebrowser.cpp.md)
- [Filebrowser.hpp](elements/widgets/Filebrowser.hpp.md) — `FileBrowserSort`, `FileBrowserOptions`, `FileBrowser`
- [Terminal.cpp](elements/widgets/Terminal.cpp.md)
- [Terminal.hpp](elements/widgets/Terminal.hpp.md) — `TerminalCell`, `TerminalOptions`, `Terminal`

## platform

- [MacStubs.cpp](platform/MacStubs.cpp.md)
- [Pty.cpp](platform/Pty.cpp.md)
- [Pty.hpp](platform/Pty.hpp.md) — `Pty`

## renderer

- [Renderer.cpp](renderer/Renderer.cpp.md)
- [Renderer.hpp](renderer/Renderer.hpp.md) — `Font`, `TextMetrics`
- [RendererImpl.hpp](renderer/RendererImpl.hpp.md) — `PosColorVertex`, `PosColorUvVertex`, `Renderer`
- [Renderer_Text.cpp](renderer/Renderer_Text.cpp.md)
- [Renderer_Texture.cpp](renderer/Renderer_Texture.cpp.md)

## utils

- [Alignment.hpp](utils/Alignment.hpp.md) — `Align`
- [Color.cpp](utils/Color.cpp.md)
- [Color.hpp](utils/Color.hpp.md) — `Color`
- [Cursor.hpp](utils/Cursor.hpp.md) — `CursorType`
- [DateAndTime.cpp](utils/DateAndTime.cpp.md)
- [DateAndTime.hpp](utils/DateAndTime.hpp.md) — `Weekday`, `Month`, `Date`, `Time`, …
- [Dimension.hpp](utils/Dimension.hpp.md) — `Dimension`
- [FileTree.hpp](utils/FileTree.hpp.md) — `FileKind`, `EntryType`, `FSEntry`, `File`, …
- [Gradient.cpp](utils/Gradient.cpp.md)
- [Gradient.hpp](utils/Gradient.hpp.md) — `GradientColor`, `Gradient`
- [Material.hpp](utils/Material.hpp.md) — `Material`
- [Math.hpp](utils/Math.hpp.md) — `Vec2f`, `Vec2u`, `Vec2i`, `Rectf`
- [OS.cpp](utils/OS.cpp.md)
- [Resources.cpp](utils/Resources.cpp.md)
- [Resources.hpp](utils/Resources.hpp.md) — `Resources`
- [Theme.cpp](utils/Theme.cpp.md)
- [Timer.hpp](utils/Timer.hpp.md) — `Timer`

## wt

- [HeadlessBackend.cpp](wt/HeadlessBackend.cpp.md) — `Pty`
- [Translator.cpp](wt/Translator.cpp.md) — `KnobWidget`, `KnobGeometry`, `kTextboxJs`
- [Translator.hpp](wt/Translator.hpp.md) — `Translator`
- [UiloWt.cpp](wt/UiloWt.cpp.md) — `UiloApplication`
- [UiloWt.hpp](wt/UiloWt.hpp.md) — `Config`, `Session`

## (root)

- [Page.cpp](Page.cpp.md)
- [Page.hpp](Page.hpp.md) — `Page`
- [Palette.cpp](Palette.cpp.md)
- [Palette.hpp](Palette.hpp.md) — `Palette`
- [UILO.cpp](UILO.cpp.md)
- [UILO.hpp](UILO.hpp.md) — `UILO`

## All types

- [Align](utils/Alignment.hpp.md#align)
- [Button](elements/interactible/Button.hpp.md#button)
- [ButtonOptions](elements/interactible/Button.hpp.md#buttonoptions)
- [Canvas](elements/containers/Canvas.hpp.md#canvas)
- [CanvasOptions](elements/containers/Canvas.hpp.md#canvasoptions)
- [Color](utils/Color.hpp.md#color)
- [Column](elements/containers/Column.hpp.md#column)
- [ColumnOptions](elements/containers/Column.hpp.md#columnoptions)
- [Config](wt/UiloWt.hpp.md#config)
- [Container](elements/containers/Container.hpp.md#container)
- [CursorType](utils/Cursor.hpp.md#cursortype)
- [Date](utils/DateAndTime.hpp.md#date)
- [DateField](elements/widgets/DateField.hpp.md#datefield)
- [DateFieldLayout](elements/widgets/DateField.hpp.md#datefieldlayout)
- [DateFieldOptions](elements/widgets/DateField.hpp.md#datefieldoptions)
- [DatePicker](elements/widgets/DatePicker.hpp.md#datepicker)
- [DatePickerMode](elements/widgets/DatePicker.hpp.md#datepickermode)
- [DatePickerOptions](elements/widgets/DatePicker.hpp.md#datepickeroptions)
- [DateTime](utils/DateAndTime.hpp.md#datetime)
- [Dimension](utils/Dimension.hpp.md#dimension)
- [Directory](utils/FileTree.hpp.md#directory)
- [Dropdown](elements/interactible/Dropdown.hpp.md#dropdown)
- [DropdownOptions](elements/interactible/Dropdown.hpp.md#dropdownoptions)
- [Element](elements/Element.hpp.md#element)
- [ElementType](elements/Element.hpp.md#elementtype)
- [EntryType](utils/FileTree.hpp.md#entrytype)
- [FSEntry](utils/FileTree.hpp.md#fsentry)
- [File](utils/FileTree.hpp.md#file)
- [FileBrowser](elements/widgets/Filebrowser.hpp.md#filebrowser)
- [FileBrowserOptions](elements/widgets/Filebrowser.hpp.md#filebrowseroptions)
- [FileBrowserSort](elements/widgets/Filebrowser.hpp.md#filebrowsersort)
- [FileKind](utils/FileTree.hpp.md#filekind)
- [FileTree](utils/FileTree.hpp.md#filetree)
- [Font](renderer/Renderer.hpp.md#font)
- [Gradient](utils/Gradient.hpp.md#gradient)
- [GradientColor](utils/Gradient.hpp.md#gradientcolor)
- [GridLineStyle](elements/containers/Canvas.hpp.md#gridlinestyle)
- [Icon](elements/decoration/Icon.hpp.md#icon)
- [IconOptions](elements/decoration/Icon.hpp.md#iconoptions)
- [Image](elements/decoration/Image.hpp.md#image)
- [ImageOptions](elements/decoration/Image.hpp.md#imageoptions)
- [Interactible](elements/interactible/Interactible.hpp.md#interactible)
- [Knob](elements/interactible/Knob.hpp.md#knob)
- [KnobArcDir](elements/interactible/Knob.hpp.md#knobarcdir)
- [KnobGeometry](wt/Translator.cpp.md#knobgeometry)
- [KnobOptions](elements/interactible/Knob.hpp.md#knoboptions)
- [KnobValueChangedFuncPtr](elements/interactible/Knob.hpp.md#knobvaluechangedfuncptr)
- [KnobWidget](wt/Translator.cpp.md#knobwidget)
- [Material](utils/Material.hpp.md#material)
- [Modifier](elements/Modifier.hpp.md#modifier)
- [Month](utils/DateAndTime.hpp.md#month)
- [Page](Page.hpp.md#page)
- [Palette](Palette.hpp.md#palette)
- [PosColorUvVertex](renderer/RendererImpl.hpp.md#poscoloruvvertex)
- [PosColorVertex](renderer/RendererImpl.hpp.md#poscolorvertex)
- [Pty](platform/Pty.hpp.md#pty)
- [Rectf](utils/Math.hpp.md#rectf)
- [Renderer](renderer/RendererImpl.hpp.md#renderer)
- [Resizer](elements/interactible/Resizer.hpp.md#resizer)
- [ResizerDir](elements/interactible/Resizer.hpp.md#resizerdir)
- [ResizerOptions](elements/interactible/Resizer.hpp.md#resizeroptions)
- [Resources](utils/Resources.hpp.md#resources)
- [Row](elements/containers/Row.hpp.md#row)
- [RowOptions](elements/containers/Row.hpp.md#rowoptions)
- [Session](wt/UiloWt.hpp.md#session)
- [Slider](elements/interactible/Slider.hpp.md#slider)
- [SliderOptions](elements/interactible/Slider.hpp.md#slideroptions)
- [SliderOrientation](elements/interactible/Slider.hpp.md#sliderorientation)
- [Spacer](elements/decoration/Spacer.hpp.md#spacer)
- [SpacerOptions](elements/decoration/Spacer.hpp.md#spaceroptions)
- [Terminal](elements/widgets/Terminal.hpp.md#terminal)
- [TerminalCell](elements/widgets/Terminal.hpp.md#terminalcell)
- [TerminalOptions](elements/widgets/Terminal.hpp.md#terminaloptions)
- [Text](elements/decoration/Text.hpp.md#text)
- [TextMetrics](renderer/Renderer.hpp.md#textmetrics)
- [TextOptions](elements/decoration/Text.hpp.md#textoptions)
- [Textbox](elements/interactible/Textbox.hpp.md#textbox)
- [TextboxOptions](elements/interactible/Textbox.hpp.md#textboxoptions)
- [ThumbShape](elements/interactible/Slider.hpp.md#thumbshape)
- [Time](utils/DateAndTime.hpp.md#time)
- [Timer](utils/Timer.hpp.md#timer)
- [Translator](wt/Translator.hpp.md#translator)
- [UILO](UILO.hpp.md#uilo)
- [UiloApplication](wt/UiloWt.cpp.md#uiloapplication)
- [ValueChangedFuncPtr](elements/interactible/Slider.hpp.md#valuechangedfuncptr)
- [Vec2f](utils/Math.hpp.md#vec2f)
- [Vec2i](utils/Math.hpp.md#vec2i)
- [Vec2u](utils/Math.hpp.md#vec2u)
- [Waveform](elements/decoration/Waveform.hpp.md#waveform)
- [WaveformLayout](elements/decoration/Waveform.hpp.md#waveformlayout)
- [WaveformOptions](elements/decoration/Waveform.hpp.md#waveformoptions)
- [WaveformStyle](elements/decoration/Waveform.hpp.md#waveformstyle)
- [Weekday](utils/DateAndTime.hpp.md#weekday)
- [WeekdayLabelStyle](elements/widgets/DatePicker.hpp.md#weekdaylabelstyle)
- [cb_traits](elements/Modifier.hpp.md#cb-traits)
- [kTextboxJs](wt/Translator.cpp.md#ktextboxjs)
