#include "Menu.hpp"
#include "Audio.hpp"
#include "Atlas.hpp"
#include <algorithm>

namespace tempest {

void updatePauseMenu(MenuState& m, SkillState& skills, const Input& in, Audio* audio) {
    if (in.pressed(Action::Left) || in.pressed(Action::Right)) {
        m.tab = (m.tab == MenuTab::Sheet) ? MenuTab::Tree : MenuTab::Sheet;
        if (audio) audio->play("select");
    }
    if (m.tab != MenuTab::Tree) return;
    int n = 0;
    skillTable(n);
    if (in.pressed(Action::Up)) {
        skills.cursor = (skills.cursor + n - 1) % n;
        if (audio) audio->play("select");
    }
    if (in.pressed(Action::Down)) {
        skills.cursor = (skills.cursor + 1) % n;
        if (audio) audio->play("select");
    }
    if (in.pressed(Action::Interact) || in.pressed(Action::Attack) || in.pressed(Action::Jump)) {
        SkillId id = static_cast<SkillId>(skills.cursor);
        if (buySkill(skills, id)) {
            if (audio) audio->play("levelup");
        } else if (hasSkill(skills, id) && skillDef(id).kSlot) {
            skills.equippedK = id;
            if (audio) audio->play("select");
        } else if (audio) {
            audio->play("select");
        }
    }
}

void drawCharacterSheet(Renderer& r, const Texture& atlas, const SkillState& skills,
                        int hp, int maxHp, float x, float y) {
    r.drawQuad(Rect{x, y, 440, 280}, {0.08f, 0.09f, 0.16f, 0.92f});
    r.drawText(atlas, "GALE", {x + 16, y + 12}, 2.f, {1, 0.9f, 0.5f, 1});
    r.drawText(atlas, "STORM KNIGHT", {x + 16, y + 40}, 2.f, {0.6f, 0.8f, 1, 1});
    r.drawText(atlas, "LV " + std::to_string(skills.level), {x + 16, y + 72}, 2.f);
    int need = 8 + (skills.level - 1) * 4;
    r.drawText(atlas, "XP " + std::to_string(skills.xp) + "/" + std::to_string(need), {x + 140, y + 72}, 2.f);
    float ratio = need ? std::min(1.f, skills.xp / float(need)) : 0.f;
    r.drawQuad(Rect{x + 16, y + 108, 360, 10}, {0.2f, 0.2f, 0.3f, 1});
    r.drawQuad(Rect{x + 16, y + 108, 360 * ratio, 10}, {0.4f, 0.75f, 1.f, 1});
    r.drawText(atlas, "HP " + std::to_string(hp) + "/" + std::to_string(maxHp), {x + 16, y + 126}, 2.f);
    r.drawText(atlas, "ATK " + std::to_string(1 + (hasSkill(skills, SkillId::SpearDamage) ? 1 : 0)), {x + 220, y + 126}, 2.f);
    r.drawText(atlas, "ESSENCE " + std::to_string(skills.gold), {x + 16, y + 154}, 2.f);
    r.drawText(atlas, "POINTS " + std::to_string(skills.points), {x + 220, y + 154}, 2.f);
    r.drawText(atlas, "STRIKE  TEMPEST", {x + 16, y + 190}, 2.f, {0.8f, 0.9f, 1, 1});
    r.drawText(atlas, std::string("K SLOT  ") + skillDef(skills.equippedK).name, {x + 16, y + 214}, 2.f);
    const char* dash = hasSkill(skills, SkillId::BoltStep) ? "BOLT STEP READY" : "DASH LOCKED";
    r.drawText(atlas, dash, {x + 16, y + 238}, 2.f, {0.7f, 0.85f, 1, 1});
}

void drawSkillTree(Renderer& r, const Texture& atlas, const SkillState& skills, float x, float y) {
    r.drawQuad(Rect{x, y, 500, 320}, {0.08f, 0.09f, 0.16f, 0.92f});
    r.drawText(atlas, "SKILL TREE  PTS " + std::to_string(skills.points), {x + 12, y + 8}, 2.f, {1, 0.9f, 0.5f, 1});
    int n = 0;
    auto* nodes = skillTable(n);
    for (int i = 0; i < n; ++i) {
        const SkillNode& nd = nodes[i];
        float nx = x + 16 + nd.col * 72.f;
        float ny = y + 48 + nd.row * 48.f;
        bool own = hasSkill(skills, nd.id);
        bool ok = canBuy(skills, nd.id);
        glm::vec4 tint = own ? glm::vec4{1, 1, 1, 1} : (ok ? glm::vec4{0.8f, 0.8f, 1, 1} : glm::vec4{0.35f, 0.35f, 0.4f, 1});
        if (i == skills.cursor) {
            r.drawQuad(Rect{nx - 4, ny - 4, 32, 32}, {1.f, 0.85f, 0.3f, 1});
        }
        r.drawSprite(atlas, Rect{nx, ny, 24, 24}, spriteUV(nd.icon), tint);
    }
    const SkillNode& cur = nodes[std::clamp(skills.cursor, 0, n - 1)];
    r.drawText(atlas, cur.name, {x + 12, y + 250}, 2.f, {1, 1, 1, 1});
    bool own = hasSkill(skills, cur.id);
    bool ok = canBuy(skills, cur.id);
    std::string req = own ? "OWNED" : (ok ? "E/J TO BUY" : "LOCKED NEED PREREQ");
    r.drawText(atlas, req, {x + 12, y + 278}, 2.f, own ? glm::vec4{0.5f, 1, 0.6f, 1} : glm::vec4{1, 0.8f, 0.5f, 1});
}

void drawTitle(Renderer& r, const Texture& atlas, const MenuState& m, float bob) {
    r.drawText(atlas, "TEMPEST TUMBLE", {64, 48 + bob}, 3.f, {0.7f, 0.9f, 1, 1});
    r.drawText(atlas, "GALE THE STORM KNIGHT", {80, 96 + bob}, 2.f, {0.95f, 0.85f, 0.4f, 1});
    const char* items[] = {"START", "CHARACTER", "QUIT"};
    for (int i = 0; i < 3; ++i) {
        glm::vec4 tint = (m.titleIndex == i) ? glm::vec4{1, 0.9f, 0.4f, 1} : glm::vec4{0.8f, 0.85f, 1, 1};
        std::string line = (m.titleIndex == i ? "> " : "  ") + std::string(items[i]);
        r.drawText(atlas, line, {140, 168.f + i * 32.f}, 2.f, tint);
    }
    r.drawText(atlas, "JUMP OR START TO PLAY", {64, 320}, 2.f, {0.6f, 0.7f, 0.9f, 1});
}

} // namespace tempest
