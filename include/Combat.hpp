#pragma once
#include "Audio.hpp"
#include "Entity.hpp"
#include "Input.hpp"
#include "Skills.hpp"
#include "Tilemap.hpp"
#include <vector>

namespace tempest {

struct CombatState {
    float attackT = 0.f;
    float dashT = 0.f;
    float guardT = 0.f;
    float cycloneT = 0.f;
    float lungeT = 0.f;
    bool usedUpdraft = false;
    float coyote = 0.f;
    float fxT = 0.f;
    glm::vec2 fxPos{0, 0};
    std::string fxSprite = "slash";
};

void tickCombat(CombatState& c, Entity& gale, SkillState& skills, const Input& in,
                const Tilemap& map, std::vector<Entity>& enemies, Audio& audio, float dt);
void hurt(Entity& e, int dmg, const glm::vec2& knock, Audio& audio);

} // namespace tempest
