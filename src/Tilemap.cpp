#include "Tilemap.hpp"
#include <algorithm>

namespace tempest {

Tile Tilemap::fromChar(char c) {
    switch (c) {
    case '#': return Tile::Cliff;
    case '=': return Tile::Dirt;
    case 'X': return Tile::Stone;
    case 'B': return Tile::Brick;
    case 'W': return Tile::Wood;
    case '^': return Tile::Spike;
    case '-': return Tile::Platform;
    case 'D': return Tile::Door;
    case '*': return Tile::Bush;
    case 'f': return Tile::Flower;
    case 'o': return Tile::Crate;
    case '.': return Tile::Cobble;
    case '~': return Tile::Water;
    case '|': return Tile::Banner;
    default:  return Tile::Empty;
    }
}

const char* Tilemap::spriteName(Tile t) {
    switch (t) {
    case Tile::Cliff: return "cliff";
    case Tile::Dirt: return "dirt";
    case Tile::Stone: return "stone";
    case Tile::Brick: return "brick";
    case Tile::Wood: return "wood";
    case Tile::Spike: return "spike";
    case Tile::Platform: return "platform";
    case Tile::Door: return "door";
    case Tile::DoorOpen: return "door_open";
    case Tile::Bush: return "bush";
    case Tile::Flower: return "flower";
    case Tile::Crate: return "crate";
    case Tile::Cobble: return "cobble";
    case Tile::Water: return "water";
    case Tile::Banner: return "banner";
    default: return "spark";
    }
}

bool Tilemap::loadFromRows(const std::vector<std::string>& rows) {
    if (rows.empty()) return false;
    h_ = static_cast<int>(rows.size());
    w_ = 0;
    for (auto& r : rows) w_ = std::max(w_, static_cast<int>(r.size()));
    tiles_.assign(static_cast<size_t>(w_ * h_), Tile::Empty);
    markers_.clear();
    for (int y = 0; y < h_; ++y) {
        const auto& row = rows[static_cast<size_t>(y)];
        for (int x = 0; x < static_cast<int>(row.size()); ++x) {
            char c = row[static_cast<size_t>(x)];
            if (c == 'P' || c == 'C' || c == '1' || c == '2' || c == 'N') {
                markers_.push_back({c, x, y});
                tiles_[static_cast<size_t>(y * w_ + x)] = Tile::Empty;
            } else if (c == 'D') {
                markers_.push_back({c, x, y});
                tiles_[static_cast<size_t>(y * w_ + x)] = Tile::Door;
            } else {
                tiles_[static_cast<size_t>(y * w_ + x)] = fromChar(c);
            }
        }
    }
    return true;
}

Tile Tilemap::at(int tx, int ty) const {
    if (ty < 0) return Tile::Empty;
    if (tx < 0 || tx >= w_) return Tile::Stone;
    if (ty >= h_) return Tile::Empty; // pits fall through
    return tiles_[static_cast<size_t>(ty * w_ + tx)];
}
void Tilemap::set(int tx, int ty, Tile t) {
    if (tx < 0 || ty < 0 || tx >= w_ || ty >= h_) return;
    tiles_[static_cast<size_t>(ty * w_ + tx)] = t;
}
bool Tilemap::solid(int tx, int ty) const {
    Tile t = at(tx, ty);
    return t == Tile::Cliff || t == Tile::Dirt || t == Tile::Stone || t == Tile::Brick ||
           t == Tile::Wood || t == Tile::Crate || t == Tile::Cobble || t == Tile::Platform;
}
bool Tilemap::spike(int tx, int ty) const { return at(tx, ty) == Tile::Spike; }
bool Tilemap::isDoor(int tx, int ty) const {
    Tile t = at(tx, ty);
    return t == Tile::Door || t == Tile::DoorOpen;
}

} // namespace tempest
