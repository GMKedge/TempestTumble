#include "Level.hpp"
#include <filesystem>
#include <iostream>
#include <queue>
#include <string>
#include <vector>

// Mirrors Game.cpp motion so maps stay clearable without Bolt Step:
//   kMaxSpeed=95, kJumpVel=-210, kGravity=520
//   hang=0.808s  jumpDist=76.7px=4.80 tiles  jumpH=42.4px=2.65 tiles
// Mandatory pits: <= 3 tiles. Jump reach used here: dx<=4, up<=2.
static const int kMaxJumpDx = 4;
static const int kMaxJumpUp = 2;

static bool air(const tempest::Tilemap& map, int x, int y) {
    if (x < 0 || y < 0 || x >= map.width() || y >= map.height()) return false;
    tempest::Tile t = map.at(x, y);
    return t == tempest::Tile::Empty || t == tempest::Tile::Door || t == tempest::Tile::DoorOpen ||
           t == tempest::Tile::Bush || t == tempest::Tile::Flower || t == tempest::Tile::Banner;
}

static bool standable(const tempest::Tilemap& map, int x, int y) {
    if (!air(map, x, y)) return false;
    return map.solid(x, y + 1);
}

static bool canReachDoor(const tempest::Tilemap& map, int px, int py, int& visited) {
    const int w = map.width(), h = map.height();
    std::vector<char> seen(static_cast<size_t>(w * h), 0);
    std::queue<std::pair<int, int>> q;
    auto idx = [&](int x, int y) { return y * w + x; };
    if (standable(map, px, py)) {
        q.push({px, py});
        seen[static_cast<size_t>(idx(px, py))] = 1;
    }
    int doorX = -1, doorY = -1;
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            if (map.isDoor(x, y)) { doorX = x; doorY = y; }
    visited = 0;
    while (!q.empty()) {
        auto [x, y] = q.front();
        q.pop();
        visited++;
        if (map.isDoor(x, y) || (doorX >= 0 && x == doorX && (y == doorY || y + 1 == doorY)))
            return true;
        auto tryGo = [&](int nx, int ny) {
            if (nx < 0 || ny < 0 || nx >= w || ny >= h) return;
            if (seen[static_cast<size_t>(idx(nx, ny))]) return;
            if (!standable(map, nx, ny)) return;
            seen[static_cast<size_t>(idx(nx, ny))] = 1;
            q.push({nx, ny});
        };
        tryGo(x - 1, y);
        tryGo(x + 1, y);
        for (int dx = -kMaxJumpDx; dx <= kMaxJumpDx; ++dx) {
            for (int dy = -kMaxJumpUp; dy <= 3; ++dy) {
                if (dx == 0 && dy == 0) continue;
                tryGo(x + dx, y + dy);
            }
        }
    }
    return false;
}

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
    int solids = 0;
    int ptx = -1, pty = -1;
    for (auto& m : map.markers()) {
        if (m.ch == 'P') { player = true; ptx = m.tx; pty = m.ty; }
    }
    for (int y = 0; y < map.height(); ++y)
        for (int x = 0; x < map.width(); ++x) {
            if (map.at(x, y) == tempest::Tile::Door) door = true;
            if (map.solid(x, y)) solids++;
        }
    int yFloor = map.height() - 3;
    int run = 0, wideMandatory = 0;
    for (int x = 0; x < map.width(); ++x) {
        bool hole = !map.solid(x, yFloor) && !map.solid(x, map.height() - 1);
        if (hole) {
            run++;
        } else {
            if (run > 3) wideMandatory++;
            run = 0;
        }
    }
    if (run > 3) wideMandatory++;
    if (player) {
        for (int yy = pty; yy < map.height(); ++yy)
            if (map.solid(ptx, yy)) { floorUnder = true; break; }
    }
    int vis = 0;
    bool reach = player && canReachDoor(map, ptx, pty, vis);
    std::cout << path << "  " << map.width() << "x" << map.height()
              << "  P=" << player << " door=" << door << " floor=" << floorUnder
              << " solids=" << solids << " widePits=" << wideMandatory
              << " reachDoor=" << reach << " vis=" << vis
              << " npcs=" << data.npcs.size() << "\n";
    if (!player) { std::cerr << "FAIL no Player marker\n"; return false; }
    if (!door) { std::cerr << "FAIL no Door tiles\n"; return false; }
    if (!floorUnder) { std::cerr << "FAIL no solid floor under spawn\n"; return false; }
    if (solids < 20) { std::cerr << "FAIL too few solids\n"; return false; }
    if (!reach) { std::cerr << "FAIL door not reachable with jump (no dash required)\n"; return false; }
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
