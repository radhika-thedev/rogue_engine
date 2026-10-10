#include <iostream>
#include <vector>
#include <algorithm>
#include <random>

enum class TileType {
    Wall,
    Floor
};

struct Rect {
    int x, y, w, h;
    Rect(int x_pos, int y_pos, int width, int height)
        : x(x_pos), y(y_pos), w(width), h(height) {}

    int centerX() const { return x + w / 2; }
    int centerY() const { return y + h / 2; }

    // Intersects check with a 1-tile buffer to prevent touching walls
    bool intersects(const Rect& other) const {
        return (x <= other.x + other.w && x + w >= other.x &&
                y <= other.y + other.h && y + h >= other.y);
    }
};

const int MAP_WIDTH = 45;
const int MAP_HEIGHT = 18;

void carveRoom(const Rect& room, std::vector<std::vector<TileType>>& map) {
    for (int y = room.y; y < room.y + room.h; ++y) {
        for (int x = room.x; x < room.x + room.w; ++x) {
            map[y][x] = TileType::Floor;
        }
    }
}

void carveHorizontalCorridor(int x1, int x2, int y, std::vector<std::vector<TileType>>& map) {
    int startX = std::min(x1, x2);
    int endX = std::max(x1, x2);
    for (int x = startX; x <= endX; ++x) {
        map[y][x] = TileType::Floor;
    }
}

void carveVerticalCorridor(int y1, int y2, int x, std::vector<std::vector<TileType>>& map) {
    int startY = std::min(y1, y2);
    int endY = std::max(y1, y2);
    for (int y = startY; y <= endY; ++y) {
        map[y][x] = TileType::Floor;
    }
}

int main() {
    // 1. Solid rock initialization
    std::vector<std::vector<TileType>> map(
        MAP_HEIGHT, std::vector<TileType>(MAP_WIDTH, TileType::Wall)
    );

    // 2. Setup standard C++11 pseudo-random number generator
    std::mt19937 rng(1337); // Fixed seed for reproducible generation
    std::uniform_int_distribution<int> distX(1, MAP_WIDTH - 10);
    std::uniform_int_distribution<int> distY(1, MAP_HEIGHT - 8);
    std::uniform_int_distribution<int> distSize(4, 7);

    std::vector<Rect> rooms;
    const int MAX_ATTEMPTS = 30;

    // 3. Attempt to place rooms
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
            carveRoom(newRoom, map);

            // Connect to previous room with corridor
            if (!rooms.empty()) {
                int prevX = rooms.back().centerX();
                int prevY = rooms.back().centerY();
                int newX = newRoom.centerX();
                int newY = newRoom.centerY();

                // 50% coin toss on corridor direction
                if (rng() % 2 == 0) {
                    carveHorizontalCorridor(prevX, newX, prevY, map);
                    carveVerticalCorridor(prevY, newY, newX, map);
                } else {
                    carveVerticalCorridor(prevY, newY, prevX, map);
                    carveHorizontalCorridor(prevX, newX, newY, map);
                }
            }

            rooms.push_back(newRoom);
        }
    }

    // 4. Render map
    std::cout << "Procedural Dungeon (" << rooms.size() << " rooms connected):\n\n";
    for (int y = 0; y < MAP_HEIGHT; ++y) {
        for (int x = 0; x < MAP_WIDTH; ++x) {
            std::cout << (map[y][x] == TileType::Floor ? "." : "#");
        }
        std::cout << "\n";
    }

    return 0;
}