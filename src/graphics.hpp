#ifndef GRAPHICS_HPP
#define GRAPHICS_HPP

//NOTE: positive y goes down in this game, defying established convention, because that makes my life easier
static const float PIXELS_PER_UNIT = 8;

struct Graphics {
    Image player;
    Image cursor;
    Image ghost;
    Tileset tileset;
};

static inline Graphics load_graphics() {
    Graphics g = {};
    g.player = load_image("res/player-placeholder.png");
    g.cursor = load_image("res/cursor.png");
    g.ghost = load_image("res/ghost.png");
    g.tileset = load_tileset("res/groundilesheet.png", 16, 16); //yes it's spelled wrong NO YOU CAN'T CHANGE IT
    return g;
}

#endif//GRAPHICS_HPP
