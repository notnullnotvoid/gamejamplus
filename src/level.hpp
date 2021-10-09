#ifndef VOXEL_LEVEL_HPP
#define VOXEL_LEVEL_HPP

#include "math.hpp"
#include "list.hpp"

struct Player {
    Vec2 pos;
    Vec2 vel;
    Vec2 cursor; //virtual mouse cursor in player-relative space
};

struct Enemy {
    Vec2 pos;
};

struct Level {
    Player player;
    Vec2 camCenter;
    List<Enemy> enemies;
};

static inline Level init_level() {
    Level level = {};
    level.enemies.add({ .pos = vec2(-10, 10) });
    level.enemies.add({ .pos = vec2(  3, 12) });
    level.enemies.add({ .pos = vec2( 11, 10) });
    return level;
}

#endif // VOXEL_LEVEL_HPP
