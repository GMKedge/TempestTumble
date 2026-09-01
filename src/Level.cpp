#include "Level.hpp"
#include <fstream>
#include <sstream>
#include <cstdio>

namespace tempest {
namespace {
std::string stripCr(std::string s) {
    if (!s.empty() && s.back() == '\r') s.pop_back();
    return s;
}
std::string rtrimSpaces(std::string s) {
    while (!s.empty() && s.back() == ' ') s.pop_back();
    return s;
}

void parseNpcLine(const std::string& line, LevelData& out) {
    NpcDef n;
    std::istringstream ss(line.substr(4));
    ss >> n.tx >> n.ty >> n.role;
    std::string rest;
    std::getline(ss, rest);
    if (!rest.empty() && rest[0] == ' ') rest.erase(0, 1);
    auto bar = rest.find('|');
    if (bar != std::string::npos) {
        n.title = rest.substr(0, bar);
        n.line = rest.substr(bar + 1);
    } else {
        n.title = n.role;
        n.line = rest;
    }
    out.npcs.push_back(n);
}
}

bool loadLevelFile(const std::string& path, LevelData& out) {
    std::ifstream in(path);
    if (!in) return false;
    out = LevelData{};
    std::string line;
    bool inTiles = false;
    std::vector<std::string> rows;
    while (std::getline(in, line)) {
        line = stripCr(line);
        if (rtrimSpaces(line).rfind("npc ", 0) == 0) {
            parseNpcLine(rtrimSpaces(line), out);
            continue;
        }
        if (!inTiles) {
            line = rtrimSpaces(line);
            if (line == "tiles:") { inTiles = true; continue; }
            auto eq = line.find('=');
            if (eq == std::string::npos || line.empty() || line[0] == '#') continue;
            auto k = line.substr(0, eq);
            auto v = line.substr(eq + 1);
            if (k == "id") out.id = v;
            else if (k == "name") out.name = v;
            else if (k == "music") out.music = v;
            else if (k == "next") out.next = v;
            else if (k == "sky") {
                std::sscanf(v.c_str(), "%f,%f,%f", &out.sky.x, &out.sky.y, &out.sky.z);
            }
        } else {
            if (rtrimSpaces(line) == "end") {
                inTiles = false;
                continue;
            }
            rows.push_back(line);
        }
    }
    if (rows.empty()) return false;
    out.map.loadFromRows(rows);
    return true;
}

} // namespace tempest
