#include "Level.hpp"
#include <filesystem>
#include <iostream>
#include <string>

static bool checkOne(const std::string& path) {
    tempest::LevelData data;
    if (!tempest::loadLevelFile(path, data)) {
        std::cerr << "FAIL load: " << path << "\n";
        return false;
    }
    const auto& map = data.map;
    if (map.width() <= 0 || map.height() <= 0) {
        std::cerr << "FAIL empty map: " << path << "\n";
        return false;
    }
    bool player = false, door = false, floorUnder = false;
    int pits = 0, solids = 0;
    int ptx = -1, pty = -1;
    for (auto& m : map.markers()) {
        if (m.ch == 'P') { player = true; ptx = m.tx; pty = m.ty; }
    }
    for (int y = 0; y < map.height(); ++y)
        for (int x = 0; x < map.width(); ++x) {
            if (map.at(x, y) == tempest::Tile::Door) door = true;
            if (map.solid(x, y)) solids++;
        }
    // A pit is a run of empty tiles on the lowest solid-row neighborhood.
    int yFloor = map.height() - 2;
    int run = 0;
    for (int x = 0; x < map.width(); ++x) {
        bool hole = !map.solid(x, yFloor) && !map.solid(x, map.height() - 1);
        if (hole) { run++; if (run >= 3) pits++; }
        else run = 0;
    }
    if (player) {
        for (int yy = pty; yy < map.height(); ++yy)
            if (map.solid(ptx, yy)) { floorUnder = true; break; }
    }
    std::cout << path << "  " << map.width() << "x" << map.height()
              << "  P=" << player << " door=" << door << " floor=" << floorUnder
              << " solids=" << solids << " pitRuns=" << pits
              << " npcs=" << data.npcs.size() << "\n";
    if (!player) { std::cerr << "FAIL no Player marker\n"; return false; }
    if (!door) { std::cerr << "FAIL no Door tiles\n"; return false; }
    if (!floorUnder) { std::cerr << "FAIL no solid floor under spawn\n"; return false; }
    if (solids < 20) { std::cerr << "FAIL too few solids\n"; return false; }
    return true;
}

int main(int argc, char** argv) {
    namespace fs = std::filesystem;
    fs::path root = argc > 1 ? fs::path(argv[1]) : fs::current_path();
    fs::path dir = root / "levels";
    int ok = 0, n = 0;
    for (auto& e : fs::directory_iterator(dir)) {
        if (e.path().extension() != ".txt") continue;
        n++;
        if (checkOne(e.path().string())) ok++;
        else return 1;
    }
    if (n == 0) { std::cerr << "no levels\n"; return 1; }
    std::cout << "OK " << ok << "/" << n << " levels\n";
    return 0;
}
