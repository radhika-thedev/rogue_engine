#pragma once

#include <unordered_map>
#include "Types.hpp"

class SpatialGrid {
private:
    std::unordered_map<Point, TileType, PointHash> grid;
    std::unordered_map<Point, bool, PointHash> explored;

public:
    void clear();
    void setTile(const Point& p, TileType type);
    TileType getTile(const Point& p) const;
    bool isWalkable(const Point& p) const;
    void markExplored(const Point& p);
    bool isExplored(const Point& p) const;
    std::size_t totalOccupiedCells() const { return grid.size(); }
};