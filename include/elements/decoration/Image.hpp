#pragma once

#include "../Element.hpp"

namespace uilo {

enum class ImageOptions : uint8_t {
    NONE             = 0,
    LockAspectWidth  = 1 << 0,
    LockAspectHeight = 1 << 1,
    Recolor          = 1 << 2,
    ClipEllipse      = 1 << 3,
    FlipH            = 1 << 4,
    FlipV            = 1 << 5,
};

inline ImageOptions operator|(ImageOptions a, ImageOptions b) {
    return static_cast<ImageOptions>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline bool hasOption(ImageOptions set, ImageOptions flag) {
    return (static_cast<uint8_t>(set) & static_cast<uint8_t>(flag)) != 0;
}

class Image : public Element {
public:
    Image(Modifier modifier,
          const std::string& path,
          ImageOptions options = ImageOptions::NONE,
          const std::string& name = "");

    Image(Modifier modifier,
          sf::Image sourceImg,
          ImageOptions options = ImageOptions::NONE,
          const std::string& name = "");

    void update(sf::FloatRect& parentBounds, float dt) override;
    void render(sf::RenderTarget& target) override;

    void setOptions(ImageOptions options) { m_options = options; rebuildTexture(); } 

    bool isLoaded() const;

private:
    void rebuildTexture();
    void init();

    sf::Image                 m_originalImage;
    sf::Texture               m_texture;
    std::optional<sf::Sprite> m_sprite;

    ImageOptions              m_options     = ImageOptions::NONE;
    sf::Color                 m_lastRecolor = sf::Color::White;
    bool                      m_loaded      = false;
};

}