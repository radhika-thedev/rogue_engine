#include <iostream>
#include <vector>
#include <string>
#include <conio.h>
#include "src/Types.hpp"
#include "src/SpatialGrid.hpp"
#include "src/Entities.hpp"
#include "src/Dungeon.hpp"

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

int main() {
    SpatialGrid dungeon;
    std::mt19937 rng(1337);

    Player player(Point(0, 0), 14);
    std::vector<Enemy> enemies;
    std::vector<Item> items;
    Point stairsPos;
    int currentFloor = 1;

    generateFloor(currentFloor, dungeon, player, enemies, items, stairsPos, rng);

    std::string combatLog = "Floor 1 generated. Find stairs (>) to descend.";
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

            if (player.pos == stairsPos) {
                currentFloor++;
                generateFloor(currentFloor, dungeon, player, enemies, items, stairsPos, rng);
                combatLog = "Descended to Floor " + std::to_string(currentFloor) + "!";
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