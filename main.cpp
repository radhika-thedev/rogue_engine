#include <iostream>
#include <unordered_map>
#include <cstdint>
#include <functional>
#include <cassert>

enum class TileType {
    Empty,
    Wall,
    Floor,
    Player,
    Enemy,
    Item
};

struct Point {
    int x;
    int y;

    Point() : x(0), y(0) {}
    Point(int x_coord, int y_coord) : x(x_coord), y(y_coord) {}

    bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }
};

struct PointHash {
    std::size_t operator()(const Point& p) const {
        std::size_t h1 = std::hash<int>{}(p.x);
        std::size_t h2 = std::hash<int>{}(p.y);
        return h1 ^ (h2 * 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
    }
};

class SpatialGrid {
private:
    std::unordered_map<Point, TileType, PointHash> grid;

public:
    void setTile(const Point& p, TileType type) {
        if (type == TileType::Empty) {
            grid.erase(p);
        } else {
            grid[p] = type;
        }
    }

    TileType getTile(const Point& p) const {
        auto it = grid.find(p);
        if (it != grid.end()) {
            return it->second;
        }
        return TileType::Empty;
    }

    bool isWalkable(const Point& p) const {
        TileType t = getTile(p);
        return (t == TileType::Floor || t == TileType::Empty);
    }

    std::size_t totalOccupiedCells() const { return grid.size(); }
    std::size_t bucketCount() const { return grid.bucket_count(); }
    float currentLoadFactor() const { return grid.load_factor(); }
};

int main() {
    std::cout << "=== Testing Phase 1: Spatial Hash Grid Engine ===\n\n";

    SpatialGrid dungeon;

    dungeon.setTile(Point(0, 0), TileType::Wall);
    dungeon.setTile(Point(0, 1), TileType::Wall);
    dungeon.setTile(Point(1, 1), TileType::Floor);

    std::cout << "Checking Point(0, 1) [Wall]: " 
              << (dungeon.isWalkable(Point(0, 1)) ? "Walkable" : "Blocked (Correct)") << "\n";
    std::cout << "Checking Point(1, 1) [Floor]: " 
              << (dungeon.isWalkable(Point(1, 1)) ? "Walkable (Correct)" : "Blocked") << "\n";

    std::cout << "\n--- Hash Table Metrics ---\n";
    std::cout << "Occupied Cells: " << dungeon.totalOccupiedCells() << "\n";
    std::cout << "Buckets:        " << dungeon.bucketCount() << "\n";
    std::cout << "Load Factor:    " << dungeon.currentLoadFactor() << "\n";

    assert(!dungeon.isWalkable(Point(0, 1)));
    assert(dungeon.isWalkable(Point(1, 1)));

    std::cout << "\n>> Phase 1 verification complete: All tests passed cleanly.\n";
    return 0;
}