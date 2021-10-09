#ifndef GRAPHICS_HPP
#define GRAPHICS_HPP

struct Graphics {
    Image player;
};

static inline Graphics load_graphics() {
    Graphics g = {};
    g.player = load_image("res/player-placeholder.png");
    return g;
}

#endif//GRAPHICS_HPP
