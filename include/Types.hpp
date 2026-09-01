#pragma once
#include <glm/glm.hpp>
#include <cstdint>
#include <string>

namespace tempest {

constexpr float kTileSize = 16.0f;
constexpr float kFixedDt = 1.0f / 60.0f;
constexpr int kMaxBatchVerts = 8192 * 6;

struct Rect {
    float x = 0, y = 0, w = 0, h = 0;
    float left() const { return x; }
    float right() const { return x + w; }
    float top() const { return y; }
    float bottom() const { return y + h; }
    glm::vec2 center() const { return {x + w * 0.5f, y + h * 0.5f}; }
    bool overlaps(const Rect& o) const {
        return right() > o.left() && left() < o.right() &&
               bottom() > o.top() && top() < o.bottom();
    }
};

inline Rect aabb(const glm::vec2& pos, const glm::vec2& size) {
    return Rect{pos.x, pos.y, size.x, size.y};
}

enum class Tile : std::uint8_t {
    Empty = 0,
    Cliff,
    Dirt,
    Stone,
    Brick,
    Wood,
    Spike,
    Platform,
    Door,
    DoorOpen,
    Bush,
    Flower,
    Crate,
    Cobble,
    Water,
    Banner,
};

enum class EntityKind { Player, Coin, Enemy, Npc, Fx };

} // namespace tempest
