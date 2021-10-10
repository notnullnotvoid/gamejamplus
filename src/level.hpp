#ifndef VOXEL_LEVEL_HPP
#define VOXEL_LEVEL_HPP

#include "math.hpp"
#include "list.hpp"
#include "tilemap.h"

//NOTE: only the ratios of different masses matter, so the units are unimportant, imagine they're kilograms
static const float PLAYER_MASS = 50;
static const float BULLET_MASS = 5;

static const float SHIELD_DISTANCE = 2.0f; //how far from the player's center they hold the shield
static const float SHIELD_WIDTH = 5.0f; //how many units wide the shield is
static const float SHIELD_THICKNESS = 0.5f; //how thick the shield's hitbox is
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

static inline OBB shield_hitbox(Player & player) {
    Vec2 shieldCenter = player.pos + noz(player.cursor) * SHIELD_DISTANCE;
    Vec2 shieldHalfOff = noz(vec2(player.cursor.y, -player.cursor.x)) * SHIELD_WIDTH * 0.5f;
    return { {
        shieldCenter + shieldHalfOff - noz(player.cursor) * SHIELD_THICKNESS * 0.5f,
        shieldCenter + shieldHalfOff + noz(player.cursor) * SHIELD_THICKNESS * 0.5f,
        shieldCenter - shieldHalfOff + noz(player.cursor) * SHIELD_THICKNESS * 0.5f,
        shieldCenter - shieldHalfOff - noz(player.cursor) * SHIELD_THICKNESS * 0.5f,
    } };
}

static const float BULLET_INTERVAL = 1.0f; //TODO: per-enemy interval
static const float BULLET_INTERVAL_VARIANCE = 1.3f;
static const float BULLET_VEL_MAX = 32.0f;
static const float BULLET_VEL_MIN = 12.0f;
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

struct Tile {
    int fg, bg, collision;
}

struct Level {
    Player player;
    Vec2 camCenter;
    List<Enemy> enemies;
    List<Bullet> bullets; //TODO: some criteria for despawning bullets

};

static inline Level init_level() {
    Level level = {};

    Tilemap sections[3];
    for (int i = 0; i < ARR_SIZE(sections); ++i) {
        sections[i].LoadMap(dsprintf(nullptr, "res/section%d.json", i)); //leak
    }

    level.player.pos = vec2(20, 20);
    level.enemies.add({ .pos = vec2(-10, 10) });
    level.enemies.add({ .pos = vec2(  3, 12) });
    level.enemies.add({ .pos = vec2( 11, 10) });


    return level;
}
#endif // VOXEL_LEVEL_HPP
