#ifdef UILO_WT

#include "Translator.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <fstream>

#include <Wt/WApplication.h>
#include <Wt/WComboBox.h>
#include <Wt/WCssStyleSheet.h>
#include <Wt/WImage.h>
#include <Wt/WJavaScript.h>
#include <Wt/WLineEdit.h>
#include <Wt/WLink.h>
#include <Wt/WSlider.h>
#include <Wt/WText.h>
#include <Wt/WTextArea.h>

#include "../elements/containers/Column.hpp"
#include "../elements/containers/Row.hpp"
#include "../elements/decoration/Image.hpp"
#include "../elements/decoration/Spacer.hpp"
#include "../elements/decoration/Text.hpp"
#include "../elements/interactible/Button.hpp"
#include "../elements/interactible/Dropdown.hpp"
#include "../elements/interactible/Knob.hpp"
#include "../elements/interactible/Resizer.hpp"
#include "../elements/interactible/Slider.hpp"
#include "../elements/interactible/Textbox.hpp"

namespace uilo::wt::detail {

/*
    KnobWidget:
    - Desc: A plain box that also owns the JSignal the drag script hands the
            settled value to. A knob is drawn entirely in CSS, so the only
            reason this is a subclass at all is that a JSignal has to live on
            a WObject.
*/
class KnobWidget : public Wt::WContainerWidget {
public:
    KnobWidget() : valueChanged(this, "knobValue") {}
    Wt::JSignal<double> valueChanged;
};

namespace {

// ---------------------------------------------------------------------------
// CSS value formatting
// ---------------------------------------------------------------------------

std::string num(float v) {
    if (std::fabs(v - std::round(v)) < 1e-4f)
        return std::to_string(static_cast<long long>(std::llround(v)));

    char buf[32];
    std::snprintf(buf, sizeof buf, "%.4f", v);
    std::string s(buf);
    while (!s.empty() && s.back() == '0') s.pop_back();
    if (!s.empty() && s.back() == '.') s.pop_back();
    return s;
}

std::string px(float v)  { return num(v) + "px"; }
std::string pct(float v) { return num(v) + "%"; }

std::string rgba(Color c) {
    if (c.a == 0)   return "transparent";
    if (c.a == 255) return "rgb(" + num(c.r) + "," + num(c.g) + "," + num(c.b) + ")";
    return "rgba(" + num(c.r) + "," + num(c.g) + "," + num(c.b) + "," + num(c.a / 255.f) + ")";
}

// Same colour, explicit alpha. Materials need this: the shader's body opacity
// is separate from the tint's own alpha.
std::string rgbaWith(Color c, float alpha) {
    return "rgba(" + num(c.r) + "," + num(c.g) + "," + num(c.b) + ","
         + num(std::clamp(alpha, 0.f, 1.f)) + ")";
}

// ---------------------------------------------------------------------------
// Alignment
// ---------------------------------------------------------------------------

// Cross-axis placement, in the order Element::resize() tests the flags: the
// edge flags win over the centre flag when both are set.
const char* crossAlign(Align a, Axis parentAxis) {
    if (parentAxis == Axis::Row) {                     // cross axis is vertical
        if (hasAlign(a, Align::Top))     return "flex-start";
        if (hasAlign(a, Align::Bottom))  return "flex-end";
        if (hasAlign(a, Align::CenterY)) return "center";
        return "flex-start";
    }
    if (hasAlign(a, Align::Left))    return "flex-start";
    if (hasAlign(a, Align::Right))   return "flex-end";
    if (hasAlign(a, Align::CenterX)) return "center";
    return "flex-start";
}

// Which of a container's three layout groups a child belongs to: 0 = start,
// 1 = centre, 2 = end. Mirrors the bucket test in Row/Column::update().
int bucketOf(Align a, Axis axis) {
    if (axis == Axis::Row) {
        if (hasAlign(a, Align::Right))   return 2;
        if (hasAlign(a, Align::CenterX)) return 1;
        return 0;
    }
    if (hasAlign(a, Align::Bottom))  return 2;
    if (hasAlign(a, Align::CenterY)) return 1;
    return 0;
}

// Text's own alignment is a single enum value, not a flag set -- Text::render()
// switches on it exactly -- so these compare rather than test bits.
const char* textJustify(Align a) {
    if (a == Align::CenterX) return "center";
    if (a == Align::Right)   return "flex-end";
    return "flex-start";
}

const char* textAlign(Align a) {
    if (a == Align::CenterX) return "center";
    if (a == Align::Right)   return "right";
    return "left";
}

const char* textAlignItems(Align a) {
    if (a == Align::CenterY) return "center";
    if (a == Align::Bottom)  return "flex-end";
    return "flex-start";
}

// ---------------------------------------------------------------------------
// Element introspection
// ---------------------------------------------------------------------------

bool isRowLike(ElementType t) {
    return t == ElementType::Row || t == ElementType::ScrollableRow ||
           t == ElementType::Button;
}

Axis axisOf(Element* el) {
    return isRowLike(el->getType()) ? Axis::Row : Axis::Column;
}

// Row, Column and Button all carry the same background vocabulary but on
// unrelated options types, so each caller unpacks it into this.
struct Background {
    Color       color{0, 0, 0, 0};
    std::string colorRole;
    Gradient    gradient;
    std::string gradientRole;
    float       rounding   = 0.f;
    bool        scrollable = false;
};

bool backgroundOf(Element* el, Background& out) {
    switch (el->getType()) {
        case ElementType::Button: {
            const auto& o = static_cast<Button*>(el)->getOptions();
            out = { o.getColor(), o.getColorRole(), o.getGradient(),
                    o.getGradientRole(), o.getRounding(), false };
            return true;
        }
        case ElementType::Row:
        case ElementType::ScrollableRow: {
            const auto& o = static_cast<Row*>(el)->getOptions();
            out = { o.getColor(), o.getColorRole(), o.getGradient(),
                    o.getGradientRole(), o.getRounding(), o.getScrollable() };
            return true;
        }
        case ElementType::Column:
        case ElementType::ScrollableColumn: {
            const auto& o = static_cast<Column*>(el)->getOptions();
            out = { o.getColor(), o.getColorRole(), o.getGradient(),
                    o.getGradientRole(), o.getRounding(), o.getScrollable() };
            return true;
        }
        case ElementType::Spacer: {
            const auto& o = static_cast<Spacer*>(el)->getOptions();
            out = { o.getColor(), o.getColorRole(), {}, {}, o.getRounding(), false };
            return true;
        }
        default:
            return false;
    }
}

// With either lock set, Image::update() overwrites one axis of its bounds with
// (other axis / aspect), so the drawn box has the picture's own proportions no
// matter what size the layout hands it.
bool aspectLocked(const ImageOptions& o) {
    return o.getLockAspectWidth() || o.getLockAspectHeight();
}

/*
    KnobGeometry:
    - Desc: The knob's arc laid out the way CSS wants it. UILO measures angles
            from +x with y running down; a conic-gradient starts at 12 o'clock
            and sweeps clockwise, which is the same direction -- just rotated a
            quarter turn, hence the +90 everywhere. `span` is always positive
            because a gradient's stops must ascend, so a clockwise knob starts
            from the far end and fills backwards (`rev`).
*/
struct KnobGeometry {
    float from  = 0.f;   // gradient origin, CSS degrees
    float span  = 0.f;   // total sweep, always positive
    float fill  = 0.f;   // boundary between arc and track, from `from`
    float rot   = 0.f;   // indicator rotation, CSS degrees
    float t     = 0.f;   // normalised value
    bool  rev   = false;
};

KnobGeometry knobGeometry(Knob* k) {
    const KnobOptions& o = k->getOptions();
    const float sweep = k->sweepDegrees();
    const float range = o.getMax() - o.getMin();

    KnobGeometry g;
    g.rev  = sweep < 0.f;
    g.span = std::fabs(sweep);
    g.t    = range > 0.f
           ? std::clamp((k->getValue() - o.getMin()) / range, 0.f, 1.f)
           : 0.f;
    g.from = o.getStartAngle() + 90.f + (g.rev ? sweep : 0.f);
    g.fill = (g.rev ? 1.f - g.t : g.t) * g.span;
    g.rot  = o.getStartAngle() + g.t * sweep + 90.f;   // == angleForValue + 90
    return g;
}

// Reads just the dimensions out of an image file's header. Image::init() gets
// these from the decoded texture; the web backend has no decoder, but the
// header is all that's needed and every one of these formats puts it up front.
bool imageSize(const std::string& path, uint32_t& w, uint32_t& h) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;

    unsigned char b[32] = {};
    in.read(reinterpret_cast<char*>(b), sizeof b);
    const std::streamsize got = in.gcount();
    auto be16 = [](const unsigned char* p) { return uint32_t(p[0]) << 8 | p[1]; };
    auto be32 = [](const unsigned char* p) {
        return uint32_t(p[0]) << 24 | uint32_t(p[1]) << 16 | uint32_t(p[2]) << 8 | p[3];
    };
    auto le16 = [](const unsigned char* p) { return uint32_t(p[1]) << 8 | p[0]; };
    auto le32 = [](const unsigned char* p) {
        return uint32_t(p[3]) << 24 | uint32_t(p[2]) << 16 | uint32_t(p[1]) << 8 | p[0];
    };

    // PNG: 8-byte signature, then an IHDR whose first two fields are the size.
    if (got >= 24 && b[0] == 0x89 && b[1] == 'P' && b[2] == 'N' && b[3] == 'G') {
        w = be32(b + 16);
        h = be32(b + 20);
        return w && h;
    }
    // GIF: "GIF87a"/"GIF89a" then the logical screen size, little-endian.
    if (got >= 10 && b[0] == 'G' && b[1] == 'I' && b[2] == 'F') {
        w = le16(b + 6);
        h = le16(b + 8);
        return w && h;
    }
    // BMP: DIB header carries a signed 32-bit size; height may be negative for
    // a top-down bitmap, so take the magnitude.
    if (got >= 26 && b[0] == 'B' && b[1] == 'M') {
        w = le32(b + 18);
        h = static_cast<uint32_t>(std::abs(static_cast<int32_t>(le32(b + 22))));
        return w && h;
    }
    // JPEG: no fixed offset -- walk the segment chain to the frame header.
    if (got >= 2 && b[0] == 0xFF && b[1] == 0xD8) {
        in.clear();
        in.seekg(2);
        while (in) {
            int marker = in.get();
            if (marker != 0xFF) continue;             // resync on the next 0xFF
            while (in && (marker = in.get()) == 0xFF) {}
            if (marker < 0) return false;
            // Standalone markers carry no length payload.
            if (marker == 0xD8 || marker == 0xD9 || (marker >= 0xD0 && marker <= 0xD7))
                continue;

            unsigned char len[2];
            if (!in.read(reinterpret_cast<char*>(len), 2)) return false;
            const uint32_t segment = be16(len);
            if (segment < 2) return false;

            // SOF0..SOF15, minus the two that aren't frame headers.
            const bool isFrame = marker >= 0xC0 && marker <= 0xCF &&
                                 marker != 0xC4 && marker != 0xC8 && marker != 0xCC;
            if (isFrame) {
                unsigned char sof[5];
                if (!in.read(reinterpret_cast<char*>(sof), 5)) return false;
                h = be16(sof + 1);
                w = be16(sof + 3);
                return w && h;
            }
            in.seekg(segment - 2, std::ios::cur);
        }
    }
    return false;
}

// Mirror-image transform for a flipped Image, or empty.
std::string flipCss(const ImageOptions& o) {
    if (!o.getFlipH() && !o.getFlipV()) return {};
    return std::string("transform:scale(") + (o.getFlipH() ? "-1" : "1") + ","
         + (o.getFlipV() ? "-1" : "1") + ");";
}

bool isScrollable(Element* el) {
    Background bg;
    return backgroundOf(el, bg) && bg.scrollable;
}

// Textbox::lineHeight() is charSize * 1.2, so CSS can match it exactly. It has
// to be a definite number rather than `normal`: the auto-grow script converts a
// maxResizeLines cap into pixels and must agree with the rendered leading.
float textboxLineHeight(const TextboxOptions& o) {
    return static_cast<float>(o.getCharSize()) * 1.2f;
}

// A Wt slider is integral where UILO's is float, so the float range is carried
// as a whole number of steps. Using the declared step means one arrow-key press
// moves the slider exactly one step, the way it does natively; a continuous
// slider has no such anchor and gets a fixed resolution instead.
int sliderSteps(const SliderOptions& o) {
    const float span = o.getMax() - o.getMin();
    if (span <= 0.f) return 1;
    if (o.getStep() > 0.f)
        return std::max(1, static_cast<int>(std::lround(span / o.getStep())));
    return 1000;
}

// ---------------------------------------------------------------------------
// Backgrounds
// ---------------------------------------------------------------------------

// UILO interpolates four corner colours bilinearly on the GPU. CSS has no
// equivalent, but the two cases the Gradient API actually encourages -- a
// vertical or horizontal fade, via setTop/setBottom or setLeft/setRight --
// are plain linear-gradients. Anything genuinely four-cornered falls back to
// a diagonal through the two opposite corners.
std::string gradientCss(const Color c[4]) {
    const Color tl = c[0], tr = c[1], bl = c[2], br = c[3];

    if (tl == tr && bl == br)
        return "linear-gradient(to bottom," + rgba(tl) + "," + rgba(bl) + ")";
    if (tl == bl && tr == br)
        return "linear-gradient(to right," + rgba(tl) + "," + rgba(tr) + ")";
    return "linear-gradient(135deg," + rgba(tl) + "," + rgba(br) + ")";
}

// Materials are shader effects natively. The browser's nearest equivalent is
// a backdrop-filter, which covers the frosted-pane look the static kinds are
// after. The animated kinds (Shimmer, Aurora, Holographic, Liquid) keep their
// tint and blur but lose the animation.
void appendMaterial(std::string& css, const Material& mat, Color elementColor) {
    Color tint = mat.tint;
    float alpha = tint.a / 255.f;

    switch (mat.kind) {
        // These four take their colour from the element rather than the tint,
        // exactly as the renderer does.
        case Material::Kind::Tinted:
        case Material::Kind::Ripple:
        case Material::Kind::Hover:
        case Material::Kind::Blur:
            tint  = elementColor;
            alpha = mat.opacity;
            break;
        default:
            break;
    }

    const float blur = mat.kind == Material::Kind::Blur ? mat.blurRadius
                     : mat.kind == Material::Kind::Frosted ? 24.f
                     : 16.f;

    const std::string filter = "blur(" + px(blur) + ") saturate(" + num(mat.saturation)
                             + ") brightness(" + num(mat.brightness) + ")";

    css += "backdrop-filter:" + filter + ";";
    css += "-webkit-backdrop-filter:" + filter + ";";
    css += "background-color:" + rgbaWith(tint, alpha) + ";";
    css += "border-radius:" + px(mat.cornerRadius) + ";";

    // The 1px specular rim the glass shader draws around the panel.
    if (mat.edgeHighlight > 0.f)
        css += "box-shadow:inset 0 0 0 1px rgba(255,255,255,"
             + num(std::clamp(mat.edgeHighlight * 0.18f, 0.f, 1.f)) + ");";
}

} // namespace

// ---------------------------------------------------------------------------
// Translator
// ---------------------------------------------------------------------------

Translator::Translator(Wt::WApplication& app, const Config& config)
    : m_app(app), m_config(config) {}

// Identity of a style: the declarations plus every pseudo rule hanging off it,
// so two elements share a class only when both match.
std::string styleKey(const std::string& css, const PseudoRules& pseudo) {
    std::string key = css;
    for (const auto& [suffix, decls] : pseudo) {
        key += '|';
        key += suffix;
        key += '{';
        key += decls;
    }
    return key;
}

// Wt adds rules with CSSStyleSheet.insertRule, which *throws* on a selector the
// browser does not recognise -- and each engine only recognises its own
// vendor-prefixed pseudo-elements. One `::-moz-range-track` is therefore enough
// to abort the whole stylesheet in Chrome and leave the page blank. Appending
// text to a <style> element goes through the ordinary CSS parser instead, which
// drops rules it cannot parse and keeps the rest, which is the behaviour these
// rules were written expecting.
void Translator::addVendorRule(const std::string& rule) {
    std::string js;
    js.reserve(rule.size() + 16);
    for (char c : rule) {
        if (c == '\\' || c == '\'') js += '\\';
        if (c == '\n') { js += "\\n"; continue; }
        js += c;
    }
    m_app.doJavaScript(
        "(function(){var s=document.getElementById('uilo-vendor');"
        "if(!s){s=document.createElement('style');s.id='uilo-vendor';"
        "document.head.appendChild(s);}"
        "s.appendChild(document.createTextNode('" + js + "'));})();");
}

std::string Translator::classFor(const std::string& css, const PseudoRules& pseudo) {
    const std::string key = styleKey(css, pseudo);

    auto it = m_classes.find(key);
    if (it != m_classes.end()) return it->second;

    const std::string name = "u" + std::to_string(m_nextClass++);
    m_app.styleSheet().addRule("." + name, css);
    for (const auto& [suffix, decls] : pseudo) {
        const std::string selector = "." + name + suffix;
        // ::placeholder, ::selection and :focus are standard and safe to insert;
        // anything vendor-prefixed has to take the lenient route.
        if (suffix.rfind("::-", 0) == 0) addVendorRule(selector + "{" + decls + "}");
        else                             m_app.styleSheet().addRule(selector, decls);
    }

    m_classes.emplace(key, name);
    return name;
}

std::string Translator::fontFamilyFor(const std::string& path) {
    if (path.empty()) return m_config.fontFamily;

    auto it = m_fonts.find(path);
    if (it != m_fonts.end()) return it->second;

    const std::string family = "uilo-font-" + std::to_string(m_fonts.size());
    // UILO loads the .ttf off disk by this path; the browser needs it fetched
    // over HTTP. The path is reused unchanged as a URL, so assets keep the same
    // relative layout under the server's docroot that they have beside a
    // desktop build's binary.
    m_app.styleSheet().addRule(
        "@font-face",
        "font-family:'" + family + "';src:url('" + path + "');font-display:swap;");

    // Keep the configured stack behind it so text still renders if the file is
    // missing or still loading.
    const std::string value = "'" + family + "'," + m_config.fontFamily;
    m_fonts.emplace(path, value);
    return value;
}

// Dragging has to be handled in the browser: routing every mousemove to the
// server and waiting for a re-render would make the splitter visibly lag the
// cursor. This mirrors Resizer::update() -- pick the sibling named by the
// direction, clamp to the configured min/max, snap to the step -- and writes
// the result straight to the target's flex-basis. Double-click drops the
// override, which is UILO's restore-original-size gesture.
const char kResizerJs[] = R"JS(
window.uiloResizer = function(hitId, o) {
  var hit = document.getElementById(hitId);
  if (!hit) return;
  var wrap = hit.parentElement, horiz = o.horiz;

  function sibling(step) {
    var n = step < 0 ? wrap.previousElementSibling : wrap.nextElementSibling;
    while (n && (n.classList.contains('uilo-resizer') ||
                 getComputedStyle(n).display === 'none'))
      n = step < 0 ? n.previousElementSibling : n.nextElementSibling;
    return n;
  }
  var target = sibling(o.prev ? -1 : 1);
  if (!target) return;

  function span() {
    var p = wrap.parentElement;
    return horiz ? p.clientWidth : p.clientHeight;
  }
  function resolve(d) { return d.pct ? d.v * span() / 100 : d.v; }

  var dragging = false, origin = 0, startSize = 0;

  hit.addEventListener('mousedown', function(e) {
    dragging = true;
    origin = horiz ? e.clientX : e.clientY;
    var r = target.getBoundingClientRect();
    startSize = horiz ? r.width : r.height;
    document.body.style.userSelect = 'none';
    document.body.style.cursor = horiz ? 'col-resize' : 'row-resize';
    e.preventDefault();
  });

  window.addEventListener('mousemove', function(e) {
    if (!dragging) return;
    var delta = (horiz ? e.clientX : e.clientY) - origin;
    // Dragging away from the target grows it when the target is behind the
    // handle, and shrinks it when the target is ahead of it.
    var size = startSize + (o.prev ? delta : -delta);
    if (o.step > 0) size = Math.round(size / o.step) * o.step;
    size = Math.max(resolve(o.min), Math.min(resolve(o.max), size));
    target.style.flex = '0 0 ' + size + 'px';
  });

  window.addEventListener('mouseup', function() {
    if (!dragging) return;
    dragging = false;
    document.body.style.userSelect = '';
    document.body.style.cursor = '';
  });

  hit.addEventListener('dblclick', function() { target.style.flex = ''; });
};
)JS";

// Textbox::update() grows a multiline+wrap box to fit its content, capped at
// maxResizeLines and never below the height the modifier gave it. A textarea
// cannot do that in CSS alone, and doing it server-side would cost a round trip
// per keystroke, so it is measured in the browser.
const char kAutoGrowJs[] = R"JS(
window.uiloAutoGrow = function(id, o) {
  var el = document.getElementById(id);
  if (!el) return;
  var initial = 0;
  function fit() {
    var cs = getComputedStyle(el);
    var pad = parseFloat(cs.paddingTop) + parseFloat(cs.paddingBottom);
    if (!initial) initial = el.getBoundingClientRect().height;
    // Collapse to content height first, or scrollHeight just reports the
    // current (larger) box back.
    el.style.flex = '0 0 auto';
    el.style.height = 'auto';
    var want = el.scrollHeight;
    el.style.height = '';
    if (o.maxLines > 0) want = Math.min(want, o.maxLines * o.lineHeight + pad);
    el.style.flex = '0 0 ' + Math.max(initial, want) + 'px';
  }
  el.addEventListener('input', fit);
  fit();
};
)JS";

// Knob drag. UILO turns vertical mouse travel into value (up = increase, a full
// range per dragPixelsPerRange), and the same reasoning as the resizer applies:
// the readout has to follow the cursor, so the arc and pointer are repainted
// locally by writing the two CSS angles the stylesheet reads. Only the settled
// value goes back to C++, on release.
const char kKnobJs[] = R"JS(
window.uiloKnob = function(id, o) {
  var el = document.getElementById(id);
  if (!el) return;
  var value = o.value;

  function paint(v) {
    var t = o.max > o.min ? (v - o.min) / (o.max - o.min) : 0;
    t = Math.max(0, Math.min(1, t));
    el.style.setProperty('--k-fill', (o.rev ? 1 - t : t) * o.span + 'deg');
    el.style.setProperty('--k-rot', (o.start + t * o.sweep + 90) + 'deg');
  }

  var dragging = false, originY = 0, startValue = 0;

  el.addEventListener('mousedown', function(e) {
    dragging = true;
    originY = e.clientY;
    startValue = value;
    document.body.style.userSelect = 'none';
    document.body.style.cursor = 'ns-resize';
    e.preventDefault();
  });

  window.addEventListener('mousemove', function(e) {
    if (!dragging) return;
    var dy = originY - e.clientY;                  // up is positive, as in UILO
    var v = startValue + (dy / o.pxPerRange) * (o.max - o.min);
    if (o.step > 0) v = o.min + Math.round((v - o.min) / o.step) * o.step;
    value = Math.max(o.min, Math.min(o.max, v));
    paint(value);
  });

  window.addEventListener('mouseup', function() {
    if (!dragging) return;
    dragging = false;
    document.body.style.userSelect = '';
    document.body.style.cursor = '';
    // A click that never moved leaves the value alone, so there is nothing to
    // report -- UILO's knob only arms the drag on press, it does not jump.
    if (value !== startValue) Wt.emit(el.id, 'knobValue', value);
  });

  // Double-click snaps back to the default, matching Knob::checkLeftClick --
  // and, like it, only when one was actually set.
  el.addEventListener('dblclick', function() {
    if (!o.hasDefault) return;
    value = o.defaultValue;
    paint(value);
    Wt.emit(el.id, 'knobValue', value);
  });

  // Server-side changes arrive as new class rules; dropping the inline
  // overrides lets them through again.
  window.uiloKnobRelease = function(elId) {
    var e = document.getElementById(elId);
    if (!e) return;
    e.style.removeProperty('--k-fill');
    e.style.removeProperty('--k-rot');
  };

  paint(value);
};
)JS";

// The filled part of a slider track is a gradient stop, so it only moves when
// --s-t does. Wt reports a drag to the server on release, which would leave the
// fill lagging the thumb for the whole gesture; this keeps them together and
// lets the authoritative value still arrive the normal way.
const char kSliderJs[] = R"JS(
window.uiloSlider = function(id) {
  var el = document.getElementById(id);
  if (!el) return;
  function paint() {
    var lo = parseFloat(el.min), hi = parseFloat(el.max);
    var t = hi > lo ? (parseFloat(el.value) - lo) / (hi - lo) : 0;
    el.style.setProperty('--s-t', t);
  }
  el.addEventListener('input', paint);
  paint();
};
window.uiloSliderRelease = function(id) {
  var el = document.getElementById(id);
  if (el) el.style.removeProperty('--s-t');
};
)JS";

// A Dimension as the {v, pct} pair the script above expects.
std::string jsDimension(Dimension d) {
    return "{v:" + num(d.value) + ",pct:" + (d.percent ? "true" : "false") + "}";
}

// The element's drawn height as a CSS length, when the tree alone determines
// it. Used to size text that has no explicit charSize, which UILO derives from
// the box height. Empty means "only the browser knows", and callers fall back.
std::string ownHeightExpr(Element* el, Axis parentAxis, bool parentScrolls,
                          const std::string& parentHeight) {
    const Modifier& mod = el->getModifier();
    const Dimension h   = mod.getHeight();
    const float op      = mod.getOuterPadding();

    if (!h.percent) return px(h.value - 2.f * op);

    // A percentage only follows from the parent when the parent is not
    // distributing this axis by flex: either height is the cross axis (the
    // parent is a Row), or the parent scrolls and children keep natural size.
    if (parentHeight.empty() || (parentAxis != Axis::Row && !parentScrolls))
        return {};

    std::string expr = "calc(" + parentHeight + " * " + num(h.value / 100.f) + ")";
    if (op > 0.f) expr = "calc(" + expr + " - " + px(2.f * op) + ")";
    return expr;
}

std::string Translator::styleFor(const Node& n, PseudoRules& pseudo) {
    Element* el   = n.element;
    Modifier& mod = el->getModifier();
    std::string css;

    // Hidden elements need nothing else, and this has to be the *only* display
    // declaration in the rule: the container and text branches below both emit
    // `display:flex`, and a later declaration would quietly win. UILO drops
    // invisible children from its layout pass outright, which is exactly what
    // display:none does to a flex item.
    if (!mod.getVisible()) return "display:none;";

    // A Resizer is invisible to layout in UILO -- its siblings are placed as if
    // it were not there, and it draws on top at the boundary between them. A
    // zero-size flex item reproduces both halves of that: it consumes no space,
    // and its absolutely-positioned strip straddles the edge it sits on.
    if (el->getType() == ElementType::Resizer)
        return "flex:0 0 0;align-self:stretch;position:relative;z-index:5;";

    const float op = mod.getOuterPadding();


    // ---- Box: size, spacing and placement within the parent ---------------
    if (n.isRoot) {
        css += "position:absolute;inset:0;";
    } else {
        const Dimension mainDim  = n.axis == Axis::Row ? mod.getWidth()  : mod.getHeight();
        const Dimension crossDim = n.axis == Axis::Row ? mod.getHeight() : mod.getWidth();
        const char* crossProp    = n.axis == Axis::Row ? "height" : "width";

        if (n.parentScrolls) {
            // A scrollable container sizes children against its viewport and
            // lets them overflow, so nothing is distributed and nothing shrinks.
            css += "flex:0 0 " + (mainDim.percent ? pct(mainDim.value)
                                                  : px(mainDim.value - 2.f * op)) + ";";
        } else if (mainDim.percent) {
            // UILO gives each percent child value/totalPct of the space left
            // over once fixed-size siblings are placed. That is precisely how
            // flex-grow distributes free space, so the percentage carries over
            // as the grow factor unchanged.
            css += "flex:" + num(mainDim.value) + " 1 0;";
        } else {
            css += "flex:0 0 " + px(mainDim.value - 2.f * op) + ";";
        }

        if (crossDim.percent) {
            css += std::string(crossProp) + ":";
            css += op > 0.f ? "calc(" + pct(crossDim.value) + " - " + px(2.f * op) + ")"
                            : pct(crossDim.value);
            css += ";";
        } else {
            css += std::string(crossProp) + ":" + px(crossDim.value - 2.f * op) + ";";
        }

        // UILO shrinks an element by 2*outerPadding and insets it by one
        // padding on every side, which is exactly a margin. Always emitted,
        // even at zero: form controls carry a UA margin that would otherwise
        // survive, and a per-type reset further down would clobber a real
        // outerPadding set here.
        css += "margin:" + px(op) + ";";

        css += std::string("align-self:") + crossAlign(mod.getAlign(), n.axis) + ";";
        css += n.autoMargin;
    }

    // ---- Background --------------------------------------------------------
    Background bg;
    const bool hasBg = backgroundOf(el, bg);
    Color resolved{0, 0, 0, 0};

    if (hasBg) {
        resolved = el->resolveColor(bg.colorRole, bg.color);
        if (bg.rounding > 0.f) css += "border-radius:" + px(bg.rounding) + ";";
    }

    const Material& mat = mod.getMaterial();
    if (mat.kind != Material::Kind::None) {
        // The material owns the background when present -- the renderer skips
        // the solid fill entirely in that case.
        appendMaterial(css, mat, resolved);
    } else if (hasBg) {
        Color corners[4];
        if (el->resolveGradient(bg.gradient, bg.gradientRole, corners))
            css += "background-image:" + gradientCss(corners) + ";";
        else if (resolved.a > 0)
            css += "background-color:" + rgba(resolved) + ";";
    }

    // ---- Per-type styling --------------------------------------------------
    switch (el->getType()) {
        case ElementType::Text: {
            auto* t = static_cast<Text*>(el);
            const auto& o = t->getOptions();

            css += "display:flex;";
            css += std::string("justify-content:") + textJustify(o.getTextAlignX()) + ";";
            css += std::string("align-items:") + textAlignItems(o.getTextAlignY()) + ";";
            css += std::string("text-align:") + textAlign(o.getTextAlignX()) + ";";
            css += "font-family:" + fontFamilyFor(o.getFontPath()) + ";";

            const float k = m_config.charSizeToFontSize;
            if (o.hasCharSize()) {
                css += "font-size:" + px(o.getCharSize() * k) + ";";
            } else if (!n.heightExpr.empty()) {
                // Text with no explicit size takes 60% of its box height.
                css += "font-size:calc(" + n.heightExpr + " * " + num(0.6f * k) + ");";
            } else {
                css += "font-size:" + px(30.f * k) + ";";
            }

            css += "color:" + rgba(el->resolveColor(o.getColorRole(), o.getColor())) + ";";
            if (o.getBold())   css += "font-weight:700;";
            if (o.getItalic()) css += "font-style:italic;";
            if (o.getUnderlined() && o.getStrikeThrough())
                css += "text-decoration:underline line-through;";
            else if (o.getUnderlined())    css += "text-decoration:underline;";
            else if (o.getStrikeThrough()) css += "text-decoration:line-through;";

            // UILO only wraps when asked; otherwise the string is drawn on one
            // line and clipped. Embedded newlines break lines either way.
            css += o.getWrap() ? "white-space:pre-wrap;" : "white-space:pre;";
            css += "overflow:hidden;";
            break;
        }

        case ElementType::TextBox: {
            const auto& o = static_cast<Textbox*>(el)->getOptions();

            css += "font-family:" + fontFamilyFor(o.getFontPath()) + ";";
            css += "font-size:" + px(o.getCharSize() * m_config.charSizeToFontSize) + ";";
            css += "line-height:" + px(textboxLineHeight(o)) + ";";
            css += "color:" + rgba(el->resolveColor(o.getTextColorRole(), o.getTextColor())) + ";";
            css += "background-color:"
                 + rgba(el->resolveColor(o.getBackgroundColorRole(), o.getBackgroundColor())) + ";";
            css += "border-radius:" + px(o.getRounding()) + ";";
            css += "padding:" + px(o.getPaddingTop()) + " " + px(o.getPaddingRight()) + " "
                 + px(o.getPaddingBottom()) + " " + px(o.getPaddingLeft()) + ";";
            css += std::string("text-align:") + textAlign(o.getTextAlignX()) + ";";
            if (o.getBold())   css += "font-weight:700;";
            if (o.getItalic()) css += "font-style:italic;";

            css += "caret-color:"
                 + rgba(el->resolveColor(o.getCursorColorRole(), o.getCursorColor())) + ";";
            css += "border:none;outline:none;";
            // A browser textarea offers a drag-to-resize grip. UILO has no such
            // gesture -- the only sizing it does is the automatic growth below --
            // so the grip is turned off.
            css += "resize:none;";

            if (o.getMultiline()) {
                // wrap=false is the "code editor" shape: long lines stay intact
                // and scroll sideways, which is what shouldWrap() gates on.
                css += o.getWrap()
                    ? "white-space:pre-wrap;overflow-wrap:break-word;overflow-x:hidden;"
                    : "white-space:pre;overflow-x:auto;";
                css += "overflow-y:auto;";
            } else {
                // Single line is the search-bar shape: no wrapping, no newline,
                // and Enter submits -- handleKeyInput() only fires
                // onEnterPressed when multiline is off.
                css += "white-space:pre;overflow:hidden;";
            }

            pseudo.emplace_back("::placeholder",
                "color:" + rgba(el->resolveColor(o.getPlaceholderColorRole(),
                                                 o.getPlaceholderColor())) + ";");
            pseudo.emplace_back("::selection",
                "background-color:" + rgba(el->resolveColor(o.getSelectionColorRole(),
                                                            o.getSelectionColor())) + ";");
            if (o.getOutlineThickness() > 0.f) {
                // Drawn inside the box, as UILO's focus outline is.
                const std::string t = px(o.getOutlineThickness());
                pseudo.emplace_back(":focus",
                    "outline:" + t + " solid "
                    + rgba(el->resolveColor(o.getOutlineColorRole(), o.getOutlineColor()))
                    + ";outline-offset:-" + t + ";");
            }
            break;
        }

        case ElementType::Dropdown: {
            const auto& o = static_cast<Dropdown*>(el)->getOptions();
            css += "font-family:" + fontFamilyFor(o.getFontPath()) + ";";
            css += "font-size:" + px(o.getCharSize() * m_config.charSizeToFontSize) + ";";
            css += "color:" + rgba(el->resolveColor(o.getHeaderTextColorRole(),
                                                    o.getHeaderTextColor())) + ";";
            css += "background-color:" + rgba(el->resolveColor(o.getHeaderColorRole(),
                                                               o.getHeaderColor())) + ";";
            css += "border-radius:" + px(o.getHeaderRounding()) + ";";
            css += "border:none;outline:none;padding:0 8px;";
            break;
        }

        case ElementType::Slider: {
            auto* sl = static_cast<Slider*>(el);
            const auto& o = sl->getOptions();
            const bool horiz = o.getOrientation() == SliderOrientation::Horizontal;

            // accent-color would colour a stock range input, but it cannot
            // resize the thumb -- and UILO's thumb is an arbitrary rectangle
            // (16x48 in the containers example) rather than the browser's small
            // circle. Opting out of the native appearance is the only way to
            // reach it, which means the track and its fill have to be drawn
            // here too.
            css += "-webkit-appearance:none;appearance:none;";
            css += "background:transparent;padding:0;border:none;";
            css += "overflow:visible;";   // a tall thumb overhangs the box

            // Slider::render(): thickness <= 1 is a fraction of the box's cross
            // axis, > 1 is pixels. The fraction needs the box size, which the
            // node knows when the tree pins it down.
            const float thickness = o.getTrackThickness();
            std::string track;
            if (thickness > 1.f)              track = px(thickness);
            else if (horiz && !n.heightExpr.empty())
                                              track = "calc(" + n.heightExpr + " * " + num(thickness) + ")";
            else                              track = pct(thickness * 100.f);

            // Half the thumb, which is both the fill's end-cap and the inset
            // the value maps into -- resolveThumbHalfWidth/Height().
            const bool circle = o.getThumbShape() == ThumbShape::Circle;
            std::string halfW, thumbW, thumbH, radius;
            if (circle) {
                const float d = o.getThumbSize().x;
                const std::string r = d > 0.f
                    ? px(d * 0.5f)
                    : (!n.heightExpr.empty() ? "calc(" + n.heightExpr + " * 0.4)" : "50%");
                halfW  = r;
                thumbW = "calc(" + r + " * 2)";
                thumbH = thumbW;
                radius = "50%";
            } else {
                halfW  = px(o.getThumbSize().x * 0.5f);
                thumbW = px(o.getThumbSize().x);
                thumbH = px(o.getThumbSize().y);
                radius = px(o.getThumbRounding());
            }
            if (!horiz) std::swap(thumbW, thumbH);

            const std::string fill  = rgba(el->resolveColor(o.getFillColorRole(),  o.getFillColor()));
            const std::string bed   = rgba(el->resolveColor(o.getTrackColorRole(), o.getTrackColor()));
            const std::string knobC = rgba(el->resolveColor(o.getThumbColorRole(), o.getThumbColor()));

            // fillW = hw + t * (trackW - 2*hw), straight from render(). --s-t is
            // the normalised value, updated locally while dragging.
            const std::string stop = "calc(" + halfW + " + var(--s-t) * (100% - "
                                   + halfW + " * 2))";
            const std::string bar  = std::string("linear-gradient(to ")
                                   + (horiz ? "right" : "top") + "," + fill + " 0 " + stop
                                   + "," + bed + " " + stop + " 100%)";

            const float t0 = (o.getMax() > o.getMin())
                ? std::clamp((sl->getValue() - o.getMin()) / (o.getMax() - o.getMin()), 0.f, 1.f)
                : 0.f;
            css += "--s-t:" + num(t0) + ";";

            std::string trackCss = (horiz ? "height:" : "width:") + track + ";";
            trackCss += "border-radius:" + px(o.getTrackRounding()) + ";background:" + bar + ";";

            std::string thumbCss = "-webkit-appearance:none;appearance:none;";
            thumbCss += "width:" + thumbW + ";height:" + thumbH + ";";
            thumbCss += "border:none;border-radius:" + radius + ";background:" + knobC + ";";
            // Centre the thumb across the track it straddles.
            thumbCss += (horiz ? "margin-top:calc((" : "margin-left:calc((")
                      + track + " - " + (horiz ? thumbH : thumbW) + ") / 2);";

            pseudo.emplace_back("::-webkit-slider-runnable-track", trackCss);
            pseudo.emplace_back("::-webkit-slider-thumb", thumbCss);
            pseudo.emplace_back("::-moz-range-track", trackCss);
            pseudo.emplace_back("::-moz-range-thumb", thumbCss);
            break;
        }

        case ElementType::Image: {
            const auto& o = static_cast<Image*>(el)->getOptions();
            if (aspectLocked(o)) {
                // The element still consumes the layout slot UILO gives it --
                // Row/Column advance by the modifier's size, not the shrunken
                // bounds -- so this stays a plain flex item and the picture
                // sits inside it. Placed at the slot's start because
                // Image::update() runs resize() (which is what applies the
                // align flags) *before* it overwrites the derived axis.
                css += "display:flex;";
                css += n.axis == Axis::Row ? "flex-direction:row;"
                                           : "flex-direction:column;";
                css += "justify-content:flex-start;align-items:flex-start;";
                css += "overflow:hidden;";
            } else {
                // No lock: UILO stretches the texture over the whole box.
                css += "object-fit:fill;";
                if (o.getClipEllipse()) css += "border-radius:50%;";
                css += flipCss(o);
            }
            break;
        }

        case ElementType::Button:
            css += "cursor:pointer;user-select:none;";
            break;

        case ElementType::Knob: {
            // The children are positioned against this box, and they read the
            // two angles from here -- so a server-side setValue() flows out
            // through the ordinary class swap, with no extra plumbing.
            const KnobGeometry g = knobGeometry(static_cast<Knob*>(el));
            css += "position:relative;cursor:ns-resize;user-select:none;";
            css += "--k-from:" + num(g.from) + "deg;";
            css += "--k-span:" + num(g.span) + "deg;";
            css += "--k-fill:" + num(g.fill) + "deg;";
            css += "--k-rot:"  + num(g.rot)  + "deg;";
            break;
        }

        default:
            break;
    }

    // ---- Container flow ----------------------------------------------------
    if (dynamic_cast<Container*>(el)) {
        css += "display:flex;";
        css += isRowLike(el->getType()) ? "flex-direction:row;" : "flex-direction:column;";
        css += "align-items:flex-start;";

        // UILO clips children to the container's (rounded) bounds; a scrollable
        // one instead lets them overflow along its own axis.
        if (bg.scrollable) {
            css += isRowLike(el->getType()) ? "overflow-x:auto;overflow-y:hidden;"
                                            : "overflow-y:auto;overflow-x:hidden;";
            // Hide the native scrollbar while keeping the container scrollable
            // (wheel/touch/keys still work). UILO will grow its own scrollbar
            // later; until then the web build shows none, for consistency.
            //   Firefox: scrollbar-width      legacy Edge: -ms-overflow-style
            //   WebKit/Chromium: ::-webkit-scrollbar { display:none }
            css += "scrollbar-width:none;-ms-overflow-style:none;";
            pseudo.emplace_back("::-webkit-scrollbar", "display:none;");
        } else {
            css += "overflow:hidden;";
        }
    }

    return css;
}

void Translator::applyImageAspect(Image* img) {
    const ImageOptions& o = img->getOptions();
    if (!aspectLocked(o) || o.getPath().empty()) return;

    Modifier& mod = img->getModifier();
    const Dimension w = mod.getWidth();
    const Dimension h = mod.getHeight();

    // Same guards as Image::init(): the driving axis has to be a concrete
    // pixel size for a derived one to mean anything.
    const bool byWidth  = o.getLockAspectWidth()  && !w.percent;
    const bool byHeight = o.getLockAspectHeight() && !h.percent;
    if (!byWidth && !byHeight) return;

    // The path is a URL to the browser, so it is relative to the server's
    // docroot; a desktop build reads the same relative path from its working
    // directory, which is the fallback.
    uint32_t iw = 0, ih = 0;
    const std::string docRoot = m_app.docRoot();
    if (!(!docRoot.empty() && imageSize(docRoot + "/" + o.getPath(), iw, ih)) &&
        !imageSize(o.getPath(), iw, ih))
        return;                       // unreadable: leave the modifier alone

    const float aspect = static_cast<float>(iw) / static_cast<float>(ih);
    if (!(aspect > 0.f)) return;

    // This is Image::init() verbatim, and it matters for far more than the
    // image itself: writing a fixed size back into the Modifier takes the
    // element out of the percent pool, so its Row/Column stops handing it all
    // the leftover space and the *siblings* get centred instead. Skipping it
    // silently pins the whole group to one edge.
    if (byWidth)  mod.setHeight(Dimension{ w.value / aspect, false });
    else          mod.setWidth (Dimension{ h.value * aspect, false });
}

void Translator::buildKnob(Knob* k, KnobWidget* w) {
    if (!m_knobJs) {
        m_app.doJavaScript(kKnobJs);
        m_knobJs = true;
    }

    const KnobOptions& o = k->getOptions();
    const KnobGeometry g = knobGeometry(k);

    // Knob::render() takes the outer radius from the box, then reserves the arc
    // and its gap so the ring sits *around* the body rather than over it.
    const float arcThick = std::max(0.f, o.getArcThickness());
    const float arcGap   = std::max(0.f, o.getArcGap());
    const float reserve  = arcThick + arcGap;      // outerR - bodyR

    auto colour = [&](const std::string& role, Color literal) {
        return rgba(k->resolveColor(role, literal));
    };

    // ---- ring: track and filled arc in one conic sweep ---------------------
    // The stops run in gradient order, so a reversed knob lists the track
    // first and the arc after it.
    const std::string arcCol   = colour(o.getArcColorRole(),   o.getArcColor());
    const std::string trackCol = colour(o.getTrackColorRole(), o.getTrackColor());
    const std::string stops = g.rev
        ? trackCol + " 0deg var(--k-fill)," + arcCol   + " var(--k-fill) var(--k-span)"
        : arcCol   + " 0deg var(--k-fill)," + trackCol + " var(--k-fill) var(--k-span)";

    // Masked to an annulus arcThickness wide at the outer edge; closest-side
    // makes 100% mean the radius.
    const std::string ring = "radial-gradient(closest-side,transparent calc(100% - "
                           + px(arcThick) + "),#000 calc(100% - " + px(arcThick) + "))";

    std::string ringCss = "position:absolute;inset:0;border-radius:50%;pointer-events:none;";
    ringCss += "background:conic-gradient(from var(--k-from)," + stops
             + ",transparent var(--k-span) 360deg);";
    ringCss += "-webkit-mask:" + ring + ";mask:" + ring + ";";

    // ---- body: the disc inside the ring ------------------------------------
    std::string bodyCss = "position:absolute;left:50%;top:50%;"
                          "transform:translate(-50%,-50%);border-radius:50%;"
                          "pointer-events:none;";
    bodyCss += "width:calc(100% - " + px(reserve * 2.f) + ");";
    bodyCss += "height:calc(100% - " + px(reserve * 2.f) + ");";
    bodyCss += "background:" + colour(o.getBodyColorRole(), o.getBodyColor()) + ";";
    if (o.getOutlineThickness() > 0.f) {
        // Drawn outside the body radius, matching the outline circle in render().
        bodyCss += "box-shadow:0 0 0 " + px(o.getOutlineThickness()) + " "
                 + colour(o.getOutlineColorRole(), o.getOutlineColor()) + ";";
    }

    // ---- indicator: a pointer from inset*bodyR out to length*bodyR ---------
    const float inset = std::clamp(o.getIndicatorInset(),  0.f, 1.f);
    const float len   = std::clamp(o.getIndicatorLength(), 0.f, 1.f);
    const float thick = o.getIndicatorThickness();
    // bodyR is `50% - reserve`, so the two radii fall out of that directly.
    const std::string bodyR = "(50% - " + px(reserve) + ")";

    std::string rotCss = "position:absolute;inset:0;pointer-events:none;"
                         "transform:rotate(var(--k-rot));";
    std::string indCss = "position:absolute;left:50%;transform:translateX(-50%);";
    indCss += "width:" + px(thick) + ";border-radius:" + px(thick * 0.5f) + ";";
    indCss += "top:calc(50% - " + bodyR + " * " + num(len) + ");";
    indCss += "height:calc(" + bodyR + " * " + num(len - inset) + ");";
    indCss += "background:" + colour(o.getIndicatorColorRole(), o.getIndicatorColor()) + ";";

    if (arcThick > 0.f)
        w->addWidget(std::make_unique<Wt::WContainerWidget>())
         ->addStyleClass(classFor(ringCss, {}));
    w->addWidget(std::make_unique<Wt::WContainerWidget>())
     ->addStyleClass(classFor(bodyCss, {}));
    if (thick > 0.f) {
        auto* rot = w->addWidget(std::make_unique<Wt::WContainerWidget>());
        rot->addStyleClass(classFor(rotCss, {}));
        rot->addWidget(std::make_unique<Wt::WContainerWidget>())
           ->addStyleClass(classFor(indCss, {}));
    }

    // setValue() re-applies the step and range clamp and fires onValueChanged,
    // exactly as a native drag would; sync() then repaints from the class and
    // the inline overrides are dropped in the same response, so there is no
    // frame where the two disagree.
    w->valueChanged.connect([this, k, w](double v) {
        k->setValue(static_cast<float>(v));
        m_app.doJavaScript("uiloKnobRelease('" + w->id() + "');");
        sync();
    });

    m_app.doJavaScript(
        "uiloKnob('" + w->id() + "',{"
        "value:" + num(k->getValue()) +
        ",min:"  + num(o.getMin()) + ",max:" + num(o.getMax()) +
        ",step:" + num(o.getStep()) +
        ",start:" + num(o.getStartAngle()) +
        ",sweep:" + num(k->sweepDegrees()) +
        ",span:"  + num(g.span) +
        ",rev:"   + (g.rev ? "true" : "false") +
        ",pxPerRange:" + num(o.getDragPixelsPerRange()) +
        ",hasDefault:" + (o.hasDefault() ? "true" : "false") +
        ",defaultValue:" + num(o.getDefaultValue()) + "});");
}

void Translator::buildResizer(Resizer* r, Wt::WContainerWidget* wrapper, Axis axis) {
    if (!m_resizerJs) {
        m_app.doJavaScript(kResizerJs);
        m_resizerJs = true;
    }

    const ResizerOptions& o = r->getOptions();
    const bool horiz = axis == Axis::Row;

    // The grab area is the modifier's size on the layout axis -- the same value
    // Row/Column use for the resizer's own bounds -- while getThickness() is
    // just the visible strip inside it.
    const Dimension hitDim = horiz ? r->getModifier().getWidth()
                                   : r->getModifier().getHeight();
    const float hit = hitDim.value;
    const float bar = o.getThickness();

    std::string hitCss = "position:absolute;display:flex;"
                         "align-items:center;justify-content:center;";
    hitCss += horiz
        ? "top:0;bottom:0;left:" + px(-hit * 0.5f) + ";width:" + px(hit) + ";cursor:col-resize;"
        : "left:0;right:0;top:" + px(-hit * 0.5f) + ";height:" + px(hit) + ";cursor:row-resize;";

    // UILO examples tend to leave the resizer transparent and fade a highlight
    // in from an onUpdateEnd handler, which is a per-frame hook the web backend
    // never runs. A neutral highlight stands in, faded in on hover by the
    // .uilo-resizer rule so the handle is still discoverable.
    Color c = r->resolveColor(o.getColorRole(), o.getColor());
    if (c.a == 0) c = Color{255, 255, 255, 100};

    std::string barCss = horiz ? "width:" + px(bar) + ";height:100%;"
                               : "height:" + px(bar) + ";width:100%;";
    barCss += "background-color:" + rgba(c) + ";border-radius:" + px(bar * 0.5f) + ";";

    auto* hitWidget = wrapper->addWidget(std::make_unique<Wt::WContainerWidget>());
    hitWidget->addStyleClass(classFor(hitCss, {}));

    auto* barWidget = hitWidget->addWidget(std::make_unique<Wt::WContainerWidget>());
    barWidget->addStyleClass("uilo-resizer-bar " + classFor(barCss, {}));

    const ResizerDir dir = o.getDirection();
    const bool prev = dir == ResizerDir::Left || dir == ResizerDir::Top;

    const Dimension mn = horiz ? o.getResizeWidthMin()  : o.getResizeHeightMin();
    const Dimension mx = horiz ? o.getResizeWidthMax()  : o.getResizeHeightMax();
    const Dimension st = horiz ? o.getResizeWidthStep() : o.getResizeHeightStep();

    m_app.doJavaScript(
        "uiloResizer('" + hitWidget->id() + "',{"
        "horiz:" + (horiz ? "true" : "false") +
        ",prev:" + (prev ? "true" : "false") +
        ",min:" + jsDimension(mn) +
        ",max:" + jsDimension(mx) +
        ",step:" + num(st.value) + "});");
}

void Translator::apply(Node& n) {
    PseudoRules pseudo;
    const std::string css = styleFor(n, pseudo);
    const std::string key = styleKey(css, pseudo);
    if (key != n.appliedCss) {
        const std::string cls = classFor(css, pseudo);
        if (!n.appliedClass.empty()) n.widget->removeStyleClass(n.appliedClass);
        n.widget->addStyleClass(cls);
        n.appliedClass = cls;
        n.appliedCss   = key;
    }

    switch (n.element->getType()) {
        case ElementType::Text: {
            const std::string& s = static_cast<Text*>(n.element)->getString();
            if (s != n.appliedText) {
                static_cast<Wt::WText*>(n.widget)->setText(Wt::WString::fromUTF8(s));
                n.appliedText = s;
            }
            break;
        }
        case ElementType::Slider: {
            auto* sl = static_cast<Slider*>(n.element);
            auto* w  = static_cast<Wt::WSlider*>(n.widget);
            const auto& o = sl->getOptions();
            const float span = o.getMax() - o.getMin();
            const int step = span > 0.f
                ? static_cast<int>(std::lround((sl->getValue() - o.getMin()) / span
                                               * sliderSteps(o)))
                : 0;
            if (w->value() != step) w->setValue(step);
            break;
        }
        case ElementType::Dropdown: {
            auto* dd = static_cast<Dropdown*>(n.element);
            auto* w  = static_cast<Wt::WComboBox*>(n.widget);
            if (w->currentIndex() != dd->getSelectedIndex())
                w->setCurrentIndex(dd->getSelectedIndex());
            break;
        }
        case ElementType::TextBox: {
            const std::string s = static_cast<Textbox*>(n.element)->getString();
            if (s != n.appliedText) {
                if (auto* e = dynamic_cast<Wt::WLineEdit*>(n.widget))
                    e->setText(Wt::WString::fromUTF8(s));
                else if (auto* a = dynamic_cast<Wt::WTextArea*>(n.widget))
                    a->setText(Wt::WString::fromUTF8(s));
                n.appliedText = s;
            }
            break;
        }
        default:
            break;
    }
}

void Translator::sync() {
    for (Node& n : m_nodes) apply(n);
}

void Translator::translate(Element* el, Wt::WContainerWidget* parent, Node node) {
    node.element = el;

    // Text is the only content-bearing widget whose content is also a child in
    // UILO's model, so everything else either has children or has a value.
    switch (el->getType()) {
        case ElementType::Text: {
            auto* w = parent->addWidget(std::make_unique<Wt::WText>());
            w->setInline(false);
            w->setTextFormat(Wt::TextFormat::Plain);
            node.widget = w;
            break;
        }

        case ElementType::Image: {
            applyImageAspect(static_cast<Image*>(el));

            const auto& o = static_cast<Image*>(el)->getOptions();
            if (!aspectLocked(o)) {
                node.widget = parent->addWidget(
                    std::make_unique<Wt::WImage>(Wt::WLink(o.getPath())));
                break;
            }

            // Aspect-locked: the element becomes a slot holding the picture.
            // Sizing the locked axis and leaving the other `auto` makes the
            // browser derive it from the image's intrinsic ratio -- the same
            // (size / aspect) Image::update() computes, but without the server
            // ever needing to know the file's dimensions. The box therefore
            // keeps a fixed shape as the layout around it moves, which is what
            // holds a clipEllipse mask steady.
            auto* slot = parent->addWidget(std::make_unique<Wt::WContainerWidget>());
            auto* img  = slot->addWidget(
                std::make_unique<Wt::WImage>(Wt::WLink(o.getPath())));

            // getLockAspectHeight() is tested first to match the if/else-if
            // order in Image::update() when both are set.
            std::string imgCss = o.getLockAspectHeight() ? "height:100%;width:auto;"
                                                         : "width:100%;height:auto;";
            imgCss += "flex:0 0 auto;";
            if (o.getClipEllipse()) imgCss += "border-radius:50%;";
            imgCss += flipCss(o);
            img->addStyleClass(classFor(imgCss, {}));

            node.widget = slot;
            break;
        }

        case ElementType::Icon: {
            // Feather-style icons are monochrome SVG authored with
            // stroke/fill="currentColor", so tinting is just a CSS `color` on
            // the inline markup -- the same palette role the desktop backend
            // rasterizes with. Emit the SVG inline (UnsafeXHTML, so Wt does not
            // strip it) and let the element's own box (sized from the Modifier)
            // drive the size; the <svg>'s viewBox/preserveAspectRatio keeps the
            // square art centered when the box is not square.
            auto* ic = static_cast<Icon*>(el);
            const auto& o = ic->getOptions();

            auto* w = parent->addWidget(std::make_unique<Wt::WText>());
            w->setInline(false);
            w->setTextFormat(Wt::TextFormat::UnsafeXHTML);
            w->setText(Wt::WString::fromUTF8(std::string(ic->getMarkup())));

            std::string css = "overflow:hidden;";
            if (!o.getPreserveOriginalColors())
                css += "color:" + rgba(el->resolveColor(o.getColorRole(), o.getColor())) + ";";
            if (o.getOpacity() < 1.f)
                css += "opacity:" + num(o.getOpacity()) + ";";

            std::string svgRule = "width:100%;height:100%;display:block;";
            std::string tf;
            if (o.getFlipH()) tf += "scaleX(-1) ";
            if (o.getFlipV()) tf += "scaleY(-1) ";
            if (!tf.empty()) svgRule += "transform:" + tf + ";";

            w->addStyleClass(classFor(css, { {" svg", svgRule} }));
            node.widget = w;
            break;
        }

        case ElementType::Slider: {
            auto* sl = static_cast<Slider*>(el);
            const auto& o = sl->getOptions();

            auto* w = parent->addWidget(std::make_unique<Wt::WSlider>());
            w->setNativeControl(true);
            w->setOrientation(o.getOrientation() == SliderOrientation::Vertical
                              ? Wt::Orientation::Vertical : Wt::Orientation::Horizontal);
            w->setRange(0, sliderSteps(o));

            // Double-click snaps back to the default, as Slider::checkLeftClick
            // does -- and, like it, only when one was actually set. A reset is
            // a discrete event, so this can go through the server rather than
            // needing the local handling a drag does. The browser's own
            // double-click threshold stands in for UILO's fixed 350 ms.
            w->doubleClicked().connect([this, sl] {
                const SliderOptions& opts = sl->getOptions();
                if (!opts.hasDefault()) return;
                sl->setValue(opts.getDefaultValue());
                sync();
            });

            if (!m_sliderJs) {
                m_app.doJavaScript(kSliderJs);
                m_sliderJs = true;
            }
            m_app.doJavaScript("uiloSlider('" + w->id() + "');");

            w->valueChanged().connect([this, sl, w](int step) {
                const auto& opts = sl->getOptions();
                // setValue() re-applies SliderOptions::step and the range clamp,
                // so the value a handler sees is the same one it would see
                // natively -- and it fires onValueChanged for us.
                sl->setValue(opts.getMin() + static_cast<float>(step) / sliderSteps(opts)
                                             * (opts.getMax() - opts.getMin()));
                // Hand the fill back to the class rule now that the server has
                // the value; both land in this same response, so they never
                // disagree on screen.
                m_app.doJavaScript("uiloSliderRelease('" + w->id() + "');");
                sync();
            });
            node.widget = w;
            break;
        }

        case ElementType::Dropdown: {
            auto* dd = static_cast<Dropdown*>(el);
            auto* w  = parent->addWidget(std::make_unique<Wt::WComboBox>());
            for (const std::string& item : dd->getItems())
                w->addItem(Wt::WString::fromUTF8(item));
            w->activated().connect([this, dd](int index) {
                dd->setSelectedIndex(index);
                sync();
            });
            node.widget = w;
            break;
        }

        case ElementType::TextBox: {
            auto* tb = static_cast<Textbox*>(el);
            const auto& o = tb->getOptions();

            Wt::WFormWidget* form = nullptr;
            if (o.getMultiline()) {
                auto* a = parent->addWidget(std::make_unique<Wt::WTextArea>());
                // A textarea's intrinsic height is `rows` (Wt defaults to 5),
                // which would be the floor the auto-grow measurement can see.
                // One row lets scrollHeight report the real content height; the
                // box's actual height comes from the layout, not from this.
                a->setRows(1);
                // No soft wrap also means the value keeps the user's own line
                // breaks only -- the browser must not insert any of its own.
                if (!o.getWrap()) a->setAttributeValue("wrap", "off");
                if (o.getMaxLength() > 0)
                    a->setAttributeValue("maxlength", std::to_string(o.getMaxLength()));
                a->changed().connect([this, tb, a] {
                    tb->setString(a->text().toUTF8());
                    if (const auto& cb = tb->getOptions().getOnStringChanged())
                        cb(a->text().toUTF8());
                    sync();
                });
                form = a;
            } else {
                auto* e = parent->addWidget(std::make_unique<Wt::WLineEdit>());
                if (o.getMaxLength() > 0)    e->setMaxLength(o.getMaxLength());
                if (o.getPasswordMode())     e->setEchoMode(Wt::EchoMode::Password);
                e->changed().connect([this, tb, e] {
                    tb->setString(e->text().toUTF8());
                    if (const auto& cb = tb->getOptions().getOnStringChanged())
                        cb(e->text().toUTF8());
                    sync();
                });
                // Only a single-line box treats Enter as a submit; multiline
                // inserts a newline and reports it through onStringChanged.
                e->enterPressed().connect([this, tb, e] {
                    tb->setString(e->text().toUTF8());
                    if (const auto& cb = tb->getOptions().getOnEnterPressed())
                        cb(e->text().toUTF8());
                    sync();
                });
                form = e;
            }
            if (!o.getPlaceholder().empty())
                form->setPlaceholderText(Wt::WString::fromUTF8(o.getPlaceholder()));
            node.widget = form;

            // Textbox::update() gates auto-height on multiline AND wrap, so a
            // no-wrap multiline box keeps the height it was declared with.
            if (o.getMultiline() && o.getWrap()) {
                if (!m_autoGrowJs) {
                    m_app.doJavaScript(kAutoGrowJs);
                    m_autoGrowJs = true;
                }
                m_app.doJavaScript(
                    "uiloAutoGrow('" + form->id() + "',{maxLines:"
                    + std::to_string(o.getMaxResizeLines())
                    + ",lineHeight:" + num(textboxLineHeight(o)) + "});");
            }
            break;
        }

        case ElementType::Knob: {
            auto* w = parent->addWidget(std::make_unique<KnobWidget>());
            node.widget = w;
            buildKnob(static_cast<Knob*>(el), w);
            break;
        }

        case ElementType::Resizer: {
            auto* wrap = parent->addWidget(std::make_unique<Wt::WContainerWidget>());
            wrap->addStyleClass("uilo-resizer");
            node.widget = wrap;
            buildResizer(static_cast<Resizer*>(el), wrap, node.axis);
            break;
        }

        default:
            // Containers, Button, Spacer, and the widgets with no web
            // equivalent yet (Knob, Waveform) are all plain boxes.
            node.widget = parent->addWidget(std::make_unique<Wt::WContainerWidget>());
            break;
    }

    node.widget->addStyleClass("uilo-el");
    if (!el->getName().empty()) node.widget->setObjectName(el->getName());

    // Every element can carry click and hover handlers, whatever its type.
    Modifier& mod = el->getModifier();
    if (auto* interactive = dynamic_cast<Wt::WInteractWidget*>(node.widget)) {
        if (mod.getOnLeftClick()) {
            if (el->getType() == ElementType::Button) {
                node.widget->setAttributeValue("role", "button");
                node.widget->setAttributeValue("tabindex", "0");
            }
            // Fetched at click time, not now: handlers can be swapped out at
            // runtime through getModifier().
            interactive->clicked().connect([this, el] {
                if (const auto& cb = el->getModifier().getOnLeftClick()) cb(el);
                sync();
            });
        }
        if (mod.getOnHoverEnter()) {
            interactive->mouseWentOver().connect([this, el] {
                if (const auto& cb = el->getModifier().getOnHoverEnter()) cb(el);
                sync();
            });
        }
        if (mod.getOnHoverExit()) {
            interactive->mouseWentOut().connect([this, el] {
                if (const auto& cb = el->getModifier().getOnHoverExit()) cb(el);
                sync();
            });
        }
    }

    m_nodes.push_back(node);
    apply(m_nodes.back());

    if (auto* container = dynamic_cast<Container*>(el)) {
        translateChildren(container, static_cast<Wt::WContainerWidget*>(node.widget),
                          axisOf(el), isScrollable(el), node.heightExpr);
    }
}

void Translator::translateChildren(Container* container, Wt::WContainerWidget* parent,
                                   Axis axis, bool scrolls,
                                   const std::string& parentHeight) {
    // UILO lays a container out in three groups -- start, centre, end -- and
    // draws them in that order regardless of child order, so the widgets are
    // emitted grouped the same way.
    std::vector<Element*> groups[3];
    for (Element* child : container->getChildren())
        groups[bucketOf(child->getModifier().getAlign(), axis)].push_back(child);

    // Auto margins absorb whatever space the sized children leave, which is
    // how the centre group gets centred and the end group reaches the far
    // edge. When percent-sized children are present they consume that space
    // first and the groups pack together -- matching UILO, where the same
    // thing happens for the same reason.
    const char* startMargin = axis == Axis::Row ? "margin-left:auto;" : "margin-top:auto;";
    const char* endMargin   = axis == Axis::Row ? "margin-right:auto;" : "margin-bottom:auto;";

    for (int g = 0; g < 3; ++g) {
        for (size_t i = 0; i < groups[g].size(); ++i) {
            Element* child = groups[g][i];

            Node node;
            node.axis          = axis;
            node.parentScrolls = scrolls;
            node.heightExpr    = ownHeightExpr(child, axis, scrolls, parentHeight);

            if (i == 0 && g > 0) node.autoMargin = startMargin;
            // A centre group with nothing after it needs a second auto margin,
            // or it would simply be pushed to the end.
            if (g == 1 && i + 1 == groups[1].size() && groups[2].empty())
                node.autoMargin += endMargin;

            translate(child, parent, node);
        }
    }
}

void Translator::build(Page& page, Wt::WContainerWidget* into) {
    Container* root = page.getRoot();
    if (!root) return;

    Node node;
    node.isRoot = true;
    node.axis   = axisOf(root);
    // The root fills the window, so its children's percentage heights are
    // ultimately a fraction of the viewport.
    node.heightExpr = "100vh";

    translate(root, into, node);
}

} // namespace uilo::wt::detail

#endif // UILO_WT
