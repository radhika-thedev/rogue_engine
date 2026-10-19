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
    Floor
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
        return getTile(p) == TileType::Floor;
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

// Check if a point is occupied by any living enemy
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

            // Tile is valid if walkable and not already visited in BFS
            if ((dungeon.isWalkable(next) || next == target) && cameFrom.find(next) == cameFrom.end()) {
                // Enemies treat other living enemies as obstacles unless it's the target tile
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

        // If adjacent to the player, execute an attack
        if (std::abs(dx) + std::abs(dy) == 1) {
            player.takeDamage(enemy.attackPower);
            combatLog += " | Enemy #" + std::to_string(enemy.id) + " hit you for " + std::to_string(enemy.attackPower) + " DMG!";
            continue;
        }

        // If inside the visibility radius, pathfind toward the player
        if (distSq <= FOV_RADIUS * FOV_RADIUS) {
            Point nextStep = getNextBFSStep(enemy.pos, player.pos, dungeon, enemies);
            if (dungeon.isWalkable(nextStep) && !(nextStep == player.pos) && !isTileOccupiedByEnemy(nextStep, enemies)) {
                enemy.pos = nextStep;
            }
        }
    }
}

void render(const SpatialGrid& dungeon, const Player& player, const std::vector<Enemy>& enemies, const std::string& message) {
    std::cout << "\033[2J\033[H"; 
    std::cout << "=== Roguelike Engine: Multi-Entity Manager ===\n";
    std::cout << "WASD: Move/Attack | Q: Quit\n\n";

    for (int y = 0; y < MAP_HEIGHT; ++y) {
        for (int x = 0; x < MAP_WIDTH; ++x) {
            Point p(x, y);

            int dx = x - player.pos.x;
            int dy = y - player.pos.y;
            bool inView = (dx * dx + dy * dy <= FOV_RADIUS * FOV_RADIUS);

            if (p == player.pos) {
                std::cout << "@";
            } else {
                // Check if any visible enemy occupies this tile
                bool enemyRendered = false;
                if (inView) {
                    for (const auto& enemy : enemies) {
                        if (enemy.alive && enemy.pos == p) {
                            std::cout << "E";
                            enemyRendered = true;
                            break;
                        }
                    }
                }

                if (!enemyRendered) {
                    if (inView) {
                        TileType t = dungeon.getTile(p);
                        std::cout << (t == TileType::Floor ? "." : "#");
                    } else if (dungeon.isExplored(p)) {
                        TileType t = dungeon.getTile(p);
                        std::cout << (t == TileType::Floor ? "," : "#");
                    } else {
                        std::cout << " ";
                    }
                }
            }
        }
        std::cout << "\n";
    }

    // Persistent HUD pane
    int aliveCount = 0;
    for (const auto& e : enemies) {
        if (e.alive) aliveCount++;
    }

    std::cout << "\n-------------------------------------------------------------\n";
    std::cout << "HP: " << player.hp << "/" << player.maxHp << " [";
    for (int i = 0; i < player.maxHp; ++i) {
        if (i < player.hp) std::cout << "=";
        else std::cout << " ";
    }
    std::cout << "] | Pos: (" << player.pos.x << ", " << player.pos.y << ") | Enemies Left: " << aliveCount << "\n";
    std::cout << "LOG: " << message << "\n";
    std::cout << "-------------------------------------------------------------\n";
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

    Player player(Point(rooms[0].centerX(), rooms[0].centerY()), 12);

    // Multi-Entity Spawner: Spawn one enemy in every room except the player's spawn room
    std::vector<Enemy> enemies;
    for (std::size_t i = 1; i < rooms.size(); ++i) {
        enemies.emplace_back(static_cast<int>(i), Point(rooms[i].centerX(), rooms[i].centerY()), 3, 2);
    }

    updateFOV(dungeon, player.pos);
    std::string combatLog = "Dungeon populated with " + std::to_string(enemies.size()) + " hostile entities.";
    render(dungeon, player, enemies, combatLog);

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

        // Player Phase: Check if bumping into any living enemy
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
                combatLog = "You defeated Enemy #" + std::to_string(targetEnemy->id) + "!";
            } else {
                combatLog = "Hit Enemy #" + std::to_string(targetEnemy->id) + "! HP left: " + std::to_string(targetEnemy->hp);
            }
        } else if (dungeon.isWalkable(nextPos)) {
            player.pos = nextPos;
            combatLog = "Moved.";
        } else {
            combatLog = "Wall blocked movement.";
        }

        // Enemy Phase: Update all active enemies
        updateEnemies(enemies, player, dungeon, combatLog);
        updateFOV(dungeon, player.pos);
        render(dungeon, player, enemies, combatLog);

        if (!player.isAlive()) {
            std::cout << "\n*** YOU DIED! Press Q to exit. ***\n";
            while (_getch() != 'q');
            break;
        }
    }

    return 0;
}
