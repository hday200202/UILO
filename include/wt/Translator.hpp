#pragma once
#ifdef UILO_WT

// Internal: walks a UILO element tree and builds the matching Wt widgets.
//
// The translation is structural, done once when a session starts. What it
// records per element is enough to re-derive that element's CSS later, which
// is how sync() reflects runtime changes (new text, a new colour, a hidden
// panel) without rebuilding anything.

#include <string>
#include <unordered_map>
#include <vector>

#include <Wt/WContainerWidget.h>

#include "UiloWt.hpp"

namespace uilo { class UILO; }

namespace uilo::wt::detail {

// The direction a container stacks its children. UILO's Row and Column differ
// only in which of width/height is the distributed axis, so almost every rule
// below is written once and parameterised by this.
enum class Axis { Row, Column };

// Rules that hang off an element's class rather than sitting in it: ":focus",
// "::placeholder", "::selection". Carried alongside the main declarations
// because they share the class, and so share its identity.
using PseudoRules = std::vector<std::pair<std::string, std::string>>;

class KnobWidget;

class Translator {
public:
    Translator(Wt::WApplication& app, UILO& uilo, const Config& config);

    // Builds the widgets for `page`'s root container as children of `into`.
    void build(Page& page, Wt::WContainerWidget* into);

    // Re-reads every translated element and applies whatever changed. Also
    // reconciles UILO's floating layer first, so opening or closing a popup is
    // reflected by the same call every event already makes.
    void sync();

private:
    /*
        Node:
        - Desc: One translated element, plus the layout context needed to
                recompute its CSS on its own. Storing the context (rather than
                re-walking the tree) is what lets sync() be a flat pass.
    */
    struct Node {
        Element*     element = nullptr;
        Wt::WWidget* widget  = nullptr;

        Axis axis            = Axis::Column;  // the *parent's* direction
        bool parentScrolls   = false;
        bool isRoot          = false;

        // "margin-left:auto;" and friends: how a centre- or end-aligned child
        // is pushed away from the ones before it. Empty for the common case.
        std::string autoMargin;

        // CSS length equal to this element's own drawn height, when it can be
        // derived from the tree ("48px", "calc(100vh * 0.5)"). Empty when the
        // height comes out of flex distribution and only the browser knows it.
        std::string heightExpr;

        // Last style applied, kept as both the declaration text (to detect a
        // change) and the class name it was interned as (to remove when it
        // does change -- Wt's own theme also writes to the class attribute,
        // so the bridge only ever adds and removes its own).
        std::string appliedCss;
        std::string appliedClass;
        std::string appliedText;  // Text/Button label content, for change detection
    };

    // Recursively translates `element` into a child widget of `parent`.
    void translate(Element* element, Wt::WContainerWidget* parent, Node node);

    // Orders a container's children into UILO's start/centre/end buckets and
    // translates each one.
    void translateChildren(Container* container, Wt::WContainerWidget* parent,
                           Axis axis, bool scrolls, const std::string& parentHeight);

    // Full CSS declaration text for a node, background and layout included.
    // Any pseudo-selector rules the element needs are appended to `pseudo`.
    std::string styleFor(const Node& node, PseudoRules& pseudo);

    // Injects a rule through the browser's ordinary CSS parser rather than
    // insertRule, so an unrecognised vendor selector is dropped instead of
    // throwing and taking the rest of the stylesheet with it.
    void addVendorRule(const std::string& rule);

    // Interns a style as a class name, so identical styling costs one rule.
    std::string classFor(const std::string& css, const PseudoRules& pseudo);

    // Maps a UILO font path to a CSS family, registering an @font-face for it
    // the first time it is seen. Empty path -> the configured default stack.
    std::string fontFamilyFor(const std::string& path);

    // Reproduces Image::init()'s aspect write-back: reads the file's real
    // dimensions and pins the derived axis to a fixed size, which is what
    // keeps the element out of its container's percent pool.
    void applyImageAspect(Image* image);

    // Builds the ring, body and pointer that make up a Knob, and wires up the
    // client-side drag.
    void buildKnob(Knob* knob, KnobWidget* widget);

    // Builds the drag strip inside a Resizer's (zero-width) wrapper and wires
    // up the client-side drag.
    void buildResizer(Resizer* resizer, Wt::WContainerWidget* wrapper, Axis axis);

    // Applies a node's current style, and its content when it has any.
    void apply(Node& node);

    // Shows the overlays whose backdrop is still floating and hides the rest,
    // translating any backdrop not seen before. Runs at the top of sync().
    void syncFloating(const std::vector<Element*>& floating);

    // Translates one floating backdrop into a fresh full-window overlay widget,
    // recording it so later syncFloating() calls can toggle it. Backdrops are
    // reused across open/close, so this runs once per distinct popup.
    void buildOverlay(Element* backdrop);

    /*
        FloatingLayer:
        - Desc: One popup mirrored on top of the page. Its nodes are kept apart
                from the page's so the layer applies (and stays styled while
                paged) independently, and its widget is only ever shown or
                hidden -- never deleted from under an event that is still
                dispatching inside it, which a click that closes its own popup
                would otherwise do.
    */
    struct FloatingLayer {
        Element*              backdrop = nullptr;
        Wt::WContainerWidget* widget   = nullptr;
        std::vector<Node>     nodes;
    };

    Wt::WApplication& m_app;
    UILO&             m_uilo;
    Config            m_config;

    std::vector<Node>                            m_nodes;
    std::vector<FloatingLayer>                   m_floatingLayers;
    std::unordered_map<std::string, std::string> m_classes;  // css text -> class
    std::unordered_map<std::string, std::string> m_fonts;    // ttf path -> family
    int                                          m_nextClass = 0;
    bool                                         m_resizerJs  = false;
    bool                                         m_autoGrowJs = false;
    bool                                         m_knobJs     = false;
    bool                                         m_sliderJs   = false;
};

} // namespace uilo::wt::detail

#endif // UILO_WT
