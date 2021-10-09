#ifndef VOXEL_LEVEL_HPP
#define VOXEL_LEVEL_HPP

#include "math.hpp"

struct Player {
    Vec2 pos;
    Vec2 vel;
};

struct Level {
    Player player;
    Vec2 camCenter;
};

#endif // VOXEL_LEVEL_HPP
