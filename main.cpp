#include <iostream>
#include <unordered_map>
#include <vector>
#include <queue>
#include <algorithm>
#include <random>
#include <cstdint>
#include <conio.h>
#include <string>

enum class TileType {
    Empty,
    Wall,
    Floor,
    Stairs
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

struct Player {
    Point pos;
    int hp;
    int maxHp;

    Player(Point startPos, int maxHealth) 
        : pos(startPos), hp(maxHealth), maxHp(maxHealth) {}

    void takeDamage(int amount) {
        hp = std::max(0, hp - amount);
    }

    void heal(int amount) {
        hp = std::min(maxHp, hp + amount);
    }

    bool isAlive() const {
        return hp > 0;
    }
};

struct Enemy {
    int id;
    Point pos;
    int hp;
    int attackPower;
    bool alive;

    Enemy(int enemyId, Point startPos, int startHp, int atk) 
        : id(enemyId), pos(startPos), hp(startHp), attackPower(atk), alive(true) {}

    void takeDamage(int amount) {
        hp -= amount;
        if (hp <= 0) {
            alive = false;
        }
    }
};

enum class ItemType {
    HealthPotion
};

struct Item {
    int id;
    Point pos;
    ItemType type;
    int value;
    bool collected;

    Item(int itemId, Point p, ItemType t, int val)
        : id(itemId), pos(p), type(t), value(val), collected(false) {}
};

class SpatialGrid {
private:
    std::unordered_map<Point, TileType, PointHash> grid;
    std::unordered_map<Point, bool, PointHash> explored;

public:
    void clear() {
        grid.clear();
        explored.clear();
    }

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
        return (t == TileType::Floor || t == TileType::Stairs);
    }

    void markExplored(const Point& p) {
        explored[p] = true;
    }

    bool isExplored(const Point& p) const {
        auto it = explored.find(p);
        return it != explored.end() && it->second;
    }
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

Point getNextBFSStep(const Point& start, const Point& target, const SpatialGrid& dungeon, const std::vector<Enemy>& enemies) {
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

void updateEnemies(std::vector<Enemy>& enemies, Player& player, const SpatialGrid& dungeon, std::string& combatLog) {
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

void render(const SpatialGrid& dungeon, const Player& player, const std::vector<Enemy>& enemies, 
            const std::vector<Item>& items, int currentFloor, const std::string& message) {
    std::cout << "\033[2J\033[H"; 
    std::cout << "=== Roguelike Engine: Floor " << currentFloor << " ===\n";
    std::cout << "WASD: Move/Attack | Q: Quit | Legend: @ Player, E Enemy, ! Potion, > Stairs\n\n";

    for (int y = 0; y < MAP_HEIGHT; ++y) {
        for (int x = 0; x < MAP_WIDTH; ++x) {
            Point p(x, y);

            int dx = x - player.pos.x;
            int dy = y - player.pos.y;
            bool inView = (dx * dx + dy * dy <= FOV_RADIUS * FOV_RADIUS);

            if (p == player.pos) {
                std::cout << "@";
            } else {
                bool drawn = false;

                if (inView) {
                    for (const auto& enemy : enemies) {
                        if (enemy.alive && enemy.pos == p) {
                            std::cout << "E";
                            drawn = true;
                            break;
                        }
                    }
                }

                if (!drawn && inView) {
                    for (const auto& item : items) {
                        if (!item.collected && item.pos == p) {
                            std::cout << "!";
                            drawn = true;
                            break;
                        }
                    }
                }

                if (!drawn) {
                    TileType t = dungeon.getTile(p);
                    if (inView) {
                        if (t == TileType::Floor) std::cout << ".";
                        else if (t == TileType::Stairs) std::cout << ">";
                        else std::cout << "#";
                    } else if (dungeon.isExplored(p)) {
                        if (t == TileType::Floor) std::cout << ",";
                        else if (t == TileType::Stairs) std::cout << ">";
                        else std::cout << "#";
                    } else {
                        std::cout << " ";
                    }
                }
            }
        }
        std::cout << "\n";
    }

    int aliveCount = 0;
    for (const auto& e : enemies) {
        if (e.alive) aliveCount++;
    }

    std::cout << "\n-------------------------------------------------------------\n";
    std::cout << "DUNGEON LEVEL: " << currentFloor 
              << " | HP: " << player.hp << "/" << player.maxHp << " [";
    for (int i = 0; i < player.maxHp; ++i) {
        if (i < player.hp) std::cout << "=";
        else std::cout << " ";
    }
    std::cout << "] | Pos: (" << player.pos.x << ", " << player.pos.y 
              << ") | Enemies Left: " << aliveCount << "\n";
    std::cout << "LOG: " << message << "\n";
    std::cout << "-------------------------------------------------------------\n";
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

    // Set player position to room 0 center
    player.pos = Point(rooms[0].centerX(), rooms[0].centerY());

    // Place stairs in the last room
    stairsPos = Point(rooms.back().centerX(), rooms.back().centerY());
    dungeon.setTile(stairsPos, TileType::Stairs);

    // Spawn scaled enemies in intermediate rooms
    for (std::size_t i = 1; i < rooms.size() - 1; ++i) {
        int enemyHp = 3 + (floorNum - 1);
        int enemyAtk = 1 + (floorNum / 2);
        enemies.emplace_back(static_cast<int>(i), Point(rooms[i].centerX(), rooms[i].centerY()), enemyHp, enemyAtk);
    }

    // Spawn potions
    int itemIdCounter = 1;
    for (std::size_t i = 1; i < rooms.size(); i += 2) {
        Point itemPos(rooms[i].x + 1, rooms[i].y + 1);
        items.emplace_back(itemIdCounter++, itemPos, ItemType::HealthPotion, 4);
    }

    updateFOV(dungeon, player.pos);
}

int main() {
    SpatialGrid dungeon;
    std::mt19937 rng(1337);

    Player player(Point(0, 0), 14);
    std::vector<Enemy> enemies;
    std::vector<Item> items;
    Point stairsPos;
    int currentFloor = 1;

    generateFloor(currentFloor, dungeon, player, enemies, items, stairsPos, rng);

    std::string combatLog = "Entered Floor 1. Find the stairs (>) to descend.";
    render(dungeon, player, enemies, items, currentFloor, combatLog);

    bool running = true;
    while (running && player.isAlive()) {
        char key = _getch();
        Point nextPos = player.pos;

        if (key == 'w' || key == 'W') nextPos.y -= 1;
        else if (key == 's' || key == 'S') nextPos.y += 1;
        else if (key == 'a' || key == 'A') nextPos.x -= 1;
        else if (key == 'd' || key == 'D') nextPos.x += 1;
        else if (key == 'q' || key == 'Q') {
            running = false;
            std::cout << "\nEngine Session Ended.\n";
            break;
        } else {
            continue;
        }

        combatLog = "";

        // Check for attack
        Enemy* targetEnemy = nullptr;
        for (auto& enemy : enemies) {
            if (enemy.alive && enemy.pos == nextPos) {
                targetEnemy = &enemy;
                break;
            }
        }

        if (targetEnemy) {
            targetEnemy->takeDamage(1);
            if (!targetEnemy->alive) {
                combatLog = "Defeated Enemy #" + std::to_string(targetEnemy->id) + "!";
            } else {
                combatLog = "Hit Enemy #" + std::to_string(targetEnemy->id) + "! HP: " + std::to_string(targetEnemy->hp);
            }
        } else if (dungeon.isWalkable(nextPos)) {
            player.pos = nextPos;
            combatLog = "Moved.";

            // Item collection
            for (auto& item : items) {
                if (!item.collected && item.pos == player.pos) {
                    if (item.type == ItemType::HealthPotion) {
                        player.heal(item.value);
                        item.collected = true;
                        combatLog = "Used Health Potion (+ " + std::to_string(item.value) + " HP).";
                    }
                    break;
                }
            }

            // Descend stairs check
            if (player.pos == stairsPos) {
                currentFloor++;
                generateFloor(currentFloor, dungeon, player, enemies, items, stairsPos, rng);
                combatLog = "Descended to Floor " + std::to_string(currentFloor) + "! Enemies grow stronger.";
                render(dungeon, player, enemies, items, currentFloor, combatLog);
                continue;
            }
        } else {
            combatLog = "Wall blocked movement.";
        }

        updateEnemies(enemies, player, dungeon, combatLog);
        updateFOV(dungeon, player.pos);
        render(dungeon, player, enemies, items, currentFloor, combatLog);

        if (!player.isAlive()) {
            std::cout << "\n*** YOU DIED ON FLOOR " << currentFloor << "! Press Q to exit. ***\n";
            while (_getch() != 'q');
            break;
        }
    }

    return 0;
}