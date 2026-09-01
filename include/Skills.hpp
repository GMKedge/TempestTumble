#pragma once
#include "Config.hpp"
#include <string>
#include <vector>

namespace tempest {

enum class SkillId {
    TempestStrike = 0,
    WiderStrike,
    CycloneCleave,
    Stormguard,
    BoltStep,
    Updraft,
    GaleVault,
    AirCurrent,
    MaxHp,
    SpearDamage,
    EssenceMagnet,
    Count
};

struct SkillNode {
    SkillId id = SkillId::TempestStrike;
    SkillId prereq = SkillId::Count; // Count = none
    int cost = 1;
    const char* name = "";
    const char* desc = "";
    const char* icon = "node_core";
    int col = 0, row = 0;
    bool kSlot = false; // can be equipped on K
};

struct SkillState {
    bool owned[static_cast<int>(SkillId::Count)]{};
    int points = 0;
    int xp = 0;
    int level = 1;
    int gold = 0;
    SkillId equippedK = SkillId::TempestStrike;
    int cursor = 0;
};

const SkillNode& skillDef(SkillId id);
const SkillNode* skillTable(int& n);
bool hasSkill(const SkillState& s, SkillId id);
bool canBuy(const SkillState& s, SkillId id);
bool buySkill(SkillState& s, SkillId id);
void grantXp(SkillState& s, int amount, std::string& message, float& messageTimer, bool& ding);
void applyStats(const SkillState& s, int& maxHp, int& damage, float& magnet, float& coyote, float& dashTime);
void loadSkills(SkillState& s, const std::string& path);
void saveSkills(const SkillState& s, const std::string& path);
std::vector<SkillId> kSlotSkills(const SkillState& s);
void cycleEquippedK(SkillState& s);

} // namespace tempest
