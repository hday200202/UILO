#pragma once

#include <vector>
#include <cstdint>
#include "../Element.hpp"

namespace uilo {

class Image : public Element {
public:
    Image(
        Modifier modifier,
        const uint8_t* pixels,
        uint32_t width,
        uint32_t height,
        bool keepAspectRatio = true,
        const std::string& name = ""
    );

    const std::vector<uint8_t>& getPixels() const;
    uint32_t getNativeWidth() const;
    uint32_t getNativeHeight() const;

    void setKeepAspectRatio(bool keep);
    bool getKeepAspectRatio() const;

    void recolor(const Color& keyColor, const Color& newColor);

    void update(Bounds& parentBounds, float dt) override;
    void render(Renderer& renderer) override;

private:
    std::vector<uint8_t> m_pixels;
    uint32_t m_nativeWidth = 0;
    uint32_t m_nativeHeight = 0;
    bool m_keepAspectRatio = true;
};

} // namespace uilo