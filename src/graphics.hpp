#ifndef GRAPHICS_HPP
#define GRAPHICS_HPP

//NOTE: positive y goes down in this game, defying established convention, because that makes my life easier
static const int PIXELS_PER_UNIT = 8;

struct Graphics {
    Image player;
};

static inline Graphics load_graphics() {
    Graphics g = {};
    g.player = load_image("res/player-placeholder.png");
    return g;
}

#endif//GRAPHICS_HPP
