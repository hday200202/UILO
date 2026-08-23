#pragma once

#include "elements/Elements.hpp"
#include "../Page.hpp"

/*
    Factory.hpp:
    - Desc: Lowercase factory functions, one per element type, each returning a
            heap-allocated element. Allocation is deliberate and not a leak: an
            element is owned by the UILO it is bound to, which registers it in its
            element pool and deletes it at shutdown, so a tree reads as a nested
            declaration without anyone tracking lifetimes.
    - Every factory takes the same shape -- Modifier, then the type's Options,
      then children where the type has them, then an optional name -- so one
      spelling carries across the whole library.
*/

namespace uilo {

inline Column* column(
    Modifier modifier = {}, 
    ColumnOptions options = {}, 
    contains children = {}, 
    const std::string& name = ""
) { return new Column(modifier, options, children, name); }

inline Row* row(
    Modifier modifier = {}, 
    RowOptions options = {}, 
    contains children = {}, 
    const std::string& name = ""
) { return new Row(modifier, options, children, name); }

inline Canvas* canvas(
    Modifier modifier = {},
    CanvasOptions options = {},
    contains children = {},
    const std::string& name = ""
) { return new Canvas(modifier, options, children, name); }

inline Spacer* spacer(
    Modifier modifier = {}, 
    SpacerOptions options = {}, 
    const std::string& name = ""
) { return new Spacer(modifier, options, name); }

inline Image* image(
    Modifier modifier = {}, 
    ImageOptions options = {}, 
    const std::string& name = ""
) { return new Image(modifier, options, name); }

inline Icon* icon(
    Modifier modifier = {},
    IconOptions options = {},
    const std::string& name = ""
) { return new Icon(modifier, options, name); }

inline Text* text(
    Modifier modifier = {}, 
    TextOptions options = {}, 
    const std::string& name = ""
) { return new Text(modifier, options, name); }

inline Waveform* waveform(
    Modifier modifier = {},
    WaveformOptions options = {},
    const std::string& name = ""
) { return new Waveform(modifier, options, name); }

inline Page* page(
    Container* root, 
    const std::string& name
) { return new Page(root, name); }

inline Button* button(
    Modifier modifier = {}, 
    ButtonOptions options = {}, 
    const std::string& name = ""
) { return new Button(modifier, options, name); }

inline Slider* slider(
    Modifier modifier = {}, 
    SliderOptions options = {}, 
    const std::string& name = ""
) { return new Slider(modifier, options, name); }

inline Knob* knob(
    Modifier modifier = {},
    KnobOptions options = {},
    const std::string& name = ""
) { return new Knob(modifier, options, name); }

inline Dropdown* dropdown(
    Modifier modifier = {}, 
    DropdownOptions options = {},
    std::initializer_list<std::string> items = {},
    const std::string& name = ""
) { return new Dropdown(modifier, options, items, name); }

inline Resizer* resizer(
    Modifier modifier = {}, 
    ResizerOptions options = {}, 
    const std::string& name = ""
) { return new Resizer(modifier, options, name); }

inline Terminal* terminal(
    Modifier modifier = {},
    TerminalOptions options = {},
    const std::string& name = ""
) { return new Terminal(modifier, options, name); }

inline Textbox* textbox(
    Modifier modifier = {},
    TextboxOptions options = {},
    const std::string& name = ""
) { return new Textbox(modifier, options, name); }

inline FileBrowser* filebrowser(
    Modifier modifier = {},
    FileBrowserOptions options = {},
    const std::string& name = ""
) { return new FileBrowser(modifier, options, name); }

inline DatePicker* datepicker(
    Modifier modifier = {},
    DatePickerOptions options = {},
    const std::string& name = ""
) { return new DatePicker(modifier, options, name); }

inline DateField* datefield(
    Modifier modifier = {},
    DateFieldOptions options = {},
    const std::string& name = ""
) { return new DateField(modifier, options, name); }

/*
    Named-role helpers:
    - Desc:     The short spellings of the roles Defaults.hpp declares. Each is
                a one-line wrapper over the factory above, so

                    h1("Title")

                and

                    text(Modifier("h1"), TextOptions("h1").setContent("Title"))

                build the same element. Reach for the long form when an element
                needs something the role does not say; reach for the role
                directly when defining a variant of your own.
    - What any of these look like is in include/Defaults.hpp, not here. Change
      the "h1" prototype there and every h1() in the application follows.
*/

/*
    heading(std::string_view role, const std::string& content, const std::string& name):
    - Params:   std::string_view role, const std::string& content,
                const std::string& name
    - Returns:  Text*
    - Desc:     Shared body of h1/h2/h3 and the other text roles. Both halves of
                the role are looked up: the Modifier for the line box, the
                TextOptions for the glyphs.
*/
inline Text* heading(
    std::string_view role,
    const std::string& content,
    const std::string& name = ""
) {
    return text(Modifier(role), TextOptions(role).setContent(content), name);
}

inline Text* h1(const std::string& content, const std::string& name = "") {
    return heading("h1", content, name);
}

inline Text* h2(const std::string& content, const std::string& name = "") {
    return heading("h2", content, name);
}

inline Text* h3(const std::string& content, const std::string& name = "") {
    return heading("h3", content, name);
}

/* Paragraph copy: wraps, and sized for reading rather than for a heading. */
inline Text* body(const std::string& content, const std::string& name = "") {
    return heading("body", content, name);
}

/* The dimmed line above a control. */
inline Text* caption(const std::string& content, const std::string& name = "") {
    return heading("caption", content, name);
}

/* A filled surface. */
inline Column* panel(contains children = {}, const std::string& name = "") {
    return column(Modifier(), ColumnOptions("panel"), children, name);
}

/* A filled surface with a border, a radius and room around its contents. */
inline Column* card(contains children = {}, const std::string& name = "") {
    return column(Modifier(), ColumnOptions("card"), children, name);
}


/*
    labelledButton(std::string_view role, std::string_view labelRole, const std::string& caption, F&& onClick, const std::string& name):
    - Params:   std::string_view role, std::string_view labelRole,
                const std::string& caption, F&& onClick, const std::string& name
    - Returns:  Button*
    - Desc:     Shared body of primaryButton/ghostButton. The label is its own
                Text, so it follows the label role rather than repeating a size
                and an alignment at the call site.
    - The Modifier takes the same role as the Options, so a theme can give
      "primary" a width without every call site restating it.
*/
template <class F>
inline Button* labelledButton(
    std::string_view role,
    std::string_view labelRole,
    const std::string& caption,
    F&& onClick,
    const std::string& name = ""
) {
    return button(
        Modifier(role)
            .setOnLeftClick(std::forward<F>(onClick)),
        ButtonOptions(role)
            .setLabel(heading(labelRole, caption)),
        name
    );
}

/* The affirmative button: accent fill, text that reads against it. */
template <class F>
inline Button* primaryButton(
    const std::string& caption,
    F&& onClick,
    const std::string& name = ""
) {
    return labelledButton("primary", "label.onAccent", caption,
                          std::forward<F>(onClick), name);
}

/* The quiet button: no fill, just a border. */
template <class F>
inline Button* ghostButton(
    const std::string& caption,
    F&& onClick,
    const std::string& name = ""
) {
    return labelledButton("ghost", "label", caption,
                          std::forward<F>(onClick), name);
}

}
