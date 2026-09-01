#pragma once
#include <glm/glm.hpp>
#include <cstdint>
#include <string>

namespace tempest {

constexpr float kTileSize = 16.0f;
constexpr float kFixedDt = 1.0f / 60.0f;
constexpr int kMaxBatchVerts = 8192 * 6;

// Logical world camera. Integer-scaled into the window (letterboxed).
constexpr float kViewW = 640.f;
constexpr float kViewH = 360.f;

// Player motion (see tools/physics.txt). Designed so a run-jump clears ~4.8 tiles.
// hangTime = 2*|kJumpVel|/kGravity = 0.808 s
// jumpDist = kMaxSpeed * hangTime = 76.7 px = 4.80 tiles
// jumpHeight = kJumpVel^2 / (2*kGravity) = 42.4 px = 2.65 tiles
// Mandatory pits without Bolt Step: <= 3 tiles (floor(4.80)-1).
constexpr float kMaxSpeed = 95.f;
constexpr float kJumpVel = -210.f;
constexpr float kGravity = 520.f;
constexpr float kMaxFall = 280.f;

// Tempest Strike: separate reach box in front of the 10x16 body hurtbox.
constexpr float kStrikeReach = 32.f;
constexpr float kStrikeReachWide = 40.f;
constexpr float kAttackDuration = 0.44f;
constexpr float kAttackWindup = 0.10f;
constexpr float kAttackRecover = 0.08f; // active = windup .. duration-recover

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
