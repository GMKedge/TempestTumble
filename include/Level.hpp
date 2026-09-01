#pragma once
#include "Tilemap.hpp"
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace tempest {

struct NpcDef {
    int tx = 0, ty = 0;
    std::string role;
    std::string title;
    std::string line;
};

struct LevelData {
    std::string id;
    std::string name;
    std::string music = "storm";
    std::string next;
    glm::vec3 sky{0.18f, 0.22f, 0.38f};
    Tilemap map;
    std::vector<NpcDef> npcs;
};

bool loadLevelFile(const std::string& path, LevelData& out);

} // namespace tempest
