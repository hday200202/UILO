#pragma once

#include <optional>
#include <string>

#include "../Element.hpp"

namespace uilo {

class ImageOptions {
public:
    ImageOptions() = default;

    ImageOptions& setPath(const std::string& path) { m_path = path;  return *this; }
    ImageOptions& setColor(const Color& c)          { m_color = c;    return *this; }
    ImageOptions& setColorRole(const std::string& r) { m_colorRole = r; return *this; }
    ImageOptions& setLockAspectWidth(bool v)        { m_lockAspectWidth  = v;   return *this; }
    ImageOptions& setLockAspectHeight(bool v)       { m_lockAspectHeight = v;   return *this; }
    ImageOptions& setRecolor(bool v)                { m_recolor          = v;   return *this; }
    ImageOptions& setClipEllipse(bool v)            { m_clipEllipse      = v;   return *this; }
    ImageOptions& setFlipH(bool v)                  { m_flipH            = v;   return *this; }
    ImageOptions& setFlipV(bool v)                  { m_flipV            = v;   return *this; }

    const std::string& getPath()             const { return m_path; }
    Color              getColor()            const { return m_color; }
    const std::string& getColorRole()        const { return m_colorRole; }
    bool                            getLockAspectWidth()  const { return m_lockAspectWidth; }
    bool                            getLockAspectHeight() const { return m_lockAspectHeight; }
    bool                            getRecolor()          const { return m_recolor; }
    bool                            getClipEllipse()      const { return m_clipEllipse; }
    bool                            getFlipH()            const { return m_flipH; }
    bool                            getFlipV()            const { return m_flipV; }

private:
    std::string m_path;
    Color       m_color           = Color::White;
    std::string m_colorRole;
    bool m_lockAspectWidth  = false;
    bool m_lockAspectHeight = false;
    bool m_recolor          = false;
    bool m_clipEllipse      = false;
    bool m_flipH            = false;
    bool m_flipV            = false;
};

class Image : public Element {
public:
    explicit Image(Modifier modifier, ImageOptions options = {}, const std::string& name = "");

    void update(Rectf& parentBounds, float dt) override;
    void render() override;

    const ImageOptions& getOptions() const     { return m_options; }
    ImageOptions&       getOptions()           { return m_options; }
    void setOptions(const ImageOptions& opts)  { m_options = opts; rebuildTexture(); }

    bool isLoaded() const;

private:
    void rebuildTexture();
    void init();

    ImageOptions  m_options;
    uint16_t      m_textureHandle = 0xFFFFu;
    uint32_t      m_textureWidth  = 0;
    uint32_t      m_textureHeight = 0;
    Color         m_lastRecolor   = Color::White;
    bool          m_loaded        = false;
};

}