#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "../Element.hpp"

namespace uilo {

/*
    ImageOptions:
    - Desc: Everything an Image draws: the file it comes from, an optional tint,
            whether either axis is driven by the source aspect ratio, and the
            per-draw transforms -- horizontal and vertical flips, and an elliptical
            clip for a round avatar. The tint only applies with recolor on, and
            comes as a literal plus a role, where the role wins when it resolves
            against the active Palette.
    - Locking an aspect makes one axis follow the other rather than the slot the
      parent gave it, so only one of the two lock flags is meaningful at a time.
*/
class ImageOptions {
public:
    ImageOptions() = default;

    ImageOptions& setPath(const std::string& path)    { m_path = path; return *this; }
    ImageOptions& setColor(const Color& c)            { m_color = c; return *this; }
    ImageOptions& setColorRole(const std::string& r)  { m_colorRole = r; return *this; }
    ImageOptions& setLockAspectWidth(bool v)          { m_lockAspectWidth = v; return *this; }
    ImageOptions& setLockAspectHeight(bool v)         { m_lockAspectHeight = v; return *this; }
    ImageOptions& setRecolor(bool v)                  { m_recolor = v; return *this; }
    ImageOptions& setClipEllipse(bool v)              { m_clipEllipse = v; return *this; }
    ImageOptions& setFlipH(bool v)                    { m_flipH = v; return *this; }
    ImageOptions& setFlipV(bool v)                    { m_flipV = v; return *this; }

    const std::string& getPath()              const { return m_path; }
    Color              getColor()             const { return m_color; }
    const std::string& getColorRole()         const { return m_colorRole; }
    bool               getLockAspectWidth()   const { return m_lockAspectWidth; }
    bool               getLockAspectHeight()  const { return m_lockAspectHeight; }
    bool               getRecolor()           const { return m_recolor; }
    bool               getClipEllipse()       const { return m_clipEllipse; }
    bool               getFlipH()             const { return m_flipH; }
    bool               getFlipV()             const { return m_flipV; }

private:
    std::string m_path;
    Color       m_color = Color::White;
    std::string m_colorRole;

    bool m_lockAspectWidth  = false;
    bool m_lockAspectHeight = false;
    bool m_recolor          = false;
    bool m_clipEllipse      = false;
    bool m_flipH            = false;
    bool m_flipV            = false;
};


/*
    Image:
    - Desc:     A textured quad loaded from a file. The texture is loaded on the
                first
            update, once the element is bound to a UILO and so has a renderer to
            load it through, and is cached by path -- so several Images naming the
            same file share one texture. Either axis can be driven by the source
            aspect ratio instead of by the slot the parent gave it.
    - getPixel and setPixel work in source-texture texels: (0,0) is the image's
      top-left, x below getTextureWidth() and y below getTextureHeight(), and
      neither on-screen size nor flips nor recolor affects them. Reads out of
      bounds, or before the element is attached to a UILO, return transparent;
      writes there are ignored.
    - The first pixel access decodes the file into a CPU-side buffer. The first
      *write* then detaches this Image onto a private texture, so the other
      Images sharing that file are unaffected, and bgfx's immutable cached
      texture is left alone. Writes are batched and uploaded once per frame at
      render time.
*/
class Image : public Element {
public:
    explicit Image(
        Modifier modifier,
        ImageOptions options = {},
        const std::string& name = ""
    );

    ~Image() override;

    void update(Rectf& parentBounds, float dt) override;
    void render() override;

    const ImageOptions& getOptions() const { return m_options; }
    ImageOptions&       getOptions()       { return m_options; }
    void                setOptions(const ImageOptions& opts) { m_options = opts; rebuildTexture(); }

    bool isLoaded() const;

    Color getPixel(const uint32_t x, const uint32_t y) const;
    void  setPixel(const uint32_t x, const uint32_t y, const Color& color);

    uint32_t getTextureWidth()  const { return m_textureWidth; }
    uint32_t getTextureHeight() const { return m_textureHeight; }

private:
    void rebuildTexture();
    void init();
    bool ensurePixels() const;
    void syncPixels();
    void releaseOwnedTexture();

    ImageOptions m_options;
    uint16_t     m_textureHandle = 0xFFFFu;
    uint32_t     m_textureWidth  = 0;
    uint32_t     m_textureHeight = 0;
    Color        m_lastRecolor   = Color::White;
    bool         m_loaded        = false;

    // CPU-side RGBA8 copy of the texture for get/setPixel. Mutable because the
    // decode is lazy and getPixel is logically const.
    mutable std::vector<uint8_t> m_pixels;
    mutable uint32_t             m_pixelsWidth  = 0;
    mutable uint32_t             m_pixelsHeight = 0;

    bool m_pixelsDirty = false;   /* CPU buffer has writes not yet uploaded */
    bool m_ownsTexture = false;   /* m_textureHandle is private, not cached */
};

}
