#pragma once
#include "Renderer.hpp"
#include "Texture.hpp"
#include "Types.hpp"
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace tempest {

enum class PartSlot {
    Cape = 0,
    BackArm,
    FarLeg,
    Torso,
    Head,
    NearLeg,
    FrontArm,
    Weapon,
    Count
};

// Kinematic chain: rest is local offset from parent joint (or hips/feet if parent < 0).
// extra is pose motion, clamped so |extra| never exceeds maxExtra.
struct BodyPart {
    std::string sprite;
    glm::vec2 local{0, 0};   // rest offset from parent (y down-positive? y-down: negative is up)
    glm::vec2 extra{0, 0};
    float z = 0.f;
    float scaleX = 1.f;
    float scaleY = 1.f;
    float w = 32.f;
    float h = 32.f;
    int parent = -1;         // PartSlot index, or -1 = hips/feet origin
    float maxExtra = 8.f;
};

enum class PuppetPose { Idle, Walk, Jump, Attack, Dash, Guard, Hurt };

struct Puppet {
    BodyPart parts[static_cast<int>(PartSlot::Count)];
    float anim = 0.f;
    float capeSway = 0.f;
    float capeFollowX = 0.f;
    PuppetPose pose = PuppetPose::Idle;
    bool flipX = false;

    void setupGale();
    void setupWisp();
    void setupGolem();
    void setupTrainer();
    void tick(float dt, bool moving, bool onGround, float faceDir);
    void draw(Renderer& r, const Texture& atlas, glm::vec2 feet, const glm::vec4& tint) const;
};

} // namespace tempest
