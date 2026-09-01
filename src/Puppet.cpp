#include "Puppet.hpp"
#include "Atlas.hpp"
#include <algorithm>
#include <cmath>
#include <vector>

namespace tempest {
namespace {
BodyPart& part(Puppet& p, PartSlot s) { return p.parts[static_cast<int>(s)]; }

void base(BodyPart& b, const char* spr, float lx, float ly, float z) {
    b.sprite = spr;
    b.local = {lx, ly};
    b.extra = {0, 0};
    b.z = z;
    b.scaleX = b.scaleY = 1.f;
    b.w = b.h = 16.f;
}
}

void Puppet::setupGale() {
    // Origin is feet. local.y is offset of sprite TOP from feet (negative = up).
    base(part(*this, PartSlot::Cape), "cape0", -2.f, -30.f, 0.f);
    base(part(*this, PartSlot::BackArm), "arm_b0", -6.f, -26.f, 1.f);
    base(part(*this, PartSlot::FarLeg), "leg_far0", -3.f, -16.f, 2.f);
    base(part(*this, PartSlot::Torso), "torso0", 0.f, -28.f, 3.f);
    base(part(*this, PartSlot::Head), "head0", 0.f, -40.f, 4.f);
    base(part(*this, PartSlot::NearLeg), "leg0", 3.f, -16.f, 5.f);
    base(part(*this, PartSlot::FrontArm), "arm_f0", 6.f, -26.f, 6.f);
    base(part(*this, PartSlot::Weapon), "spear0", 10.f, -24.f, 7.f);
    pose = PuppetPose::Idle;
}

void Puppet::setupWisp() {
    for (int i = 0; i < static_cast<int>(PartSlot::Count); ++i)
        parts[i].sprite.clear();
    base(part(*this, PartSlot::Torso), "wisp_glow", 0.f, -14.f, 0.f);
    base(part(*this, PartSlot::Head), "wisp0", 0.f, -16.f, 1.f);
}

void Puppet::setupGolem() {
    for (int i = 0; i < static_cast<int>(PartSlot::Count); ++i)
        parts[i].sprite.clear();
    base(part(*this, PartSlot::FarLeg), "golem_legs0", 0.f, -16.f, 0.f);
    base(part(*this, PartSlot::Torso), "golem_torso", 0.f, -26.f, 1.f);
    base(part(*this, PartSlot::Head), "golem_head", 0.f, -38.f, 2.f);
}

void Puppet::setupTrainer() {
    for (int i = 0; i < static_cast<int>(PartSlot::Count); ++i)
        parts[i].sprite.clear();
    base(part(*this, PartSlot::Torso), "npc_trainer", 0.f, -16.f, 0.f);
}

void Puppet::tick(float dt, bool moving, bool onGround, float faceDir) {
    anim += dt;
    flipX = faceDir < 0.f;
    capeFollowX += ((flipX ? -1.f : 1.f) - capeFollowX) * std::min(1.f, dt * 6.f);

    auto& cape = part(*this, PartSlot::Cape);
    auto& barm = part(*this, PartSlot::BackArm);
    auto& farm = part(*this, PartSlot::FrontArm);
    auto& torso = part(*this, PartSlot::Torso);
    auto& head = part(*this, PartSlot::Head);
    auto& nleg = part(*this, PartSlot::NearLeg);
    auto& fleg = part(*this, PartSlot::FarLeg);
    auto& weap = part(*this, PartSlot::Weapon);

    for (int i = 0; i < static_cast<int>(PartSlot::Count); ++i) {
        parts[i].extra = {0, 0};
        parts[i].scaleX = parts[i].scaleY = 1.f;
    }

    if (pose == PuppetPose::Attack) {
        float t = std::fmod(anim, 0.22f);
        farm.sprite = "arm_f2";
        weap.sprite = "spear1";
        farm.extra = {6.f, 2.f};
        weap.extra = {8.f, 1.f};
        torso.extra = {4.f, 0.f};
        head.extra = {3.f, 1.f};
        cape.extra = {-2.f, 1.f};
        (void)t;
    } else if (pose == PuppetPose::Dash) {
        farm.sprite = "arm_f1";
        weap.sprite = "spear1";
        torso.scaleX = 1.15f;
        torso.scaleY = 0.85f;
        head.extra = {4.f, 2.f};
        cape.extra = {-6.f, 0.f};
        nleg.sprite = "leg1";
        fleg.sprite = "leg_far1";
    } else if (pose == PuppetPose::Guard) {
        farm.sprite = "arm_f1";
        weap.sprite = "spear0";
        torso.extra = {-3.f, 0.f};
        cape.extra = {2.f, 0.f};
    } else if (pose == PuppetPose::Hurt) {
        head.extra = {-2.f, 2.f};
        cape.extra = {3.f, 0.f};
    } else if (!onGround || pose == PuppetPose::Jump) {
        pose = PuppetPose::Jump;
        float stretch = (faceDir == 0.f) ? 1.f : 1.f;
        (void)stretch;
        nleg.sprite = "leg_tuck";
        fleg.sprite = "leg_tuck";
        nleg.extra = {2.f, 4.f};
        fleg.extra = {-2.f, 4.f};
        torso.scaleY = 1.12f;
        torso.scaleX = 0.92f;
        head.extra = {0.f, -2.f};
        farm.sprite = "arm_f1";
        barm.sprite = "arm_b1";
        weap.sprite = "spear0";
        cape.sprite = "cape2";
        cape.extra = {-capeFollowX * 3.f, 2.f};
    } else if (moving) {
        pose = PuppetPose::Walk;
        float ph = anim * 10.f;
        int step = (static_cast<int>(std::floor(ph)) % 2);
        nleg.sprite = step ? "leg1" : "leg0";
        fleg.sprite = step ? "leg_far0" : "leg_far1";
        float swing = std::sin(ph) * 3.f;
        nleg.extra = {swing, std::abs(std::sin(ph)) * -1.5f};
        fleg.extra = {-swing, std::abs(std::sin(ph + 3.14f)) * -1.5f};
        farm.sprite = step ? "arm_f1" : "arm_f0";
        barm.sprite = step ? "arm_b0" : "arm_b1";
        farm.extra = {-swing, 0.f};
        barm.extra = {swing, 0.f};
        weap.extra = {-swing * 0.6f, 0.f};
        weap.sprite = "spear0";
        head.sprite = (std::sin(ph) > 0) ? "head1" : "head0";
        head.extra = {0.f, std::sin(ph * 2.f) * 1.2f};
        int cf = static_cast<int>(std::floor(anim * 6.f + 0.8f)) % 3;
        cape.sprite = cf == 0 ? "cape0" : (cf == 1 ? "cape1" : "cape2");
        cape.extra = {-capeFollowX * 2.f - swing * 0.4f, 0.f};
        torso.sprite = "torso0";
    } else {
        pose = PuppetPose::Idle;
        float breath = std::sin(anim * 2.4f);
        torso.scaleY = 1.f + breath * 0.04f;
        torso.extra = {0.f, breath * -0.8f};
        torso.sprite = (breath > 0.3f) ? "torso1" : "torso0";
        head.extra = {0.f, breath * -0.5f};
        head.sprite = "head0";
        nleg.sprite = "leg0";
        fleg.sprite = "leg_far0";
        farm.sprite = "arm_f0";
        barm.sprite = "arm_b0";
        weap.sprite = "spear0";
        int cf = static_cast<int>(std::floor(anim * 2.f)) % 3;
        cape.sprite = cf == 0 ? "cape0" : (cf == 1 ? "cape1" : "cape2");
        cape.extra = {std::sin(anim * 1.7f) * 1.5f, 0.f};
    }
}

void Puppet::draw(Renderer& r, const Texture& atlas, glm::vec2 feet, const glm::vec4& tint) const {
    struct Item { float z; int i; };
    std::vector<Item> order;
    for (int i = 0; i < static_cast<int>(PartSlot::Count); ++i) {
        if (parts[i].sprite.empty()) continue;
        order.push_back({parts[i].z, i});
    }
    std::sort(order.begin(), order.end(), [](const Item& a, const Item& b) { return a.z < b.z; });
    for (auto& it : order) {
        const BodyPart& p = parts[it.i];
        float ox = p.local.x + p.extra.x;
        if (flipX) ox = -ox;
        float oy = p.local.y + p.extra.y;
        float w = p.w * p.scaleX;
        float h = p.h * p.scaleY;
        Rect dest{feet.x + ox - w * 0.5f, feet.y + oy, w, h};
        r.drawSprite(atlas, dest, spriteUV(p.sprite), tint, flipX);
    }
}

} // namespace tempest
