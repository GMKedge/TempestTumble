#include "Puppet.hpp"
#include "Atlas.hpp"
#include <algorithm>
#include <cmath>
#include <vector>

namespace tempest {
namespace {
BodyPart& part(Puppet& p, PartSlot s) { return p.parts[static_cast<int>(s)]; }

void base(BodyPart& b, const char* spr, float lx, float ly, float z, float w, float h,
          int parent, float maxExtra) {
    b.sprite = spr;
    b.local = {lx, ly};
    b.extra = {0, 0};
    b.z = z;
    b.scaleX = b.scaleY = 1.f;
    b.w = w;
    b.h = h;
    b.parent = parent;
    b.maxExtra = maxExtra;
}

glm::vec2 clampExtra(glm::vec2 extra, float maxLen) {
    float L = std::sqrt(extra.x * extra.x + extra.y * extra.y);
    if (L > maxLen && L > 1e-4f) extra *= (maxLen / L);
    return extra;
}

constexpr float kPi = 3.14159265f;
} // namespace

void Puppet::setupGale() {
    // Hips/feet origin. Torso parents head, arms, legs, cape. Weapon parents to front-arm wrist.
    const int T = static_cast<int>(PartSlot::Torso);
    const int A = static_cast<int>(PartSlot::FrontArm);
    base(part(*this, PartSlot::Torso), "torso0", 0.f, -36.f, 3.f, 32.f, 32.f, -1, 6.f);
    base(part(*this, PartSlot::Head), "head0", 0.f, -18.f, 4.f, 32.f, 32.f, T, 2.f);
    base(part(*this, PartSlot::Cape), "cape0", -6.f, -8.f, 0.f, 32.f, 32.f, T, 10.f);
    base(part(*this, PartSlot::BackArm), "arm_b0", -10.f, 4.f, 1.f, 32.f, 32.f, T, 6.f);
    base(part(*this, PartSlot::FrontArm), "arm_f0", 10.f, 4.f, 6.f, 32.f, 32.f, T, 6.f);
    base(part(*this, PartSlot::FarLeg), "leg_far0", -5.f, 14.f, 2.f, 32.f, 32.f, T, 8.f);
    base(part(*this, PartSlot::NearLeg), "leg0", 5.f, 14.f, 5.f, 32.f, 32.f, T, 8.f);
    base(part(*this, PartSlot::Weapon), "spear0", 18.f, 6.f, 7.f, 48.f, 32.f, A, 10.f);
    pose = PuppetPose::Idle;
}

void Puppet::setupWisp() {
    for (int i = 0; i < static_cast<int>(PartSlot::Count); ++i)
        parts[i].sprite.clear();
    const int T = static_cast<int>(PartSlot::Torso);
    base(part(*this, PartSlot::Torso), "wisp_glow", 0.f, -16.f, 0.f, 32.f, 32.f, -1, 4.f);
    base(part(*this, PartSlot::Head), "wisp0", 0.f, -2.f, 1.f, 32.f, 32.f, T, 2.f);
}

void Puppet::setupGolem() {
    for (int i = 0; i < static_cast<int>(PartSlot::Count); ++i)
        parts[i].sprite.clear();
    const int T = static_cast<int>(PartSlot::Torso);
    base(part(*this, PartSlot::FarLeg), "golem_legs0", 0.f, -24.f, 0.f, 32.f, 32.f, -1, 6.f);
    base(part(*this, PartSlot::Torso), "golem_torso", 0.f, -22.f, 1.f, 32.f, 32.f, static_cast<int>(PartSlot::FarLeg), 4.f);
    base(part(*this, PartSlot::Head), "golem_head", 0.f, -16.f, 2.f, 32.f, 32.f, T, 2.f);
}

void Puppet::setupTrainer() {
    for (int i = 0; i < static_cast<int>(PartSlot::Count); ++i)
        parts[i].sprite.clear();
    base(part(*this, PartSlot::Torso), "npc_trainer", 0.f, -32.f, 0.f, 32.f, 32.f, -1, 2.f);
}

void Puppet::tick(float dt, bool moving, bool onGround, float faceDir) {
    anim += dt;
    flipX = faceDir < 0.f;

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
    farm.maxExtra = 6.f;
    weap.maxExtra = 10.f;

    if (pose == PuppetPose::Attack) {
        // Multi-frame: wind-up, thrust, hold, recover. Driven by anim (reset on strike).
        float t = anim;
        farm.sprite = "arm_f2";
        weap.sprite = "spear1";
        nleg.sprite = "leg0";
        fleg.sprite = "leg_far0";
        if (t < kAttackWindup) {
            float u = t / kAttackWindup;
            farm.extra = {-8.f * u, -1.f * u};
            weap.extra = {-6.f * u, 1.f * u};
            torso.extra = {-2.f * u, 0.f};
            head.extra = {-1.f * u, 0.f};
            nleg.extra = {2.f * u, 0.f};
            cape.extra = {4.f * u, 0.f};
            farm.sprite = "arm_f1";
            weap.sprite = "spear0";
        } else if (t < kAttackDuration - kAttackRecover) {
            farm.maxExtra = 18.f;
            weap.maxExtra = 8.f;
            farm.extra = {16.f, 1.f};
            weap.extra = {6.f, 0.f};
            torso.extra = {5.f, 0.f};
            head.extra = {2.f, 0.f};
            nleg.extra = {-2.f, 0.f};
            fleg.extra = {3.f, 0.f};
            cape.extra = {-6.f, 1.f};
        } else {
            float u = std::min(1.f, (t - (kAttackDuration - kAttackRecover)) / kAttackRecover);
            farm.extra = {16.f * (1.f - u), 0.f};
            weap.extra = {4.f * (1.f - u), 0.f};
            torso.extra = {5.f * (1.f - u), 0.f};
            farm.maxExtra = 18.f;
        }
        cape.sprite = "cape2";
    } else if (pose == PuppetPose::Dash) {
        farm.sprite = "arm_f1";
        weap.sprite = "spear1";
        torso.scaleX = 1.08f;
        torso.scaleY = 0.92f;
        torso.extra = {3.f, 0.f};
        head.extra = {1.5f, 1.f};
        cape.extra = {-8.f, 0.f};
        nleg.sprite = "leg1";
        fleg.sprite = "leg_far1";
        nleg.extra = {4.f, 0.f};
        fleg.extra = {-2.f, 0.f};
        cape.sprite = "cape2";
    } else if (pose == PuppetPose::Guard) {
        farm.sprite = "arm_f1";
        weap.sprite = "spear0";
        torso.extra = {-3.f, 0.f};
        cape.extra = {4.f, 0.f};
        farm.extra = {-4.f, 2.f};
    } else if (pose == PuppetPose::Hurt) {
        head.extra = {-2.f, 1.f};
        cape.extra = {5.f, 0.f};
        torso.extra = {-2.f, 0.f};
    } else if (!onGround || pose == PuppetPose::Jump) {
        pose = PuppetPose::Jump;
        nleg.sprite = "leg_tuck";
        fleg.sprite = "leg_tuck";
        nleg.extra = {2.f, -6.f}; // knees tuck toward hips
        fleg.extra = {-2.f, -6.f};
        torso.scaleY = 1.06f;
        torso.scaleX = 0.96f;
        head.extra = {0.f, -1.5f};
        farm.sprite = "arm_f1";
        barm.sprite = "arm_b1";
        farm.extra = {2.f, -4.f};
        barm.extra = {-2.f, -4.f};
        weap.sprite = "spear0";
        weap.extra = {2.f, -3.f};
        cape.sprite = "cape2";
        cape.extra = {-capeFollowX * 0.4f, 3.f};
    } else if (moving) {
        pose = PuppetPose::Walk;
        const float period = 0.4f;
        float th = anim * (2.f * kPi / period);
        float near = std::sin(th);
        float farv = std::sin(th + kPi);
        float liftN = std::max(0.f, -std::cos(th)) * -4.f;
        float liftF = std::max(0.f, -std::cos(th + kPi)) * -4.f;
        nleg.sprite = (near > 0.f) ? "leg1" : "leg0";
        fleg.sprite = (farv > 0.f) ? "leg_far1" : "leg_far0";
        nleg.extra = {near * 5.f, liftN};
        fleg.extra = {farv * 5.f, liftF};
        farm.sprite = (near > 0.f) ? "arm_f0" : "arm_f1";
        barm.sprite = (near > 0.f) ? "arm_b1" : "arm_b0";
        farm.extra = {-near * 4.f, 0.f};
        barm.extra = {near * 4.f, 0.f};
        weap.extra = {-near * 2.f, 0.f};
        weap.sprite = "spear0";
        torso.extra = {-near * 1.2f, 0.f};
        head.sprite = (std::sin(th) > 0) ? "head1" : "head0";
        head.extra = {0.f, std::sin(th * 2.f) * 1.4f};
        int cf = static_cast<int>(std::floor(anim * 6.f + 0.8f)) % 3;
        cape.sprite = cf == 0 ? "cape0" : (cf == 1 ? "cape1" : "cape2");
        cape.extra = {-capeFollowX * 0.35f - near * 0.8f, 0.f};
        torso.sprite = "torso0";
        capeFollowX += (torso.extra.x - capeFollowX) * std::min(1.f, dt * 8.f);
    } else {
        pose = PuppetPose::Idle;
        float breath = std::sin(anim * 2.4f);
        torso.scaleY = 1.f + breath * 0.03f;
        torso.extra = {0.f, breath * -0.6f};
        torso.sprite = (breath > 0.3f) ? "torso1" : "torso0";
        head.extra = {0.f, breath * -0.4f};
        head.sprite = "head0";
        nleg.sprite = "leg0";
        fleg.sprite = "leg_far0";
        farm.sprite = "arm_f0";
        barm.sprite = "arm_b0";
        weap.sprite = "spear0";
        int cf = static_cast<int>(std::floor(anim * 2.f)) % 3;
        cape.sprite = cf == 0 ? "cape0" : (cf == 1 ? "cape1" : "cape2");
        cape.extra = {std::sin(anim * 1.7f) * 1.5f, 0.f};
        capeFollowX += (0.f - capeFollowX) * std::min(1.f, dt * 6.f);
    }

    for (int i = 0; i < static_cast<int>(PartSlot::Count); ++i)
        parts[i].extra = clampExtra(parts[i].extra, parts[i].maxExtra);
}

void Puppet::draw(Renderer& r, const Texture& atlas, glm::vec2 feet, const glm::vec4& tint) const {
    glm::vec2 world[static_cast<int>(PartSlot::Count)];
    bool ready[static_cast<int>(PartSlot::Count)] = {};
    auto resolve = [&](auto&& self, int i) -> glm::vec2 {
        if (ready[i]) return world[i];
        const BodyPart& p = parts[i];
        glm::vec2 parentW = feet;
        if (p.parent >= 0 && p.parent < static_cast<int>(PartSlot::Count))
            parentW = self(self, p.parent);
        glm::vec2 local = p.local + p.extra;
        if (flipX) local.x = -local.x;
        world[i] = parentW + local;
        ready[i] = true;
        return world[i];
    };
    for (int i = 0; i < static_cast<int>(PartSlot::Count); ++i) {
        if (!parts[i].sprite.empty()) resolve(resolve, i);
    }

    struct Item { float z; int i; };
    std::vector<Item> order;
    for (int i = 0; i < static_cast<int>(PartSlot::Count); ++i) {
        if (parts[i].sprite.empty()) continue;
        order.push_back({parts[i].z, i});
    }
    std::sort(order.begin(), order.end(), [](const Item& a, const Item& b) { return a.z < b.z; });
    for (auto& it : order) {
        const BodyPart& p = parts[it.i];
        float w = p.w * p.scaleX;
        float h = p.h * p.scaleY;
        Rect dest{world[it.i].x - w * 0.5f, world[it.i].y, w, h};
        r.drawSprite(atlas, dest, spriteUV(p.sprite), tint, flipX);
    }
}

} // namespace tempest
