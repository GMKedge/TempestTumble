#pragma once
#include "Types.hpp"
#include <string>
#include <vector>

namespace tempest {

class Tilemap {
public:
    bool loadFromRows(const std::vector<std::string>& rows);
    int width() const { return w_; }
    int height() const { return h_; }
    float pixelWidth() const { return w_ * kTileSize; }
    float pixelHeight() const { return h_ * kTileSize; }
    Tile at(int tx, int ty) const;
    void set(int tx, int ty, Tile t);
    bool solid(int tx, int ty) const;
    bool spike(int tx, int ty) const;
    bool isDoor(int tx, int ty) const;
    static Tile fromChar(char c);
    static const char* spriteName(Tile t);

    struct Marker {
        char ch = 0;
        int tx = 0, ty = 0;
    };
    const std::vector<Marker>& markers() const { return markers_; }

private:
    int w_ = 0, h_ = 0;
    std::vector<Tile> tiles_;
    std::vector<Marker> markers_;
};

} // namespace tempest
