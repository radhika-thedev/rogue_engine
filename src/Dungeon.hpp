#pragma once

#include <vector>
#include <random>
#include <string>
#include "Types.hpp"
#include "SpatialGrid.hpp"
#include "Entities.hpp"

constexpr int MAP_WIDTH = 45;
constexpr int MAP_HEIGHT = 18;
constexpr int FOV_RADIUS = 5;

void updateFOV(SpatialGrid& dungeon, const Point& playerPos);

bool isTileOccupiedByEnemy(const Point& p, const std::vector<Enemy>& enemies);

Point getNextBFSStep(const Point& start, const Point& target, 
                    const SpatialGrid& dungeon, const std::vector<Enemy>& enemies);

void updateEnemies(std::vector<Enemy>& enemies, Player& player, 
                   const SpatialGrid& dungeon, std::string& combatLog);

void generateFloor(int floorNum, SpatialGrid& dungeon, Player& player, 
                   std::vector<Enemy>& enemies, std::vector<Item>& items, 
                   Point& stairsPos, std::mt19937& rng);