#pragma once
#include "Puppet.hpp"
#include "Types.hpp"
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

    Rect bounds() const { return aabb(pos, size); }
    glm::vec2 feet() const { return {pos.x + size.x * 0.5f, pos.y + size.y}; }
};

} // namespace tempest
