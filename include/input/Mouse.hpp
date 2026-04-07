#pragma once

#include "../math/Vec2.hpp"
#include <any>
#include <optional>
#include <string>

namespace uilo {

class Mouse {
public:
    static Mouse& get() {
        static Mouse instance;
        return instance;
    }

    // --- Positions ---
    Vec2f windowPosition() const { return m_windowPos; }
    Vec2f monitorPosition() const { return m_monitorPos; }

    void setWindowPosition(Vec2f pos) { m_windowPos = pos; }
    void setMonitorPosition(Vec2f pos) { m_monitorPos = pos; }

    // --- Bucket (drag payload) ---
    // Place anything in the bucket, with an optional tag to identify it.
    template <typename T>
    void fill(T&& value, const std::string& tag = "") {
        m_bucket = std::forward<T>(value);
        m_tag = tag;
    }

    // Check if the bucket has something in it.
    bool hasCargo() const { return m_bucket.has_value(); }

    // Check if the bucket has something with a specific tag.
    bool hasCargo(const std::string& tag) const { return m_bucket.has_value() && m_tag == tag; }

    // Get the tag of whatever is in the bucket.
    const std::string& tag() const { return m_tag; }

    // Retrieve the contents. Returns nullptr if empty or wrong type.
    template <typename T>
    T* peek() {
        return std::any_cast<T>(&m_bucket);
    }

    // Retrieve and empty the bucket. Returns std::nullopt if empty or wrong type.
    template <typename T>
    std::optional<T> take() {
        auto* val = std::any_cast<T>(&m_bucket);
        if (!val) return std::nullopt;
        std::optional<T> result = std::move(*val);
        empty();
        return result;
    }

    // Empty the bucket.
    void empty() {
        m_bucket.reset();
        m_tag.clear();
    }

private:
    Mouse() = default;
    Mouse(const Mouse&) = delete;
    Mouse& operator=(const Mouse&) = delete;

    Vec2f m_windowPos{0.f, 0.f};
    Vec2f m_monitorPos{0.f, 0.f};

    std::any m_bucket;
    std::string m_tag;
};

} // namespace uilo