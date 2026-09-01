#include "Skills.hpp"
#include <algorithm>

namespace tempest {
namespace {
const SkillNode kNodes[] = {
    {SkillId::TempestStrike, SkillId::Count, 0, "TEMPEST STRIKE", "FREE CORE LUNGE", "node_core", 0, 1, false},
    {SkillId::WiderStrike, SkillId::TempestStrike, 1, "WIDER STRIKE", "LONGER HITBOX", "node_atk", 1, 0, false},
    {SkillId::CycloneCleave, SkillId::WiderStrike, 1, "CYCLONE CLEAVE", "SPIN AND CARRY", "node_atk", 2, 0, true},
    {SkillId::Stormguard, SkillId::CycloneCleave, 1, "STORMGUARD", "SHIELD KNOCKBACK", "node_atk", 3, 0, true},
    {SkillId::BoltStep, SkillId::TempestStrike, 1, "BOLT STEP", "DASH WITH IFRAMES", "node_mob", 1, 2, false},
    {SkillId::Updraft, SkillId::BoltStep, 1, "UPDRAFT", "AIR BURST JUMP", "node_mob", 2, 2, true},
    {SkillId::GaleVault, SkillId::Updraft, 1, "GALE VAULT", "LEAP WALLS AND PITS", "node_mob", 3, 2, true},
    {SkillId::AirCurrent, SkillId::GaleVault, 1, "AIR CURRENT", "COYOTE AND LONG DASH", "node_mob", 4, 2, false},
    {SkillId::MaxHp, SkillId::TempestStrike, 1, "STORM HEART", "PLUS TWO MAX HP", "node_hp", 1, 3, false},
    {SkillId::SpearDamage, SkillId::TempestStrike, 1, "RUNED SPEAR", "PLUS ONE DAMAGE", "node_atk", 2, 3, false},
    {SkillId::EssenceMagnet, SkillId::TempestStrike, 1, "ESSENCE MAGNET", "WIDER COIN PULL", "node_stat", 3, 3, false},
};
constexpr int kN = static_cast<int>(sizeof(kNodes) / sizeof(kNodes[0]));
}

const SkillNode& skillDef(SkillId id) {
    return kNodes[static_cast<int>(id)];
}
const SkillNode* skillTable(int& n) { n = kN; return kNodes; }

bool hasSkill(const SkillState& s, SkillId id) {
    return s.owned[static_cast<int>(id)];
}

bool canBuy(const SkillState& s, SkillId id) {
    if (hasSkill(s, id)) return false;
    const SkillNode& n = skillDef(id);
    if (s.points < n.cost) return false;
    if (n.prereq == SkillId::Count) return true;
    return hasSkill(s, n.prereq);
}

bool buySkill(SkillState& s, SkillId id) {
    if (!canBuy(s, id)) return false;
    s.owned[static_cast<int>(id)] = true;
    s.points -= skillDef(id).cost;
    if (skillDef(id).kSlot) s.equippedK = id;
    return true;
}

void grantXp(SkillState& s, int amount, std::string& message, float& messageTimer, bool& ding) {
    ding = false;
    s.xp += amount;
    int need = 8 + (s.level - 1) * 4;
    while (s.xp >= need) {
        s.xp -= need;
        s.level += 1;
        s.points += 1;
        message = "POINT EARNED - OPEN SKILL TREE";
        messageTimer = 3.5f;
        ding = true;
        need = 8 + (s.level - 1) * 4;
    }
}

void applyStats(const SkillState& s, int& maxHp, int& damage, float& magnet, float& coyote, float& dashTime) {
    maxHp = 5 + (hasSkill(s, SkillId::MaxHp) ? 2 : 0);
    damage = 1 + (hasSkill(s, SkillId::SpearDamage) ? 1 : 0);
    magnet = hasSkill(s, SkillId::EssenceMagnet) ? 28.f : 10.f;
    coyote = hasSkill(s, SkillId::AirCurrent) ? 0.16f : 0.08f;
    dashTime = hasSkill(s, SkillId::AirCurrent) ? 0.18f : 0.12f;
}

void loadSkills(SkillState& s, const std::string& path) {
    s = SkillState{};
    s.owned[static_cast<int>(SkillId::TempestStrike)] = true;
    Config cfg;
    if (!cfg.load(path)) return;
    s.level = cfg.getInt("level", 1);
    s.xp = cfg.getInt("xp", 0);
    s.points = cfg.getInt("points", 0);
    s.gold = cfg.getInt("gold", 0);
    s.equippedK = static_cast<SkillId>(cfg.getInt("equipped_k", 0));
    for (int i = 0; i < static_cast<int>(SkillId::Count); ++i) {
        std::string key = std::string("skill_") + std::to_string(i);
        s.owned[i] = cfg.getInt(key, i == 0 ? 1 : 0) != 0;
    }
    s.owned[0] = true;
}

void saveSkills(const SkillState& s, const std::string& path) {
    Config cfg;
    cfg.setInt("level", s.level);
    cfg.setInt("xp", s.xp);
    cfg.setInt("points", s.points);
    cfg.setInt("gold", s.gold);
    cfg.setInt("equipped_k", static_cast<int>(s.equippedK));
    for (int i = 0; i < static_cast<int>(SkillId::Count); ++i)
        cfg.setInt(std::string("skill_") + std::to_string(i), s.owned[i] ? 1 : 0);
    cfg.save(path);
}

std::vector<SkillId> kSlotSkills(const SkillState& s) {
    std::vector<SkillId> out;
    int n = 0;
    auto* t = skillTable(n);
    for (int i = 0; i < n; ++i)
        if (t[i].kSlot && hasSkill(s, t[i].id)) out.push_back(t[i].id);
    return out;
}

void cycleEquippedK(SkillState& s) {
    auto slots = kSlotSkills(s);
    if (slots.empty()) return;
    auto it = std::find(slots.begin(), slots.end(), s.equippedK);
    if (it == slots.end() || it + 1 == slots.end()) s.equippedK = slots.front();
    else s.equippedK = *(it + 1);
}

} // namespace tempest
