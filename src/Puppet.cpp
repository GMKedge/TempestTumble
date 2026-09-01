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

glm::vec2 sprSz(const char* name, float fbW, float fbH) {
    glm::vec2 s = spriteSize(name);
    if (s.x < 1.f) s.x = fbW;
    if (s.y < 1.f) s.y = fbH;
    return s;
}

constexpr float kPi = 3.14159265f;

void resolveWorld(const Puppet& pup, glm::vec2 feet, bool flipX, glm::vec2* world, bool* ready) {
    auto resolve = [&](auto&& self, int i) -> glm::vec2 {
        if (ready[i]) return world[i];
        const BodyPart& p = pup.parts[i];
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
        ready[i] = false;
        if (!pup.parts[i].sprite.empty()) resolve(resolve, i);
    }
}

Rect partDest(const BodyPart& p, glm::vec2 world) {
    float w = p.w * p.scaleX;
    float h = p.h * p.scaleY;
    return Rect{world.x - w * 0.5f, world.y, w, h};
}
} // namespace

void Puppet::setupGale() {
    const int T = static_cast<int>(PartSlot::Torso);
    const int A = static_cast<int>(PartSlot::FrontArm);
    glm::vec2 head = sprSz("gale_head0", 30.f, 36.f);
    glm::vec2 torso = sprSz("gale_torso0", 38.f, 40.f);
    glm::vec2 leg = sprSz("gale_legL0", 24.f, 38.f);
    glm::vec2 arm = sprSz("gale_armF0", 22.f, 36.f);
    glm::vec2 cape = sprSz("gale_cape0", 21.f, 48.f);
    glm::vec2 swd = sprSz("gale_sword0", 10.f, 36.f);

    // Hips/feet origin. Rest offsets measured from assembled sheet poses (1/4 scale):
    // legs sit on the ground, torso overlaps hips, helm sits on the gorget.
    const float hip = 8.f;
    base(part(*this, PartSlot::FarLeg), "gale_legR0", -hip, -leg.y, 2.f, leg.x, leg.y, -1, 7.f);
    base(part(*this, PartSlot::NearLeg), "gale_legL0", hip, -leg.y, 4.f, leg.x, leg.y, -1, 7.f);
    float torsoTop = -leg.y - torso.y + 18.f;
    base(part(*this, PartSlot::Torso), "gale_torso0", 0.f, torsoTop, 3.f, torso.x, torso.y, -1, 5.f);
    base(part(*this, PartSlot::Head), "gale_head0", 1.f, -head.y + 14.f, 5.f, head.x, head.y, T, 3.f);
    float capeY = std::min(4.f, -torsoTop - cape.y + 2.f);
    base(part(*this, PartSlot::Cape), "gale_cape0", -6.f, capeY, 0.f, cape.x, cape.y, T, 8.f);
    base(part(*this, PartSlot::BackArm), "gale_armB0", -arm.x * 0.28f, 6.f, 1.f, arm.x, arm.y, T, 6.f);
    base(part(*this, PartSlot::FrontArm), "gale_armF0", arm.x * 0.32f, 7.f, 6.f, arm.x, arm.y, T, 6.f);
    glm::vec2 thrust = sprSz("gale_sword_thrust", 41.f, 21.f);
    (void)thrust;
    base(part(*this, PartSlot::Weapon), "gale_sword0", swd.x * 0.15f + 10.f, 10.f, 7.f, swd.x, swd.y, A, 10.f);
    pose = PuppetPose::Idle;
}

void Puppet::setupWisp() {
    for (int i = 0; i < static_cast<int>(PartSlot::Count); ++i)
        parts[i].sprite.clear();
    const int T = static_cast<int>(PartSlot::Torso);
    glm::vec2 g = sprSz("wisp_glow", 32.f, 32.f);
    glm::vec2 h = sprSz("wisp0", 32.f, 32.f);
    const float s = 40.f / 32.f;
    base(part(*this, PartSlot::Torso), "wisp_glow", 0.f, -g.y * s, 0.f, g.x * s, g.y * s, -1, 4.f);
    base(part(*this, PartSlot::Head), "wisp0", 0.f, 4.f, 1.f, h.x * s, h.y * s, T, 2.f);
}

void Puppet::setupGolem() {
    for (int i = 0; i < static_cast<int>(PartSlot::Count); ++i)
        parts[i].sprite.clear();
    glm::vec2 lg = sprSz("golem_legs0", 32.f, 32.f);
    glm::vec2 to = sprSz("golem_torso", 32.f, 32.f);
    glm::vec2 hd = sprSz("golem_head", 32.f, 32.f);
    const float s = 44.f / 32.f;
    const int L = static_cast<int>(PartSlot::FarLeg);
    const int T = static_cast<int>(PartSlot::Torso);
    base(part(*this, PartSlot::FarLeg), "golem_legs0", 0.f, -lg.y * s, 0.f, lg.x * s, lg.y * s, -1, 6.f);
    base(part(*this, PartSlot::Torso), "golem_torso", 0.f, -to.y * s + 12.f, 1.f, to.x * s, to.y * s, L, 4.f);
    base(part(*this, PartSlot::Head), "golem_head", 0.f, -hd.y * s + 10.f, 2.f, hd.x * s, hd.y * s, T, 2.f);
}

void Puppet::setupTrainer() {
    for (int i = 0; i < static_cast<int>(PartSlot::Count); ++i)
        parts[i].sprite.clear();
    glm::vec2 n = sprSz("npc_trainer", 32.f, 32.f);
    const float s = 48.f / 32.f;
    base(part(*this, PartSlot::Torso), "npc_trainer", 0.f, -n.y * s, 0.f, n.x * s, n.y * s, -1, 2.f);
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

    auto armF = [](int i) {
        static const char* n[] = {"gale_armF0", "gale_armF1", "gale_armF2", "gale_armF3"};
        return n[i & 3];
    };
    auto armB = [](int i) {
        static const char* n[] = {"gale_armB0", "gale_armB1", "gale_armB2", "gale_armB3"};
        return n[i & 3];
    };
    auto legL = [](int i) {
        static const char* n[] = {
            "gale_legL0", "gale_legL1", "gale_legL2", "gale_legL3", "gale_legL4", "gale_legL5"};
        return n[i % 6];
    };
    auto legR = [](int i) {
        static const char* n[] = {
            "gale_legR0", "gale_legR1", "gale_legR2", "gale_legR3", "gale_legR4", "gale_legR5"};
        return n[i % 6];
    };
    auto capeN = [](int i) {
        static const char* n[] = {"gale_cape0", "gale_cape1", "gale_cape2"};
        return n[i % 3];
    };

    if (pose == PuppetPose::Attack) {
        float t = anim;
        nleg.sprite = "gale_legL0";
        fleg.sprite = "gale_legR2";
        head.sprite = "gale_head0";
        torso.sprite = "gale_torso0";
        cape.sprite = "gale_cape2";
        if (t < kAttackWindup) {
            float u = t / kAttackWindup;
            farm.sprite = armF(1);
            barm.sprite = armB(1);
            weap.sprite = "gale_sword1";
            farm.extra = {-7.f * u, -2.f * u};
            weap.extra = {-5.f * u, 2.f * u};
            torso.extra = {-2.f * u, 0.f};
            head.extra = {-1.f * u, 0.f};
            nleg.extra = {2.f * u, 0.f};
            cape.extra = {4.f * u, 0.f};
        } else if (t < kAttackDuration - kAttackRecover) {
            farm.maxExtra = 12.f;
            weap.maxExtra = 22.f;
            farm.sprite = armF(2);
            barm.sprite = armB(0);
            weap.sprite = "gale_sword_thrust";
            glm::vec2 tw = sprSz("gale_sword_thrust", 41.f, 21.f);
            weap.w = tw.x;
            weap.h = tw.y;
            farm.extra = {11.f, 2.f};
            weap.extra = {18.f, 1.f};
            torso.extra = {3.f, 0.f};
            head.extra = {1.f, 0.f};
            nleg.extra = {-2.f, 0.f};
            fleg.extra = {3.f, 0.f};
            cape.extra = {-5.f, 1.f};
        } else {
            float u = std::min(1.f, (t - (kAttackDuration - kAttackRecover)) / kAttackRecover);
            farm.maxExtra = 12.f;
            weap.maxExtra = 16.f;
            farm.sprite = armF(3);
            weap.sprite = "gale_sword0";
            glm::vec2 sw = sprSz("gale_sword0", 10.f, 36.f);
            weap.w = sw.x;
            weap.h = sw.y;
            farm.extra = {11.f * (1.f - u), 0.f};
            weap.extra = {10.f * (1.f - u), 0.f};
            torso.extra = {3.f * (1.f - u), 0.f};
        }
    } else if (pose == PuppetPose::Dash) {
        farm.sprite = armF(2);
        barm.sprite = armB(1);
        weap.sprite = "gale_sword_thrust";
        glm::vec2 tw = sprSz("gale_sword_thrust", 41.f, 21.f);
        weap.w = tw.x;
        weap.h = tw.y;
        torso.scaleX = 1.06f;
        torso.scaleY = 0.94f;
        torso.extra = {3.f, 0.f};
        head.extra = {1.5f, 1.f};
        cape.extra = {-7.f, 0.f};
        nleg.sprite = "gale_legL3";
        fleg.sprite = "gale_legR5";
        nleg.extra = {4.f, 0.f};
        fleg.extra = {-2.f, 0.f};
        cape.sprite = "gale_cape2";
    } else if (pose == PuppetPose::Guard) {
        farm.sprite = armF(1);
        weap.sprite = "gale_sword1";
        torso.extra = {-3.f, 0.f};
        cape.extra = {4.f, 0.f};
        farm.extra = {-4.f, 2.f};
        head.sprite = "gale_head1";
    } else if (pose == PuppetPose::Hurt) {
        head.extra = {-2.f, 1.f};
        cape.extra = {5.f, 0.f};
        torso.extra = {-2.f, 0.f};
        head.sprite = "gale_head1";
    } else if (!onGround || pose == PuppetPose::Jump) {
        pose = PuppetPose::Jump;
        nleg.sprite = "gale_leg_tuck";
        fleg.sprite = "gale_leg_tuck";
        nleg.extra = {2.f, -5.f};
        fleg.extra = {-2.f, -5.f};
        torso.scaleY = 1.04f;
        torso.scaleX = 0.97f;
        head.extra = {0.f, -1.5f};
        head.sprite = "gale_head2";
        farm.sprite = armF(1);
        barm.sprite = armB(1);
        farm.extra = {2.f, -3.f};
        barm.extra = {-2.f, -3.f};
        weap.sprite = "gale_sword1";
        weap.extra = {2.f, -2.f};
        cape.sprite = "gale_cape2";
        cape.extra = {-capeFollowX * 0.4f, 3.f};
    } else if (moving) {
        pose = PuppetPose::Walk;
        const float period = 0.48f;
        float th = anim * (2.f * kPi / period);
        float near = std::sin(th);
        float farv = std::sin(th + kPi);
        float liftN = std::max(0.f, -std::cos(th)) * -3.5f;
        float liftF = std::max(0.f, -std::cos(th + kPi)) * -3.5f;
        int fi = static_cast<int>(std::floor(anim / period * 6.f));
        if (fi < 0) fi = 0;
        nleg.sprite = legL(fi);
        fleg.sprite = legR(fi + 3);
        nleg.extra = {near * 4.5f, liftN};
        fleg.extra = {farv * 4.5f, liftF};
        farm.sprite = armF((fi + 2) & 3);
        barm.sprite = armB(fi & 3);
        farm.extra = {-near * 3.5f, 0.f};
        barm.extra = {near * 3.5f, 0.f};
        weap.sprite = "gale_sword0";
        weap.extra = {-near * 1.6f, 0.f};
        torso.extra = {-near * 1.0f, 0.f};
        torso.sprite = "gale_torso0";
        int hi = (std::sin(th) > 0.35f) ? 2 : ((std::sin(th) < -0.35f) ? 1 : 0);
        head.sprite = hi == 0 ? "gale_head0" : (hi == 1 ? "gale_head1" : "gale_head2");
        glm::vec2 hs = sprSz(head.sprite.c_str(), 30.f, 36.f);
        head.w = hs.x;
        head.h = hs.y;
        head.extra = {0.f, std::sin(th * 2.f) * 1.2f};
        cape.sprite = capeN(static_cast<int>(std::floor(anim * 6.f + 0.8f)));
        cape.extra = {-capeFollowX * 0.35f - near * 0.7f, 0.f};
        capeFollowX += (torso.extra.x - capeFollowX) * std::min(1.f, dt * 8.f);
    } else {
        pose = PuppetPose::Idle;
        float breath = std::sin(anim * 2.4f);
        torso.scaleY = 1.f + breath * 0.025f;
        torso.extra = {0.f, breath * -0.5f};
        torso.sprite = (breath > 0.35f) ? "gale_torso1" : "gale_torso0";
        glm::vec2 ts = sprSz(torso.sprite.c_str(), 38.f, 40.f);
        torso.w = ts.x;
        torso.h = ts.y;
        head.extra = {0.f, breath * -0.35f};
        head.sprite = "gale_head0";
        nleg.sprite = "gale_legL0";
        fleg.sprite = "gale_legR0";
        farm.sprite = armF(0);
        barm.sprite = armB(0);
        weap.sprite = "gale_sword0";
        glm::vec2 sw = sprSz("gale_sword0", 10.f, 36.f);
        weap.w = sw.x;
        weap.h = sw.y;
        cape.sprite = capeN(static_cast<int>(std::floor(anim * 2.f)));
        cape.extra = {std::sin(anim * 1.7f) * 1.4f, 0.f};
        capeFollowX += (0.f - capeFollowX) * std::min(1.f, dt * 6.f);
    }

    for (int i = 0; i < static_cast<int>(PartSlot::Count); ++i)
        parts[i].extra = clampExtra(parts[i].extra, parts[i].maxExtra);
}

Rect Puppet::visualBounds(glm::vec2 feet, bool flipX, bool includeWeapon) const {
    glm::vec2 world[static_cast<int>(PartSlot::Count)];
    bool ready[static_cast<int>(PartSlot::Count)] = {};
    resolveWorld(*this, feet, flipX, world, ready);
    bool any = false;
    float x0 = 0, y0 = 0, x1 = 0, y1 = 0;
    for (int i = 0; i < static_cast<int>(PartSlot::Count); ++i) {
        if (parts[i].sprite.empty()) continue;
        if (!includeWeapon && i == static_cast<int>(PartSlot::Weapon)) continue;
        Rect d = partDest(parts[i], world[i]);
        if (!any) {
            x0 = d.x;
            y0 = d.y;
            x1 = d.x + d.w;
            y1 = d.y + d.h;
            any = true;
        } else {
            x0 = std::min(x0, d.x);
            y0 = std::min(y0, d.y);
            x1 = std::max(x1, d.x + d.w);
            y1 = std::max(y1, d.y + d.h);
        }
    }
    if (!any) return Rect{feet.x - 5.f, feet.y - 16.f, 10.f, 16.f};
    return Rect{x0, y0, x1 - x0, y1 - y0};
}

glm::vec2 Puppet::restBodySize() const {
    Rect v = visualBounds({0.f, 0.f}, false, false);
    const float inset = 2.f;
    return {std::max(8.f, v.w - inset * 2.f), std::max(12.f, v.h - inset * 2.f)};
}

void Puppet::draw(Renderer& r, const Texture& atlas, glm::vec2 feet, const glm::vec4& tint) const {
    glm::vec2 world[static_cast<int>(PartSlot::Count)];
    bool ready[static_cast<int>(PartSlot::Count)] = {};
    resolveWorld(*this, feet, flipX, world, ready);

    struct Item {
        float z;
        int i;
    };
    std::vector<Item> order;
    for (int i = 0; i < static_cast<int>(PartSlot::Count); ++i) {
        if (parts[i].sprite.empty()) continue;
        order.push_back({parts[i].z, i});
    }
    std::sort(order.begin(), order.end(), [](const Item& a, const Item& b) { return a.z < b.z; });
    for (auto& it : order) {
        const BodyPart& p = parts[it.i];
        r.drawSprite(atlas, partDest(p, world[it.i]), spriteUV(p.sprite), tint, flipX);
    }
}

} // namespace tempest
