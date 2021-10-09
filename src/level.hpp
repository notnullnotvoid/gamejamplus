#ifndef VOXEL_LEVEL_HPP
#define VOXEL_LEVEL_HPP

#include "math.hpp"
#include "list.hpp"
#include "tilemap.h"

struct Player {
    Vec2 pos;
    Vec2 vel;
    Vec2 cursor; //virtual mouse cursor in player-relative space
};

static const float BULLET_INTERVAL = 1.0f; //TODO: per-enemy interval //TODO: some randomness?
static const float BULLET_VEL = 12.0f;
struct Enemy {
    Vec2 pos;
    float timer;
};

struct Bullet {
    Vec2 pos;
    Vec2 vel;
};

struct Level {
    Player player;
    Vec2 camCenter;
    List<Enemy> enemies;
    List<Bullet> bullets; //TODO: some criteria for despawning bullets
    Tilemap map;
};

static inline Level init_level() {
    Level level = {};
    level.map.LoadMap("res/testmap.json");

    level.player.pos = vec2(0, -10);
    level.enemies.add({ .pos = vec2(-10, 10) });
    level.enemies.add({ .pos = vec2(  3, 12) });
    level.enemies.add({ .pos = vec2( 11, 10) });


    return level;
}

void AddTile(int id, int x, int y);
#endif // VOXEL_LEVEL_HPP
