#pragma once
#include "Puppet.hpp"
#include "Types.hpp"
#include <algorithm>
#include <glm/glm.hpp>
#include <string>

namespace tempest {

struct Entity {
    EntityKind kind = EntityKind::Player;
    int brain = 0; // 0 none, 1 wisp, 2 golem, 3 npc
    glm::vec2 pos{0, 0};
    glm::vec2 vel{0, 0};
    glm::vec2 size{10, 16};
    glm::vec2 spawn{0, 0};
    bool onGround = false;
    bool facingRight = true;
    bool alive = true;
    float animTime = 0.f;
    float hurtFlash = 0.f;
    float iTimer = 0.f;
    float stateTime = 0.f;
    int hp = 5;
    int maxHp = 5;
    int damage = 1;
    Puppet puppet;
    std::string name = "Gale";
    std::string talk;

    glm::vec2 feet() const { return {pos.x + size.x * 0.5f, pos.y + size.y}; }

    // Body AABB from drawn parts (weapon excluded). Inset 2px so outline fluff
    // does not block doorways. Coins keep their own size box.
    Rect bounds() const {
        if (kind == EntityKind::Coin || kind == EntityKind::Fx)
            return aabb(pos, size);
        bool any = false;
        for (int i = 0; i < static_cast<int>(PartSlot::Count); ++i) {
            if (!puppet.parts[i].sprite.empty()) { any = true; break; }
        }
        if (!any) return aabb(pos, size);
        Rect v = puppet.visualBounds(feet(), !facingRight, false);
        const float inset = 2.f;
        v.x += inset;
        v.y += inset;
        v.w = std::max(4.f, v.w - inset * 2.f);
        v.h = std::max(4.f, v.h - inset * 2.f);
        return v;
    }

    void applyPuppetSize() {
        glm::vec2 s = puppet.restBodySize();
        size.x = std::max(8.f, s.x);
        size.y = std::max(12.f, s.y);
    }

    void syncCollisionFromPuppet() {
        if (kind == EntityKind::Coin || kind == EntityKind::Fx) return;
        glm::vec2 f = feet();
        Rect b = bounds();
        if (b.w < 4.f || b.h < 4.f) return;
        pos.x = f.x - b.w * 0.5f;
        pos.y = f.y - b.h;
        size = {b.w, b.h};
    }
};

} // namespace tempest
