#pragma once
#include "Entity.hpp"
#include "Tilemap.hpp"

namespace tempest {

struct MoveHit {
    bool hitX = false;
    bool hitY = false;
    bool onGround = false;
    bool hitSpike = false;
    bool hitDoor = false;
};

MoveHit moveAndCollide(Entity& a, const Tilemap& map, float dt, bool passPits = false);

} // namespace tempest
