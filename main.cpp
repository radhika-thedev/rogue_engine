#include <iostream>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <random>
#include <cstdint>
#include <conio.h>

enum class TileType {
    Empty,
    Wall,
    Floor,
    Player
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
    std::unordered_map<Point, bool, PointHash> explored;

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
        return TileType::Wall;
    }

    bool isWalkable(const Point& p) const {
        TileType t = getTile(p);
        return (t == TileType::Floor);
    }

    void markExplored(const Point& p) {
        explored[p] = true;
    }

    bool isExplored(const Point& p) const {
        auto it = explored.find(p);
        return it != explored.end() && it->second;
    }

    std::size_t totalOccupiedCells() const { return grid.size(); }
};

struct Rect {
    int x, y, w, h;
    Rect(int x_pos, int y_pos, int width, int height)
        : x(x_pos), y(y_pos), w(width), h(height) {}

    int centerX() const { return x + w / 2; }
    int centerY() const { return y + h / 2; }

    bool intersects(const Rect& other) const {
        return (x <= other.x + other.w && x + w >= other.x &&
                y <= other.y + other.h && y + h >= other.y);
    }
};

const int MAP_WIDTH = 45;
const int MAP_HEIGHT = 18;
const int FOV_RADIUS = 5;

void carveRoom(const Rect& room, SpatialGrid& dungeon) {
    for (int y = room.y; y < room.y + room.h; ++y) {
        for (int x = room.x; x < room.x + room.w; ++x) {
            dungeon.setTile(Point(x, y), TileType::Floor);
        }
    }
}

void carveHorizontalCorridor(int x1, int x2, int y, SpatialGrid& dungeon) {
    int startX = std::min(x1, x2);
    int endX = std::max(x1, x2);
    for (int x = startX; x <= endX; ++x) {
        dungeon.setTile(Point(x, y), TileType::Floor);
    }
}

void carveVerticalCorridor(int y1, int y2, int x, SpatialGrid& dungeon) {
    int startY = std::min(y1, y2);
    int endY = std::max(y1, y2);
    for (int y = startY; y <= endY; ++y) {
        dungeon.setTile(Point(x, y), TileType::Floor);
    }
}

// Compute visibility and record explored tiles
void updateFOV(SpatialGrid& dungeon, const Point& playerPos) {
    for (int dy = -FOV_RADIUS; dy <= FOV_RADIUS; ++dy) {
        for (int dx = -FOV_RADIUS; dx <= FOV_RADIUS; ++dx) {
            if (dx * dx + dy * dy <= FOV_RADIUS * FOV_RADIUS) {
                Point p(playerPos.x + dx, playerPos.y + dy);
                if (p.x >= 0 && p.x < MAP_WIDTH && p.y >= 0 && p.y < MAP_HEIGHT) {
                    dungeon.markExplored(p);
                }
            }
        }
    }
}

void renderDungeon(const SpatialGrid& dungeon, const Point& playerPos) {
    std::cout << "\033[2J\033[H"; 
    std::cout << "=== Roguelike Engine with FOV (Radius: " << FOV_RADIUS << ") ===\n";
    std::cout << "WASD to Move | Q to Quit | Pos: (" << playerPos.x << ", " << playerPos.y << ")\n\n";

    for (int y = 0; y < MAP_HEIGHT; ++y) {
        for (int x = 0; x < MAP_WIDTH; ++x) {
            Point p(x, y);

            if (p == playerPos) {
                std::cout << "@";
                continue;
            }

            int dx = x - playerPos.x;
            int dy = y - playerPos.y;
            bool inView = (dx * dx + dy * dy <= FOV_RADIUS * FOV_RADIUS);

            if (inView) {
                // In direct line of sight
                TileType t = dungeon.getTile(p);
                std::cout << (t == TileType::Floor ? "." : "#");
            } else if (dungeon.isExplored(p)) {
                // Previously seen, but currently out of sight (Fog of War)
                TileType t = dungeon.getTile(p);
                std::cout << (t == TileType::Floor ? "," : "#");
            } else {
                // Complete darkness
                std::cout << " ";
            }
        }
        std::cout << "\n";
    }
}

int main() {
    SpatialGrid dungeon;
    std::mt19937 rng(1337);
    std::uniform_int_distribution<int> distX(2, MAP_WIDTH - 10);
    std::uniform_int_distribution<int> distY(2, MAP_HEIGHT - 8);
    std::uniform_int_distribution<int> distSize(4, 7);

    std::vector<Rect> rooms;
    const int MAX_ATTEMPTS = 30;

    for (int i = 0; i < MAX_ATTEMPTS; ++i) {
        int w = distSize(rng);
        int h = distSize(rng);
        int x = distX(rng);
        int y = distY(rng);

        Rect newRoom(x, y, w, h);

        bool failed = false;
        for (const auto& other : rooms) {
            if (newRoom.intersects(other)) {
                failed = true;
                break;
            }
        }

        if (!failed) {
            carveRoom(newRoom, dungeon);

            if (!rooms.empty()) {
                int prevX = rooms.back().centerX();
                int prevY = rooms.back().centerY();
                int newX = newRoom.centerX();
                int newY = newRoom.centerY();

                if (rng() % 2 == 0) {
                    carveHorizontalCorridor(prevX, newX, prevY, dungeon);
                    carveVerticalCorridor(prevY, newY, newX, dungeon);
                } else {
                    carveVerticalCorridor(prevY, newY, prevX, dungeon);
                    carveHorizontalCorridor(prevX, newX, newY, dungeon);
                }
            }

            rooms.push_back(newRoom);
        }
    }

    Point playerPos(rooms[0].centerX(), rooms[0].centerY());
    updateFOV(dungeon, playerPos);
    renderDungeon(dungeon, playerPos);

    bool running = true;
    while (running) {
        char key = _getch();
        Point nextPos = playerPos;

        if (key == 'w' || key == 'W') nextPos.y -= 1;
        else if (key == 's' || key == 'S') nextPos.y += 1;
        else if (key == 'a' || key == 'A') nextPos.x -= 1;
        else if (key == 'd' || key == 'D') nextPos.x += 1;
        else if (key == 'q' || key == 'Q') {
            running = false;
            std::cout << "\nEngine Exited Cleanly.\n";
            break;
        }

        if (dungeon.isWalkable(nextPos)) {
            playerPos = nextPos;
            updateFOV(dungeon, playerPos);
        }

        renderDungeon(dungeon, playerPos);
    }

    return 0;
}