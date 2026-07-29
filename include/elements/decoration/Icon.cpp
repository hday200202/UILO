#include "Icon.hpp"

#include "../../UILO.hpp"
#include "../../renderer/Renderer.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <sstream>

// NanoSVG is this translation unit's job to instantiate. Its warnings are not
// ours to fix, so they are silenced rather than polluting a -Wall -Wextra build.
#if defined(__clang__) || defined(__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wunused-parameter"
#  pragma GCC diagnostic ignored "-Wunused-function"
#  pragma GCC diagnostic ignored "-Wsign-compare"
#  pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#elif defined(_MSC_VER)
#  pragma warning(push)
#  pragma warning(disable : 4244 4245 4996)
#endif

#define NANOSVG_IMPLEMENTATION
#define NANOSVG_ALL_COLOR_KEYWORDS
#include "nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvgrast.h"

#if defined(__clang__) || defined(__GNUC__)
#  pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#  pragma warning(pop)
#endif

namespace uilo {

namespace {

// One rasterizer for the process. Rasterizing is single-threaded here (it runs
// from render()), and the context holds reusable scratch buffers, so building a
// fresh one per icon would throw that away.
NSVGrasterizer* rasterizer() {
    struct Holder {
        NSVGrasterizer* r = nsvgCreateRasterizer();
        ~Holder() { if (r) nsvgDeleteRasterizer(r); }
    };
    static Holder holder;
    return holder.r;
}

// NanoSVG packs colours as 0xAABBGGRR.
uint32_t packColor(Color c) {
    return (static_cast<uint32_t>(c.a) << 24) |
           (static_cast<uint32_t>(c.b) << 16) |
           (static_cast<uint32_t>(c.g) << 8)  |
            static_cast<uint32_t>(c.r);
}

/*
    parseViewBox
    - Pulls the four viewBox numbers straight out of the markup. NanoSVG applies
      the viewBox transform during parsing and does not report it back, but the
      original units are needed for two things: the icon's intrinsic aspect
      ratio, and converting an authored stroke width into the parsed (scaled)
      units NSVGshape carries.
*/
bool parseViewBox(std::string_view markup, float& w, float& h) {
    const std::size_t at = markup.find("viewBox");
    if (at == std::string_view::npos) return false;

    const std::size_t quote = markup.find('"', at);
    if (quote == std::string_view::npos) return false;
    const std::size_t close = markup.find('"', quote + 1);
    if (close == std::string_view::npos) return false;

    const std::string values(markup.substr(quote + 1, close - quote - 1));
    std::istringstream ss(values);
    float minX = 0.f, minY = 0.f, vbW = 0.f, vbH = 0.f;
    if (!(ss >> minX >> minY >> vbW >> vbH)) return false;
    if (vbW <= 0.f || vbH <= 0.f) return false;

    w = vbW;
    h = vbH;
    return true;
}

} // namespace


Icon::Icon(Modifier modifier, IconOptions options, const std::string& name)
    : m_options(std::move(options))
{
    m_modifier = modifier;
    m_name     = name;
    m_type     = ElementType::Icon;
}

Icon::~Icon() {
    releaseTexture();
}

void Icon::setOptions(const IconOptions& opts) {
    m_options = opts;
    // Force a re-raster: the source, colour, or stroke may all have moved.
    m_rasterSource.clear();
    m_rasterW = 0;
    m_rasterH = 0;
    m_sourceAspectKnown = false;
    m_dirty = true;
}

void Icon::releaseTexture() {
    if (!m_textureValid) return;
    if (m_uiloRef) {
        Texture tex;
        tex.handle = m_textureHandle;
        m_uiloRef->getRenderer().destroyTexture(tex);
    }
    m_textureHandle = 0xFFFFu;
    m_textureValid  = false;
}

std::string_view Icon::getMarkup() const {
    if (!m_options.getMarkup().empty()) return m_options.getMarkup();

    if (!m_options.getFile().empty()) {
        // Read once and keep it; the returned view has to outlive this call.
        if (m_fileMarkupPath != m_options.getFile()) {
            std::ifstream in(m_options.getFile(), std::ios::binary);
            if (!in) {
                std::fprintf(stderr, "[UILO] Icon: could not open '%s'\n",
                             m_options.getFile().c_str());
                m_fileMarkup.clear();
            } else {
                std::ostringstream ss;
                ss << in.rdbuf();
                m_fileMarkup = ss.str();
            }
            m_fileMarkupPath = m_options.getFile();
        }
        return m_fileMarkup;
    }

    if (!m_options.getIcon().empty())
        return Resources::get().iconRegistry().find(m_options.getIcon());

    return {};
}

float Icon::getSourceAspect() const {
    const std::string_view markup = getMarkup();
    if (markup.empty()) return 1.f;

    if (!m_sourceAspectKnown) {
        float vbW = 0.f, vbH = 0.f;
        m_sourceAspect      = parseViewBox(markup, vbW, vbH) ? (vbW / vbH) : 1.f;
        m_sourceAspectKnown = true;
    }
    return m_sourceAspect;
}

bool Icon::ensureRaster(uint32_t pxW, uint32_t pxH, Color tint) {
    if (pxW == 0 || pxH == 0) return false;

    const std::string_view markup = getMarkup();
    if (markup.empty()) return false;

    const float stroke = m_options.hasStrokeWidth() ? m_options.getStrokeWidth() : -1.f;

    // Everything the pixels depend on. Anything else (flips, the destination
    // rect) is handled at draw time and must not force a re-raster.
    const bool sameKey = m_textureValid
        && m_rasterW == pxW
        && m_rasterH == pxH
        && m_rasterColor.r == tint.r && m_rasterColor.g == tint.g
        && m_rasterColor.b == tint.b && m_rasterColor.a == tint.a
        && m_rasterStroke == stroke
        && m_rasterPreserveColors == m_options.getPreserveOriginalColors()
        && m_rasterSource.size() == markup.size()
        && std::equal(m_rasterSource.begin(), m_rasterSource.end(), markup.begin());
    if (sameKey) return true;

    NSVGrasterizer* rast = rasterizer();
    if (!rast || !m_uiloRef) return false;

    // nsvgParse writes into its input, so it gets a private, NUL-terminated copy.
    std::vector<char> mutableMarkup(markup.begin(), markup.end());
    mutableMarkup.push_back('\0');

    NSVGimage* image = nsvgParse(mutableMarkup.data(), "px", 96.f);
    if (!image || image->width <= 0.f || image->height <= 0.f) {
        if (image) nsvgDelete(image);
        return false;
    }

    // Authored stroke width -> parsed units. NanoSVG has already folded the
    // viewBox scale into every shape, so the override has to be folded the same
    // way or a stroke set to 1.5 would come out ~4x too thin on a 24-unit grid
    // drawn at 100px.
    float docScale = 1.f;
    if (m_options.hasStrokeWidth()) {
        float vbW = 0.f, vbH = 0.f;
        if (parseViewBox(markup, vbW, vbH) && vbW > 0.f)
            docScale = image->width / vbW;
    }

    const uint32_t packed  = packColor(tint);
    const bool     recolor = !m_options.getPreserveOriginalColors();
    const float    opacity = std::clamp(m_options.getOpacity(), 0.f, 1.f);

    for (NSVGshape* shape = image->shapes; shape; shape = shape->next) {
        if (recolor) {
            // The built-ins declare stroke="currentColor", which no SVG parser
            // can resolve on its own -- NanoSVG leaves a grey behind. Retinting
            // whichever paints the shape actually uses is what makes colouring
            // an icon a one-liner without touching the markup.
            //
            // Only paints that are already active get retinted. A shape the
            // author left unpainted stays unpainted: NanoSVG applies SVG's
            // default black fill, so "no paint at all" means the art really was
            // meant to be invisible (a <line> with no stroke draws nothing in a
            // browser either), and forcing one on would fight the source.
            if (shape->stroke.type != NSVG_PAINT_NONE) {
                shape->stroke.type  = NSVG_PAINT_COLOR;
                shape->stroke.color = packed;
            }
            if (shape->fill.type != NSVG_PAINT_NONE) {
                shape->fill.type  = NSVG_PAINT_COLOR;
                shape->fill.color = packed;
            }
        }

        if (m_options.hasStrokeWidth() && shape->stroke.type != NSVG_PAINT_NONE)
            shape->strokeWidth = m_options.getStrokeWidth() * docScale;

        shape->opacity *= opacity;
    }

    const float scaleX = static_cast<float>(pxW) / image->width;
    const float scaleY = static_cast<float>(pxH) / image->height;
    // One uniform scale: nsvgRasterize has no non-uniform mode, and stretching
    // is the quad's job at draw time. Fitting uses the smaller factor;
    // stretching rasterizes at the larger one so the wider axis keeps its detail.
    const float scale = m_options.getPreserveAspect()
        ? std::min(scaleX, scaleY)
        : std::max(scaleX, scaleY);

    const float sample = std::max(1.f, m_options.getSupersample());
    const float finalScale = scale * sample;

    const uint32_t texW = std::max(1u, static_cast<uint32_t>(
        std::lround(image->width  * finalScale)));
    const uint32_t texH = std::max(1u, static_cast<uint32_t>(
        std::lround(image->height * finalScale)));

    // bgfx textures are uint16-addressed; a pathological size would wrap.
    if (texW > 4096u || texH > 4096u) {
        nsvgDelete(image);
        return false;
    }

    std::vector<uint8_t> pixels(static_cast<std::size_t>(texW) * texH * 4u, 0u);
    nsvgRasterize(rast, image, 0.f, 0.f, finalScale,
                  pixels.data(), static_cast<int>(texW), static_cast<int>(texH),
                  static_cast<int>(texW) * 4);
    nsvgDelete(image);

    Renderer& renderer = m_uiloRef->getRenderer();

    // Reuse the texture when only its contents changed (a palette switch, a new
    // stroke width); reallocate only when the dimensions move.
    if (m_textureValid && (m_rasterTexW != texW || m_rasterTexH != texH))
        releaseTexture();

    if (!m_textureValid) {
        Texture tex = renderer.createTexture(static_cast<uint16_t>(texW),
                                             static_cast<uint16_t>(texH));
        if (!tex.valid()) return false;
        m_textureHandle = tex.handle;
        m_textureValid  = true;
        m_rasterTexW    = texW;
        m_rasterTexH    = texH;
    }

    Texture tex;
    tex.handle = m_textureHandle;
    tex.width  = static_cast<uint16_t>(texW);
    tex.height = static_cast<uint16_t>(texH);
    renderer.updateTexture(tex, pixels.data());

    m_rasterW      = pxW;
    m_rasterH      = pxH;
    m_rasterColor  = tint;
    m_rasterStroke = stroke;
    m_rasterPreserveColors = m_options.getPreserveOriginalColors();
    m_rasterSource.assign(markup.begin(), markup.end());
    return true;
}

void Icon::update(Rectf& parentBounds, float dt) {
    (void)dt;
    resize(parentBounds);
}

void Icon::render() {
    m_dirty = false;
    if (!m_uiloRef) return;
    if (m_bounds.size.x <= 0.f || m_bounds.size.y <= 0.f) return;

    const Color tint = m_uiloRef->getPalette().resolve(
        m_options.getColorRole(), m_options.getColor());

    // Rasterize at the size actually covered on screen, so the icon stays sharp
    // through setScale() and DPI changes instead of being resampled.
    const uint32_t pxW = static_cast<uint32_t>(std::lround(m_bounds.size.x));
    const uint32_t pxH = static_cast<uint32_t>(std::lround(m_bounds.size.y));
    if (!ensureRaster(pxW, pxH, tint)) return;

    Texture tex;
    tex.handle = m_textureHandle;
    tex.width  = static_cast<uint16_t>(m_rasterTexW);
    tex.height = static_cast<uint16_t>(m_rasterTexH);

    Rectf dst = m_bounds;
    if (m_options.getPreserveAspect()) {
        // Letterbox: the raster is aspect-correct, so centre it rather than
        // stretching the quad.
        const float aspect = static_cast<float>(m_rasterTexW) /
                             static_cast<float>(m_rasterTexH);
        float w = m_bounds.size.x;
        float h = w / aspect;
        if (h > m_bounds.size.y) {
            h = m_bounds.size.y;
            w = h * aspect;
        }
        dst.position.x = m_bounds.position.x + (m_bounds.size.x - w) * 0.5f;
        dst.position.y = m_bounds.position.y + (m_bounds.size.y - h) * 0.5f;
        dst.size = { w, h };
    }

    // The tint is already baked into the pixels, so the draw passes white and
    // lets the texture's own colour through.
    m_uiloRef->getRenderer().drawImage(
        dst, tex, Color::White, {{0.f, 0.f}, {1.f, 1.f}},
        m_options.getFlipH(), m_options.getFlipV(), false);
}

} // namespace uilo
