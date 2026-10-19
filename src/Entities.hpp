#pragma once

#include <algorithm>
#include "Types.hpp"

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