#ifndef VOXEL_LEVEL_HPP
#define VOXEL_LEVEL_HPP

#include "math.hpp"
#include "list.hpp"
#include "tilemap.h"
#include "trace.hpp"

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
    bool dead;
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
static const float BULLET_VEL_MAX = 40.0f;
static const float BULLET_VEL_MIN = 16.0f;
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
    int layer[3];
};

struct TileGrid {
    Tile * tiles;
    int width, height;

    //NOTE: indexed in [y][x] order!!!
    inline Tile * operator[] (int row) {
        return tiles + row * width;
    }
};

static const float WALKER_ATTACK_TIME = 0.3f;
static const float WALKER_HOME_RADIUS = 12.0f;
static const float WALKER_AGRO_RANGE = 25.0f;
static const float WALKER_ATTACK_RANGE = 5.0f;
static const float WALKER_WALK_SPEED = 10.0f;
static const float WALK_ANIM_SPEED = 4.0f; //frames per second
struct Walker {
    Vec2 home;
    Vec2 pos;
    float walkTimer; //drives the walk animation, counts up as long as the walker is moving
    float attackTimer; //counts down from `WALKER_ATTACK_TIME` to 0, overrides walking action
    bool facingRight;
};

struct Level {
    Player player;
    Vec2 camCenter;
    List<Enemy> enemies;
    List<Walker> walkers;
    List<Bullet> bullets;
    TileGrid tiles;
    Vec2 playerStartPos;
};

static inline bool collide_with_tiles(TileGrid & tiles, Rect hitbox) {
    int minx = imax(0, floorf(hitbox.x / UNITS_PER_TILE));
    int miny = imax(0, floorf(hitbox.y / UNITS_PER_TILE));
    int maxx = imin(tiles.width , ceilf((hitbox.x + hitbox.w) / UNITS_PER_TILE));
    int maxy = imin(tiles.height, ceilf((hitbox.y + hitbox.h) / UNITS_PER_TILE));
    for (int y = miny; y < maxy; ++y) {
        for (int x = minx; x < maxx; ++x) {
            if (tiles[y][x].layer[2] >= 0) {
                return true;
            }
        }
    }
    return false;
}

static inline void draw_tile_grid(Canvas & canvas, TileGrid & tiles, Tileset & tileset, int offx, int offy) { TimeFunc
    int minx = imax(0, floorf(offx / PIXELS_PER_TILE));
    int miny = imax(0, floorf(offy / PIXELS_PER_TILE));
    int maxx = imin(tiles.width , ceilf((offx + canvas.width ) / PIXELS_PER_TILE));
    int maxy = imin(tiles.height, ceilf((offy + canvas.height) / PIXELS_PER_TILE));
    for (int y = miny; y < maxy; ++y) {
        for (int x = minx; x < maxx; ++x) {
            for (int i = 0; i < 3; ++i) {
                if (tiles[y][x].layer[i] >= 0) {
                    int tx = tiles[y][x].layer[i] % tileset.width;
                    int ty = tiles[y][x].layer[i] / tileset.width;
                    draw_tile_a1(canvas, tileset, tx, ty, x * PIXELS_PER_TILE - offx, y * PIXELS_PER_TILE - offy);
                }
            }
        }
    }
}

static inline Level init_level() {
    Level level = {};

    //load Tiled section prefabs
    Tilemap sections[10];
    for (int i = 0; i < ARR_SIZE(sections); ++i) {
        sections[i].LoadMap(dsprintf(nullptr, "res/section%d.json", i)); //leak
        assert(sections[i].mapLayers.size() == 4);
        assert(sections[i].mapLayers[0].height == 50);
        assert(sections[i].mapLayers[1].height == 50);
        assert(sections[i].mapLayers[2].height == 50);
        assert(sections[i].mapLayers[3].height == 50);
    }

    //copy section0 data to the tile grid
    int width = 100000, height = 50, xstart = 0, sectionCount = 0;
    level.tiles = { (Tile *) malloc(width * height * sizeof(Tile)), width, height };
    while (xstart < width - 500) { //magic number here should be >= largest section width
        int sectionIdx = rand_int(1, ARR_SIZE(sections));
        if (xstart == 0) sectionIdx = 0;
        Tilemap & section = sections[sectionIdx];
        for (int y = 0; y < 50; ++y) {
            for (int x = 0; x < section.mapLayers[0].width; ++x) {
                level.tiles[y][x + xstart] = { {
                    section.mapLayers[0].data[section.mapLayers[0].width * y + x] - 1,
                    section.mapLayers[1].data[section.mapLayers[1].width * y + x] - 1,
                    section.mapLayers[2].data[section.mapLayers[2].width * y + x] - 1,
                } };
                static const int ghostIdx = 2;
                static const int walkerIdx = 6;
                int enemyIdx = section.mapLayers[3].data[section.mapLayers[3].width * y + x] - 1;
                if (enemyIdx == ghostIdx) {
                    level.enemies.add({ .pos = vec2(x + xstart + rand_float(), y + 0.5f) * UNITS_PER_TILE,
                                        .timer = rand_float(BULLET_INTERVAL / BULLET_INTERVAL_VARIANCE) });
                } else if (enemyIdx == walkerIdx) {
                    Vec2 pos = vec2(x + xstart + rand_float(), y) * UNITS_PER_TILE;
                    level.walkers.add({ .home = pos, .pos = pos });
                }
            }
        }
        xstart += section.mapLayers[0].width;
        ++sectionCount;
    }
    printf("sectionCount: %d\n", sectionCount);

    //spawn debug/test setup
    // level.playerStartPos = level.player.pos = vec2(20, 20);
    level.playerStartPos = level.player.pos = vec2(80, 60);
    // level.enemies.add({ .pos = vec2(-10 + 20, 10 + 30) });
    // level.enemies.add({ .pos = vec2(  3 + 20, 12 + 30) });
    // level.enemies.add({ .pos = vec2( 11 + 20, 10 + 30) });
    level.camCenter = level.player.pos;

    return level;
}
#endif // VOXEL_LEVEL_HPP
