#include "Dungeon.hpp"
#include <queue>
#include <unordered_map>
#include <algorithm>

static void carveRoom(const Rect& room, SpatialGrid& dungeon) {
    for (int y = room.y; y < room.y + room.h; ++y) {
        for (int x = room.x; x < room.x + room.w; ++x) {
            dungeon.setTile(Point(x, y), TileType::Floor);
        }
    }
}

static void carveHorizontalCorridor(int x1, int x2, int y, SpatialGrid& dungeon) {
    int startX = std::min(x1, x2);
    int endX = std::max(x1, x2);
    for (int x = startX; x <= endX; ++x) {
        dungeon.setTile(Point(x, y), TileType::Floor);
    }
}

static void carveVerticalCorridor(int y1, int y2, int x, SpatialGrid& dungeon) {
    int startY = std::min(y1, y2);
    int endY = std::max(y1, y2);
    for (int y = startY; y <= endY; ++y) {
        dungeon.setTile(Point(x, y), TileType::Floor);
    }
}

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

bool isTileOccupiedByEnemy(const Point& p, const std::vector<Enemy>& enemies) {
    for (const auto& enemy : enemies) {
        if (enemy.alive && enemy.pos == p) {
            return true;
        }
    }
    return false;
}

Point getNextBFSStep(const Point& start, const Point& target, 
                    const SpatialGrid& dungeon, const std::vector<Enemy>& enemies) {
    if (start == target) return start;

    std::queue<Point> frontier;
    frontier.push(start);

    std::unordered_map<Point, Point, PointHash> cameFrom;
    cameFrom[start] = start;

    bool found = false;
    const Point directions[4] = { {0, -1}, {0, 1}, {-1, 0}, {1, 0} };

    while (!frontier.empty()) {
        Point current = frontier.front();
        frontier.pop();

        if (current == target) {
            found = true;
            break;
        }

        for (const auto& dir : directions) {
            Point next(current.x + dir.x, current.y + dir.y);

            if ((dungeon.isWalkable(next) || next == target) && cameFrom.find(next) == cameFrom.end()) {
                if (next == target || !isTileOccupiedByEnemy(next, enemies)) {
                    frontier.push(next);
                    cameFrom[next] = current;
                }
            }
        }
    }

    if (!found) return start;

    Point curr = target;
    while (!(cameFrom[curr] == start)) {
        curr = cameFrom[curr];
    }
    return curr;
}

void updateEnemies(std::vector<Enemy>& enemies, Player& player, 
                   const SpatialGrid& dungeon, std::string& combatLog) {
    for (auto& enemy : enemies) {
        if (!enemy.alive) continue;

        int dx = player.pos.x - enemy.pos.x;
        int dy = player.pos.y - enemy.pos.y;
        int distSq = dx * dx + dy * dy;

        if (std::abs(dx) + std::abs(dy) == 1) {
            player.takeDamage(enemy.attackPower);
            combatLog += " | Enemy #" + std::to_string(enemy.id) + " hit you for " + std::to_string(enemy.attackPower) + " DMG!";
            continue;
        }

        if (distSq <= FOV_RADIUS * FOV_RADIUS) {
            Point nextStep = getNextBFSStep(enemy.pos, player.pos, dungeon, enemies);
            if (dungeon.isWalkable(nextStep) && !(nextStep == player.pos) && !isTileOccupiedByEnemy(nextStep, enemies)) {
                enemy.pos = nextStep;
            }
        }
    }
}

void generateFloor(int floorNum, SpatialGrid& dungeon, Player& player, 
                   std::vector<Enemy>& enemies, std::vector<Item>& items, 
                   Point& stairsPos, std::mt19937& rng) {
    dungeon.clear();
    enemies.clear();
    items.clear();

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

    player.pos = Point(rooms[0].centerX(), rooms[0].centerY());
    stairsPos = Point(rooms.back().centerX(), rooms.back().centerY());
    dungeon.setTile(stairsPos, TileType::Stairs);

    for (std::size_t i = 1; i < rooms.size() - 1; ++i) {
        int enemyHp = 3 + (floorNum - 1);
        int enemyAtk = 1 + (floorNum / 2);
        enemies.emplace_back(static_cast<int>(i), Point(rooms[i].centerX(), rooms[i].centerY()), enemyHp, enemyAtk);
    }

    int itemIdCounter = 1;
    for (std::size_t i = 1; i < rooms.size(); i += 2) {
        Point itemPos(rooms[i].x + 1, rooms[i].y + 1);
        items.emplace_back(itemIdCounter++, itemPos, ItemType::HealthPotion, 4);
    }

    updateFOV(dungeon, player.pos);
}