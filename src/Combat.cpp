#include "Combat.hpp"
#include "Collision.hpp"
#include <algorithm>
#include <cmath>

namespace tempest {
namespace {
void hitEnemies(Entity& gale, std::vector<Entity>& enemies, Rect box, int dmg, float knock, Audio& audio) {
    for (auto& e : enemies) {
        if (!e.alive) continue;
        if (!box.overlaps(e.bounds())) continue;
        glm::vec2 k{(e.pos.x + e.size.x * 0.5f > gale.pos.x + gale.size.x * 0.5f) ? knock : -knock, -40.f};
        hurt(e, dmg, k, audio);
    }
}

Rect strikeBox(const Entity& gale, float reach) {
    Rect box;
    box.w = reach;
    box.h = gale.size.y;
    box.y = gale.pos.y;
    box.x = gale.facingRight ? gale.bounds().right() : gale.pos.x - box.w;
    return box;
}
}

void hurt(Entity& e, int dmg, const glm::vec2& knock, Audio& audio) {
    if (e.iTimer > 0.f) return;
    e.hp -= dmg;
    e.hurtFlash = 0.15f;
    e.iTimer = (e.kind == EntityKind::Player) ? 0.7f : 0.2f;
    e.vel += knock;
    audio.play("hurt");
    if (e.hp <= 0) {
        e.alive = false;
        e.hp = 0;
    }
}

void tickCombat(CombatState& c, Entity& gale, SkillState& skills, const Input& in,
                const Tilemap& map, std::vector<Entity>& enemies, Audio& audio, float dt) {
    (void)map;
    int maxHp, dmg;
    float magnet, coyote, dashDur;
    applyStats(skills, maxHp, dmg, magnet, coyote, dashDur);
    gale.maxHp = maxHp;
    gale.damage = dmg;
    if (gale.hp > gale.maxHp) gale.hp = gale.maxHp;

    if (c.attackT > 0) c.attackT -= dt;
    if (c.dashT > 0) c.dashT -= dt;
    if (c.guardT > 0) c.guardT -= dt;
    if (c.cycloneT > 0) c.cycloneT -= dt;
    if (c.lungeT > 0) c.lungeT -= dt;
    if (c.fxT > 0) c.fxT -= dt;
    if (gale.iTimer > 0) gale.iTimer -= dt;
    if (gale.hurtFlash > 0) gale.hurtFlash -= dt;
    if (gale.onGround) {
        c.coyote = coyote;
        c.usedUpdraft = false;
    } else {
        c.coyote -= dt;
    }

    float face = gale.facingRight ? 1.f : -1.f;
    bool busy = c.dashT > 0.f || c.cycloneT > 0.f || c.lungeT > 0.f;

    // Reach vs jump (see tools/physics.txt):
    //   jumpDist ~ 76.7 px (~4.8 tiles). Strike reach is 32 px (40 Wider) FROM the
    //   actor AABB edge, so Gale pokes without body-overlap. Enemy contact damage
    //   still requires THEIR body vs Gale's 10x16 hurtbox.
    if (in.pressed(Action::Attack) && c.attackT <= 0.f && c.dashT <= 0.f) {
        c.attackT = kAttackDuration;
        c.lungeT = 0.10f;
        gale.vel.x = face * 55.f; // ~5 px of body travel, not a tackle
        gale.iTimer = std::max(gale.iTimer, 0.05f);
        gale.puppet.pose = PuppetPose::Attack;
        gale.puppet.anim = 0.f;
        audio.play("hit");
        c.fxSprite = "slash";
        c.fxT = kAttackDuration - kAttackRecover;
    }

    bool strikeActive = c.attackT > 0.f && gale.puppet.anim >= kAttackWindup &&
                        gale.puppet.anim < (kAttackDuration - kAttackRecover);
    if (strikeActive) {
        float reach = hasSkill(skills, SkillId::WiderStrike) ? kStrikeReachWide : kStrikeReach;
        Rect box = strikeBox(gale, reach);
        hitEnemies(gale, enemies, box, gale.damage, 120.f, audio);
        c.fxPos = gale.facingRight ? glm::vec2{box.right() - 16.f, box.y} : glm::vec2{box.x, box.y};
        c.fxSprite = "slash";
        if (c.fxT < 0.05f) c.fxT = 0.08f;
    }

    if (c.lungeT > 0.f) {
        gale.vel.x = face * 50.f;
        gale.puppet.pose = PuppetPose::Attack;
    }

    if (in.pressed(Action::Ability) && !busy) {
        SkillId k = skills.equippedK;
        if (k == SkillId::CycloneCleave && hasSkill(skills, k)) {
            c.cycloneT = 0.38f;
            gale.iTimer = std::max(gale.iTimer, 0.2f);
            audio.play("hit");
            c.fxSprite = "cyclone";
            c.fxT = 0.38f;
        } else if (k == SkillId::Stormguard && hasSkill(skills, k)) {
            c.guardT = 0.35f;
            gale.vel.x = -face * 180.f;
            gale.iTimer = std::max(gale.iTimer, 0.35f);
            gale.puppet.pose = PuppetPose::Guard;
            audio.play("select");
            c.fxSprite = "shield";
            c.fxT = 0.3f;
            c.fxPos = gale.pos;
        } else if (k == SkillId::Updraft && hasSkill(skills, k) && !c.usedUpdraft) {
            gale.vel.y = -280.f;
            gale.vel.x += face * 40.f;
            c.usedUpdraft = true;
            gale.puppet.pose = PuppetPose::Jump;
            audio.play("jump");
        } else if (k == SkillId::GaleVault && hasSkill(skills, k)) {
            gale.vel.x = face * 170.f;
            gale.vel.y = -320.f;
            gale.iTimer = std::max(gale.iTimer, 0.1f);
            gale.puppet.pose = PuppetPose::Jump;
            audio.play("jump");
        }
    }

    if (c.cycloneT > 0.f) {
        gale.vel.x = face * 150.f;
        gale.puppet.pose = PuppetPose::Attack;
        gale.puppet.anim += dt * 8.f;
        // Ring in front and around — does not require body overlap.
        Rect box;
        box.w = gale.size.x + 48.f;
        box.h = gale.size.y + 10.f;
        box.y = gale.pos.y - 4.f;
        box.x = gale.facingRight ? gale.pos.x - 8.f : gale.pos.x - 40.f;
        hitEnemies(gale, enemies, box, gale.damage, 80.f, audio);
        c.fxPos = gale.facingRight ? glm::vec2{box.right() - 16.f, gale.pos.y} : glm::vec2{box.x, gale.pos.y};
    }

    if (in.pressed(Action::Dash) && hasSkill(skills, SkillId::BoltStep) && c.dashT <= 0.f) {
        c.dashT = dashDur;
        gale.vel.x = face * 420.f;
        gale.vel.y = 0.f;
        gale.iTimer = std::max(gale.iTimer, dashDur);
        gale.puppet.pose = PuppetPose::Dash;
        audio.play("dash");
    }
    if (c.dashT > 0.f) {
        gale.vel.x = face * 420.f;
        gale.vel.y = 0.f;
        gale.puppet.pose = PuppetPose::Dash;
        Rect box{gale.pos.x - 4.f, gale.pos.y, gale.size.x + 8.f, gale.size.y};
        hitEnemies(gale, enemies, box, gale.damage, 60.f, audio);
    }

    if (in.pressed(Action::CycleAbility)) {
        cycleEquippedK(skills);
        audio.play("select");
    }
}

} // namespace tempest
