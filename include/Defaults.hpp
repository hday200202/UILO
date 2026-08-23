#pragma once

#include "elements/Elements.hpp"
#include "utils/Theme.hpp"

/*
    Defaults.hpp:
    - Desc:     What UILO looks like before an application says otherwise. This
                is the whole of the library's default styling: the palette's
                colour roles, one prototype per Modifier and *Options type, and
                the named variants of them that the h1()/panel()/card() helpers
                are built on. Nothing else in the library carries a look, so
                editing this file restyles everything.

                    Theme theme = defaultTheme();
                    theme.palette().set("accent", {220, 90, 90, 255});
                    ui.setTheme(theme);

    - Reading it: each block starts from theme.edit<T>(), which hands back the
      prototype for that type, and says only what the default look changes. A
      field this file does not mention keeps the type's built-in value, and can
      be pinned here by adding a line for it.
    - A prototype fills in only what a call site left alone -- it never
      overrides a setting -- and it is applied when an element binds to its
      UILO, not when it is constructed. That is what lets a tree be built before
      the UILO exists, and what lets ui.setTheme() restyle a running app.
    - Some fields are deliberately left unset, because "unset" is a behaviour
      rather than a missing value: a TextOptions with no character size sizes
      its glyphs from the element's height, an IconOptions with no stroke width
      keeps whatever its markup was drawn with, and a Button with no rounding
      follows the radius of whatever composite widget is holding it. Each is
      marked below.
*/

namespace uilo {

/* ------------------------------------------------------------------------ */
/*  Palette                                                                  */
/* ------------------------------------------------------------------------ */

/*
    darkPalette():
    - Params:   none
    - Returns:  Palette
    - Desc:     The colour roles every built-in widget reads, in their dark
                values. A palette that covers these nine names themes the whole
                library without a single role being spelled at a call site;
                everything else here is an alias pointing at one of them.
*/
inline Palette darkPalette() {
    Palette p;
    p.setFallback({255, 0, 255, 255});

    /* The nine the widgets actually read. */
    p.set("bg",          { 33,  35,  47, 255});
    p.set("panel",       { 44,  47,  60, 255});
    p.set("panelAlt",    { 40,  44,  56, 255});
    p.set("accent",      {151, 120, 206, 255});
    p.set("accentHover", {171, 140, 226, 255});
    p.set("onAccent",    {255, 255, 255, 255});
    p.set("text",        {235, 238, 245, 255});
    p.set("textDim",     {160, 168, 190, 255});
    p.set("outline",     { 80,  84, 100, 255});

    /* Aliases. Nothing points at these by default -- they are names an
       application can aim an element at without inventing a colour, and
       repointing one here restyles every element that uses it. */
    p.setAlias("app.bg",     "bg");
    p.setAlias("column.bg",  "panel");
    p.setAlias("row.bg",     "panel");
    p.setAlias("text.color", "text");
    p.setAlias("image.tint", "text");

    return p;
}


/*
    lightPalette():
    - Params:   none
    - Returns:  Palette
    - Desc:     darkPalette()'s roles in their light values, so switching the
                two restyles a running application without rebuilding anything:
                colours resolve through the palette every frame.
*/
inline Palette lightPalette() {
    Palette p;
    p.setFallback({255, 0, 255, 255});

    p.set("bg",          {238, 240, 246, 255});
    p.set("panel",       {255, 255, 255, 255});
    p.set("panelAlt",    {245, 247, 252, 255});
    p.set("accent",      {120,  90, 190, 255});
    p.set("accentHover", {140, 110, 210, 255});
    p.set("onAccent",    {255, 255, 255, 255});
    p.set("text",        { 30,  32,  44, 255});
    p.set("textDim",     {100, 108, 130, 255});
    p.set("outline",     {200, 205, 220, 255});

    p.setAlias("app.bg",     "bg");
    p.setAlias("column.bg",  "panel");
    p.setAlias("row.bg",     "panel");
    p.setAlias("text.color", "text");
    p.setAlias("image.tint", "text");

    return p;
}


/* ------------------------------------------------------------------------ */
/*  The default theme                                                        */
/* ------------------------------------------------------------------------ */

/*
    defaultTheme():
    - Params:   none
    - Returns:  Theme
    - Desc:     Everything above assembled into the theme a UILO starts on.
                Build from this rather than from a bare Theme{} when overriding
                a few things, since a Theme constructed from scratch defines no
                prototypes at all and leaves every type on its built-in
                baseline.
*/
inline Theme defaultTheme() {
    Theme theme;
    theme.setPalette(darkPalette());

    /* -------------------------------------------------------------------- */
    /*  Type scale                                                          */
    /*                                                                      */
    /*  What h1() / h2() / h3() / body() / caption() are. Both halves of a   */
    /*  heading share a name: the Modifier carries the line box, the         */
    /*  TextOptions the glyphs.                                             */
    /* -------------------------------------------------------------------- */

    struct Heading {
        const char*  role;
        unsigned int size;
        float        lineHeight;
    };
    const Heading headings[] = {
        {"h1", 42, 56.f},
        {"h2", 32, 44.f},
        {"h3", 24, 34.f},
    };
    for (const Heading& heading : headings) {
        TextOptions& style = theme.edit<TextOptions>(heading.role);
        style.setCharSize(heading.size);
        style.setBold(true);
        style.setColorRole("text");
        style.setTextAlignY(Align::CenterY);

        theme.edit<Modifier>(heading.role).setHeight(Dimension{heading.lineHeight, false});
    }

    TextOptions& body = theme.edit<TextOptions>("body");
    body.setCharSize(16);
    body.setColorRole("text");
    body.setWrap(true);

    TextOptions& caption = theme.edit<TextOptions>("caption");
    caption.setCharSize(14);
    caption.setColorRole("textDim");
    caption.setTextAlignY(Align::CenterY);
    theme.edit<Modifier>("caption").setHeight(24_px);

    /* The label inside a button: centred, and small enough that a 36px button
       does not look like a heading. */
    TextOptions& label = theme.edit<TextOptions>("label");
    label.setCharSize(15);
    label.setColorRole("text");
    label.setTextAlignX(Align::CenterX);
    label.setTextAlignY(Align::CenterY);

    TextOptions& onAccent = theme.edit<TextOptions>("label.onAccent");
    onAccent.setCharSize(15);
    onAccent.setColorRole("onAccent");
    onAccent.setTextAlignX(Align::CenterX);
    onAccent.setTextAlignY(Align::CenterY);

    /* -------------------------------------------------------------------- */
    /*  Decoration                                                          */
    /* -------------------------------------------------------------------- */

    /* Character size stays unset on purpose: a Text with none sizes its glyphs
       from the element's height, so a Text in a fixed-height row scales with
       it. Pin one here and every Text becomes a fixed size instead. The type
       scale above is where fixed sizes belong. */
    TextOptions& text = theme.edit<TextOptions>();
    text.setColorRole("text");
    text.setTextAlignX(Align::Left);
    text.setTextAlignY(Align::Top);

    /* Stroke width stays unset: the built-in icons are drawn on a 24x24 grid
       at width 1.5, and an unset width keeps whatever the markup declares. */
    theme.edit<IconOptions>().setColorRole("text");

    theme.edit<WaveformOptions>().setColorRole("accent");
    theme.edit<WaveformOptions>().setBackgroundColorRole("panelAlt");

    /* -------------------------------------------------------------------- */
    /*  Interactible                                                        */
    /* -------------------------------------------------------------------- */

    /* Rounding is left unset so a Button used as a widget's internal part --
       a context-menu row, a date cell -- follows that widget's radius. */
    theme.edit<ButtonOptions>().setColorRole("panel");

    SliderOptions& slider = theme.edit<SliderOptions>();
    slider.setTrackColorRole("panelAlt");
    slider.setFillColorRole("accent");
    slider.setThumbColorRole("text");

    KnobOptions& knob = theme.edit<KnobOptions>();
    knob.setBodyColorRole("panel");
    knob.setTrackColorRole("panelAlt");
    knob.setArcColorRole("accent");
    knob.setIndicatorColorRole("text");

    /* Character size unset -- falls back to 18. */
    TextboxOptions& textbox = theme.edit<TextboxOptions>();
    textbox.setTextColorRole("text");
    textbox.setBackgroundColorRole("panelAlt");
    textbox.setPlaceholderColorRole("textDim");
    textbox.setCursorColorRole("text");
    textbox.setOutlineColorRole("accent");
    textbox.setLineNumberColorRole("textDim");
    textbox.setCurrentLineNumberColorRole("text");

    /* Character size unset -- falls back to 14. */
    DropdownOptions& dropdown = theme.edit<DropdownOptions>();
    dropdown.setHeaderColorRole("panel");
    dropdown.setPopupColorRole("panel");
    dropdown.setItemColorRole("panel");
    dropdown.setItemHoverColorRole("accent");
    dropdown.setTextColorRole("text");
    dropdown.setHeaderTextColorRole("text");
    dropdown.setHeaderOutlineColorRole("outline");
    dropdown.setPopupOutlineColorRole("outline");
    dropdown.setDividerColorRole("outline");

    /* -------------------------------------------------------------------- */
    /*  Composite widgets                                                   */
    /* -------------------------------------------------------------------- */

    ContextMenuOptions& contextMenu = theme.edit<ContextMenuOptions>();
    contextMenu.setColorRole("panel");
    contextMenu.setHoverColorRole("accent");
    contextMenu.setTextColorRole("text");
    contextMenu.setDisabledColorRole("textDim");
    contextMenu.setSeparatorColorRole("outline");
    contextMenu.setOutlineColorRole("outline");

    FileBrowserOptions& fileBrowser = theme.edit<FileBrowserOptions>();
    fileBrowser.setBackgroundColorRole("panel");
    fileBrowser.setOutlineColorRole("outline");
    fileBrowser.setHoverColorRole("panelAlt");
    fileBrowser.setSelectedColorRole("accent");
    fileBrowser.setFileTextColorRole("text");
    fileBrowser.setDirectoryTextColorRole("text");
    fileBrowser.setSelectedTextColorRole("onAccent");
    fileBrowser.setIconColorRole("textDim");
    fileBrowser.setDirectoryArrowColorRole("textDim");
    fileBrowser.setHeaderColorRole("panelAlt");
    fileBrowser.setHeaderTextColorRole("text");

    DatePickerOptions& datePicker = theme.edit<DatePickerOptions>();
    datePicker.setBackgroundColorRole("panel");
    datePicker.setOutlineColorRole("outline");
    datePicker.setHeaderTextColorRole("text");
    datePicker.setNavColorRole("panelAlt");
    datePicker.setNavHoverColorRole("accent");
    datePicker.setNavIconColorRole("text");
    datePicker.setWeekdayColorRole("textDim");
    datePicker.setCellHoverColorRole("panelAlt");
    datePicker.setDayTextColorRole("text");
    datePicker.setAdjacentTextColorRole("textDim");
    datePicker.setDisabledTextColorRole("textDim");
    datePicker.setSelectedColorRole("accent");
    datePicker.setSelectedTextColorRole("onAccent");
    datePicker.setTodayColorRole("panelAlt");
    datePicker.setTodayTextColorRole("text");
    datePicker.setButtonColorRole("panelAlt");
    datePicker.setButtonTextColorRole("text");
    datePicker.setConfirmButtonColorRole("accent");
    datePicker.setConfirmButtonTextColorRole("onAccent");

    DateFieldOptions& dateField = theme.edit<DateFieldOptions>();
    dateField.setBackgroundColorRole("panelAlt");
    dateField.setHoverColorRole("panel");
    dateField.setOutlineColorRole("outline");
    dateField.setTextColorRole("text");
    dateField.setPlaceholderColorRole("textDim");
    dateField.setIconColorRole("textDim");
    dateField.setChevronColorRole("textDim");
    /* A date field owns the picker it pops up, and that member never binds to a
       UILO of its own -- so hand it the picker prototype rather than leaving it
       on the baseline it was born with. */
    dateField.setPickerOptions(datePicker);

    TerminalOptions& terminal = theme.edit<TerminalOptions>();
    terminal.setBackgroundColorRole("bg");
    terminal.setForegroundColorRole("text");
    terminal.setCursorColorRole("accent");

    /* -------------------------------------------------------------------- */
    /*  Surfaces                                                            */
    /*                                                                      */
    /*  What panel() and card() are. Containers are transparent by default,  */
    /*  so nesting them to organise a layout costs nothing visually and only */
    /*  the ones given a role paint.                                        */
    /* -------------------------------------------------------------------- */

    theme.edit<ColumnOptions>("panel").setColorRole("panel");
    theme.edit<RowOptions>("panel").setColorRole("panel");

    ColumnOptions& card = theme.edit<ColumnOptions>("card");
    card.setColorRole("panel");
    card.setInnerPadding(16.f);
    card.setRounding(10.f);
    card.setOutlineColorRole("outline");
    card.setOutlineThickness(1.f);

    /* -------------------------------------------------------------------- */
    /*  Buttons                                                             */
    /* -------------------------------------------------------------------- */

    theme.edit<ButtonOptions>("primary").setColorRole("accent");

    ButtonOptions& ghost = theme.edit<ButtonOptions>("ghost");
    ghost.setColorRole("");
    ghost.setOutlineColorRole("outline");
    ghost.setOutlineThickness(1.f);

    return theme;
}

} // namespace uilo
