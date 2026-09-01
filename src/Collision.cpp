#include "Collision.hpp"
#include <cmath>

namespace tempest {
namespace {
void tilesOverlapping(const Rect& r, int& x0, int& y0, int& x1, int& y1) {
    x0 = static_cast<int>(std::floor(r.x / kTileSize));
    y0 = static_cast<int>(std::floor(r.y / kTileSize));
    x1 = static_cast<int>(std::floor((r.x + r.w - 0.01f) / kTileSize));
    y1 = static_cast<int>(std::floor((r.y + r.h - 0.01f) / kTileSize));
}
}

MoveHit moveAndCollide(Entity& a, const Tilemap& map, float dt, bool passPits) {
    MoveHit hit;
    glm::vec2 d = a.vel * dt;

    a.pos.x += d.x;
    {
        int x0, y0, x1, y1;
        tilesOverlapping(a.bounds(), x0, y0, x1, y1);
        for (int ty = y0; ty <= y1; ++ty) {
            for (int tx = x0; tx <= x1; ++tx) {
                if (!map.solid(tx, ty)) continue;
                if (map.at(tx, ty) == Tile::Platform) continue;
                Rect t{tx * kTileSize, ty * kTileSize, kTileSize, kTileSize};
                if (!a.bounds().overlaps(t)) continue;
                hit.hitX = true;
                Rect b = a.bounds();
                if (d.x > 0) a.pos.x += t.x - b.right();
                else if (d.x < 0) a.pos.x += t.right() - b.left();
            }
        }
    }

    if (!passPits) a.pos.y += d.y;
    a.onGround = false;
    if (!passPits) {
        int x0, y0, x1, y1;
        tilesOverlapping(a.bounds(), x0, y0, x1, y1);
        for (int ty = y0; ty <= y1; ++ty) {
            for (int tx = x0; tx <= x1; ++tx) {
                Tile tile = map.at(tx, ty);
                if (!map.solid(tx, ty) && tile != Tile::Spike) continue;
                Rect t{tx * kTileSize, ty * kTileSize, kTileSize, kTileSize};
                if (!a.bounds().overlaps(t)) continue;
                if (tile == Tile::Spike) { hit.hitSpike = true; continue; }
                if (tile == Tile::Platform) {
                    Rect b = a.bounds();
                    if (d.y >= 0 && (b.bottom() - d.y) <= t.y + 4.f) {
                        a.pos.y += t.y - b.bottom();
                        a.vel.y = 0;
                        a.onGround = true;
                        hit.hitY = true;
                        hit.onGround = true;
                    }
                    continue;
                }
                hit.hitY = true;
                Rect b = a.bounds();
                if (d.y > 0) {
                    a.pos.y += t.y - b.bottom();
                    a.vel.y = 0;
                    a.onGround = true;
                    hit.onGround = true;
                } else if (d.y < 0) {
                    a.pos.y += t.bottom() - b.top();
                    a.vel.y = 0;
                }
            }
        }
    }

    int x0, y0, x1, y1;
    tilesOverlapping(a.bounds(), x0, y0, x1, y1);
    for (int ty = y0; ty <= y1; ++ty)
        for (int tx = x0; tx <= x1; ++tx) {
            if (map.spike(tx, ty)) hit.hitSpike = true;
            if (map.isDoor(tx, ty)) hit.hitDoor = true;
        }
    return hit;
}

} // namespace tempest
