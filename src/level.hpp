#ifndef VOXEL_LEVEL_HPP
#define VOXEL_LEVEL_HPP

#include "math.hpp"
#include "list.hpp"
#include "tilemap.h"

static const float SHIELD_DISTANCE = 2.0f; //how far from the player's center they hold the shield
static const float SHIELD_WIDTH = 5.0f; //how many units wide the shield is
struct Player {
    Vec2 pos;
    Vec2 vel;
    Vec2 cursor; //virtual mouse cursor in player-relative space
};

//NOTE: I'm giving everything except the shield AABB hitboxes for now to simplify the code
static inline Rect player_hitbox(Vec2 pos) {
    float w = 0.8f, h = 1.8f;
    return { pos.x - w / 2, pos.y - h / 2, w, h };
}

static const float BULLET_INTERVAL = 0.8f; //TODO: per-enemy interval //TODO: some randomness?
static const float BULLET_VEL = 16.0f;
struct Enemy {
    Vec2 pos;
    float timer;
};

static const float BULLET_RADIUS = 0.5f;
struct Bullet {
    Vec2 pos;
    Vec2 vel;
};

//TODO: collapse this with `player_hitbox()`
static inline Rect bullet_hitbox(Vec2 pos) {
    float w = BULLET_RADIUS * 2, h = BULLET_RADIUS * 2;
    return { pos.x - w / 2, pos.y - h / 2, w, h };
}

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
#endif // VOXEL_LEVEL_HPP
