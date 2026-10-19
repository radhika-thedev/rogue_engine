#include "SpatialGrid.hpp"

void SpatialGrid::clear() {
    grid.clear();
    explored.clear();
}

void SpatialGrid::setTile(const Point& p, TileType type) {
    if (type == TileType::Empty) {
        grid.erase(p);
    } else {
        grid[p] = type;
    }
}

TileType SpatialGrid::getTile(const Point& p) const {
    auto it = grid.find(p);
    if (it != grid.end()) {
        return it->second;
    }
    return TileType::Wall;
}

bool SpatialGrid::isWalkable(const Point& p) const {
    TileType t = getTile(p);
    return (t == TileType::Floor || t == TileType::Stairs);
}

void SpatialGrid::markExplored(const Point& p) {
    explored[p] = true;
}

bool SpatialGrid::isExplored(const Point& p) const {
    auto it = explored.find(p);
    return it != explored.end() && it->second;
}