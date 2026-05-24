#pragma once

#include <optional>

#include <SFML/Graphics.hpp>

#include "../Element.hpp"

namespace uilo {

class ImageOptions {
public:
    ImageOptions() = default;

    ImageOptions& setPath(const std::string& path) { m_path = path;            return *this; }
    ImageOptions& setImage(sf::Image img)           { m_image = std::move(img); return *this; }
    ImageOptions& setLockAspectWidth(bool v)        { m_lockAspectWidth  = v;   return *this; }
    ImageOptions& setLockAspectHeight(bool v)       { m_lockAspectHeight = v;   return *this; }
    ImageOptions& setRecolor(bool v)                { m_recolor          = v;   return *this; }
    ImageOptions& setClipEllipse(bool v)            { m_clipEllipse      = v;   return *this; }
    ImageOptions& setFlipH(bool v)                  { m_flipH            = v;   return *this; }
    ImageOptions& setFlipV(bool v)                  { m_flipV            = v;   return *this; }

    const std::string&              getPath()             const { return m_path; }
    const std::optional<sf::Image>& getImage()            const { return m_image; }
    bool                            getLockAspectWidth()  const { return m_lockAspectWidth; }
    bool                            getLockAspectHeight() const { return m_lockAspectHeight; }
    bool                            getRecolor()          const { return m_recolor; }
    bool                            getClipEllipse()      const { return m_clipEllipse; }
    bool                            getFlipH()            const { return m_flipH; }
    bool                            getFlipV()            const { return m_flipV; }

private:
    std::string              m_path;
    std::optional<sf::Image> m_image;
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

    void update(sf::FloatRect& parentBounds, float dt) override;
    void render(sf::RenderTarget& target) override;

    bool isLoaded() const;

private:
    void rebuildTexture();
    void init();

    ImageOptions              m_options;
    sf::Image                 m_originalImage;
    sf::Texture               m_texture;
    std::optional<sf::Sprite> m_sprite;
    sf::Color                 m_lastRecolor = sf::Color::White;
    bool                      m_loaded      = false;
};

}