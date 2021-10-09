#ifndef PIXEL_HPP
#define PIXEL_HPP

//TODO: move the implementations of most of these functions into the cpp file

#include "stb_image.h"
#include "color.hpp"
#include "glutil.hpp"
#include "math.hpp"
#include "common.hpp"
#include <immintrin.h>
#include <smmintrin.h>
#include <string.h>

struct Canvas {
    Pixel * pixels;
    int width;
    int height;
    int pitch; //number of pixels, NOT number of bytes!
    int margin; //number of pixels, NOT number of bytes!

    //NOTE: indexed in [y][x] order!!!
    __attribute__((__always_inline__)) Pixel * operator[] (int row) {
        return pixels + row * pitch;
    }

    __attribute__((__always_inline__)) Pixel * basePointer() {
        return pixels - margin * pitch - margin;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SPRITE OPS                                                                                                       ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct Image {
    Pixel * pixels;
    int width;
    int height;

    //NOTE: indexed in [y][x] order!!!
    __attribute__((__always_inline__)) Pixel * operator[] (int row) {
        return pixels + row * width;
    }
};

static inline Image load_image(const char * filepath) {
    int w, h, c;
    Pixel * pixels = (Pixel *) stbi_load(filepath, &w, &h, &c, 4);
    assert(pixels);
    //add 4 pixel padding for SIMD loads off the end
    pixels = (Pixel *) realloc(pixels, w * h * sizeof(Pixel) + 4 * sizeof(Pixel));
    return { pixels, w, h };
}

static inline __attribute__((__always_inline__))
void unsafe_blend(Canvas & canvas, int x, int y, Color color) {
    Pixel * row = canvas.pixels + y * canvas.pitch;
    row[x].r = (color.r * color.a + row[x].r * (255 - color.a)) >> 8;
    row[x].g = (color.g * color.a + row[x].g * (255 - color.a)) >> 8;
    row[x].b = (color.b * color.a + row[x].b * (255 - color.a)) >> 8;
}

static inline __attribute__((__always_inline__))
void blend(Canvas canvas, int x, int y, Color color) {
    if (x >= 0 && x < canvas.width && y >= 0 && y < canvas.height) {
        Pixel * row = canvas.pixels + y * canvas.pitch;
        row[x].r = (color.r * color.a + row[x].r * (255 - color.a)) >> 8;
        row[x].g = (color.g * color.a + row[x].g * (255 - color.a)) >> 8;
        row[x].b = (color.b * color.a + row[x].b * (255 - color.a)) >> 8;
    }
}

static inline __attribute__((__always_inline__))
void unsafe_blend_add(Canvas & canvas, int x, int y, Color color) {
    Pixel * row = canvas.pixels + y * canvas.pitch;
    row[x].r = imin(255, row[x].r + ((color.r * color.a) >> 8));
    row[x].g = imin(255, row[x].g + ((color.g * color.a) >> 8));
    row[x].b = imin(255, row[x].b + ((color.b * color.a) >> 8));
}

static inline __attribute__((__always_inline__))
void blend_add(Canvas canvas, int x, int y, Color color) {
    if (x >= 0 && x < canvas.width && y >= 0 && y < canvas.height) {
        Pixel * row = canvas.pixels + y * canvas.pitch;
        row[x].r = imin(255, row[x].r + ((color.r * color.a) >> 8));
        row[x].g = imin(255, row[x].g + ((color.g * color.a) >> 8));
        row[x].b = imin(255, row[x].b + ((color.b * color.a) >> 8));
    }
}

// #define SSE_BLIT

//TODO: refactor these to only differ in the blend function

void _draw_sprite_a1(Canvas & canvas, Pixel * pixels, int width, int height, int pitch, int cx, int cy);

static inline void draw_sprite_a1(Canvas & canvas, Image & image, int cx, int cy) {
    _draw_sprite_a1(canvas, image.pixels, image.width, image.height, image.width, cx, cy);
}

static inline void _draw_sprite(Canvas & canvas, Pixel * pixels, int width, int height, int pitch, int cx, int cy) {
    //destination coords
    int minx = imax(0, cx);
    int miny = imax(0, cy);
    int maxx = imin(canvas.width, cx + width);
    int maxy = imin(canvas.height, cy + height);

    if (minx >= maxx || miny >= maxy) return;

    //source coords
    int srcx = minx - cx;
    int srcy = miny - cy;
    int srcw = maxx - minx;
    int srch = maxy - miny;

    #ifdef SSE_BLIT
        const __m128i i3210 = _mm_set_epi32(3, 2, 1, 0);
        const __m128i rbMask1 = _mm_set1_epi32(0x00FF00FF);
        const __m128i gMask1 = _mm_set1_epi32(0x0000FF00);
        const __m128i iSrcw = _mm_set1_epi32(srcw);
        const __m128i rbMask2 = _mm_set1_epi32(0xFF00FF00);

        for (int y = 0; y < srch; ++y) {
            for (int x = 0; x < srcw; x += 4) {
                //we use this to set the alpha of pixels outside the region to 0 so they will be ignored
                __m128i inclusionMask = _mm_cmplt_epi32(_mm_add_epi32(i3210, _mm_set1_epi32(x)), iSrcw);

                __m128i dst = _mm_loadu_si128((__m128i *) &canvas[miny + y][minx + x]);
                __m128i src = _mm_loadu_si128((__m128i *) &pixels[(srcy + y) * pitch + srcx + x]);

                //unpack alpha and 1-alpha into 16-bit lanes
                //TODO: can we use shuffles for this?
                __m128i a = _mm_and_si128(_mm_srli_epi32(src, 24), inclusionMask);
                __m128i a1 = _mm_or_si128(a, _mm_slli_epi32(a, 16));
                __m128i a2 = _mm_sub_epi16(_mm_set1_epi16(255), a1);

                //unpack color channels into 16-bit lanes and multiple with alpha and 1-alpha
                __m128i rb1 = _mm_mullo_epi16(_mm_and_si128(src, rbMask1), a1); //0x00FF00FF -> 0xFFFFFFFF
                __m128i rb2 = _mm_mullo_epi16(_mm_and_si128(dst, rbMask1), a2); //0x00FF00FF -> 0xFFFFFFFF
                __m128i g1 = _mm_mulhi_epu16(_mm_and_si128(src, gMask1), a1); //0x0000FF00 -> 0x000000FF
                __m128i g2 = _mm_mulhi_epu16(_mm_and_si128(dst, gMask1), a2); //0x0000FF00 -> 0x000000FF

                //add multiplied source and destination color channels
                __m128i rb = _mm_and_si128(_mm_add_epi32(rb1, rb2), rbMask2);
                __m128i g = _mm_slli_epi32(_mm_add_epi32(g1, g2), 16);

                //recombine color channels and shift into place (destination alpha can be left as 0 since it's not used)
                __m128i c = _mm_srli_epi32(_mm_or_si128(rb, g), 8);
                _mm_storeu_si128((__m128i *) &canvas[miny + y][minx + x], c);
            }
        }
    #else
        for (int y = 0; y < srch; ++y) {
            for (int x = 0; x < srcw; ++x) {
                unsafe_blend(canvas, minx + x, miny + y, pixels[(srcy + y) * pitch + srcx + x]);
            }
        }
    #endif
}

static inline void _draw_sprite_flip(Canvas & canvas, Pixel * pixels, int width, int height, int pitch, int cx, int cy, int flip) {
    //destination coords
    int minx = imax(0, cx);
    int miny = imax(0, cy);
    int maxx = imin(canvas.width, cx + width);
    int maxy = imin(canvas.height, cy + height);

    if (minx >= maxx || miny >= maxy) return;

    //source coords
    int srcx = minx - cx;
    int srcy = miny - cy;
    int srcw = maxx - minx;
    int srch = maxy - miny;

    if (flip)
        for (int y = 0; y < srch; ++y) {
            for (int x = 0; x < srcw; ++x) {
                unsafe_blend(canvas, minx + x, miny + y, pixels[(srcy + y) * pitch + srcx + srcw - x]);
            }
        }
    else
        for (int y = 0; y < srch; ++y) {
            for (int x = 0; x < srcw; ++x) {
                unsafe_blend(canvas, minx + x, miny + y, pixels[(srcy + y) * pitch + srcx + x]);
            }
        }
}

static inline void draw_sprite(Canvas & canvas, Image & image, int cx, int cy) {
    _draw_sprite(canvas, image.pixels, image.width, image.height, image.width, cx, cy);
}

static inline
void _draw_sprite(Canvas & canvas, Pixel * pixels, int width, int height, int pitch, int cx, int cy, float alpha) {
    //destination coords
    int minx = imax(0, cx);
    int miny = imax(0, cy);
    int maxx = imin(canvas.width, cx + width);
    int maxy = imin(canvas.height, cy + height);

    if (minx >= maxx || miny >= maxy) return;

    //source coords
    int srcx = minx - cx;
    int srcy = miny - cy;
    int srcw = maxx - minx;
    int srch = maxy - miny;

    #ifdef SSE_BLIT
        const __m128i alpha16 = _mm_set1_epi16(alpha * 255);
        const __m128i iSrcw = _mm_set1_epi32(srcw);
        const __m128i i3210 = _mm_set_epi32(3, 2, 1, 0);
        const __m128i allo = (__m128i) _mm_setr_epi32(0xff03ff03, 0xff03ff03, 0xff07ff07, 0x0ff7ff07);
        const __m128i alhi = (__m128i) _mm_setr_epi32(0xff0bff0b, 0xff0bff0b, 0xff0fff0f, 0x0fffff0f);
        const __m128i zero = (__m128i) _mm_setr_epi32(0x00000000, 0x00000000, 0x00000000, 0x00000000);
        const __m128i i255 = (__m128i) _mm_setr_epi32(0xff00ff00, 0xff00ff00, 0xff00ff00, 0xff00ff00);

        for (int y = 0; y < srch; ++y) {
            for (int x = 0; x < srcw; x += 4) {
                __m128i inclusionMask = _mm_cmplt_epi32(_mm_add_epi32(i3210, _mm_set1_epi32(x)), iSrcw);

                Pixel * canvSrc = &canvas[miny + y][minx + x];
                __m128i under = _mm_loadu_si128((__m128i *) canvSrc);
                Pixel * imgSrc = &pixels[(srcy + y) * pitch + srcx + x];
                __m128i over = _mm_and_si128(_mm_loadu_si128((__m128i *) imgSrc), inclusionMask);

                __m128i under0 = _mm_cvtepu8_epi16(under); //@SSE4.1
                __m128i under1 = _mm_unpackhi_epi8(under, zero);
                __m128i over0 = _mm_cvtepu8_epi16(over); //@SSE4.1
                __m128i over1 = _mm_unpackhi_epi8(over, zero);
                __m128i alpha0 = _mm_mullo_epi16(_mm_shuffle_epi8(over, allo), alpha16); //@SSSE3
                __m128i alpha1 = _mm_mullo_epi16(_mm_shuffle_epi8(over, alhi), alpha16); //@SSSE3
                __m128i invAlpha0 = _mm_sub_epi8(i255, alpha0);
                __m128i invAlpha1 = _mm_sub_epi8(i255, alpha1);
                __m128i underMul0 = _mm_mulhi_epu16(under0, invAlpha0);
                __m128i underMul1 = _mm_mulhi_epu16(under1, invAlpha1);
                __m128i overMul0 = _mm_mulhi_epu16(over0, alpha0);
                __m128i overMul1 = _mm_mulhi_epu16(over1, alpha1);
                __m128i underFinal = _mm_packus_epi16(underMul0, underMul1);
                __m128i overFinal = _mm_packus_epi16(overMul0, overMul1);
                __m128i c = _mm_adds_epu8(overFinal, underFinal);
                _mm_storeu_si128((__m128i *) canvSrc, c);
            }
        }
    #else
        for (int y = 0; y < srch; ++y) {
            for (int x = 0; x < srcw; ++x) {
                Color color = pixels[(srcy + y) * pitch + srcx + x];
                color.a *= alpha;
                unsafe_blend(canvas, minx + x, miny + y, color);
            }
        }
    #endif
}

static inline void draw_sprite(Canvas & canvas, Image & image, int cx, int cy, float alpha) {
    _draw_sprite(canvas, image.pixels, image.width, image.height, image.width, cx, cy, alpha);
}

static inline void _draw_sprite_silhouette(Canvas & canvas, Pixel * pixels,
    int width, int height, int pitch, int cx, int cy, Color fill)
{
    //destination coords
    int minx = imax(0, cx);
    int miny = imax(0, cy);
    int maxx = imin(canvas.width, cx + width);
    int maxy = imin(canvas.height, cy + height);

    //source coords
    int srcx = minx - cx;
    int srcy = miny - cy;
    int srcw = maxx - minx;
    int srch = maxy - miny;

    // #ifdef SSE_BLIT
    #if 0 //TODO: SSE version
        const __m128i iSrcw = _mm_set1_epi32(srcw);
        const __m128i i3210 = _mm_set_epi32(3, 2, 1, 0);
        const __m128i zero = (__m128i) _mm_set1_epi32(0);

        for (int y = 0; y < srch; ++y) {
            for (int x = 0; x < srcw; x += 4) {
                __m128i ix = _mm_add_epi32(i3210, _mm_set1_epi32(x));
                __m128i inclusionMask = _mm_cmplt_epi32(ix, iSrcw);

                Pixel * canvSrc = &canvas[miny + y][minx + x];
                __m128i src = _mm_loadu_si128((__m128i *) canvSrc);
                Pixel * imgSrc = &pixels[(srcy + y) * pitch + srcx + x];
                __m128i img = _mm_loadu_si128((__m128i *) imgSrc);

                __m128i alphaMask = _mm_and_si128(_mm_cmplt_epi32(img, zero), inclusionMask);
                __m128i c = _mm_blendv_epi8(src, img, alphaMask);
                _mm_storeu_si128((__m128i *) canvSrc, c);
            }
        }
    #else
        for (int y = 0; y < srch; ++y) {
            for (int x = 0; x < srcw; ++x) {
                Color c = pixels[(srcy + y) * pitch + srcx + x];
                if (c.a > 127) {
                    unsafe_blend(canvas, minx + x, miny + y, fill);
                }
            }
        }
    #endif
}

static inline void draw_sprite_silhouette(Canvas & canvas, Image & image, int cx, int cy, Color fill) {
    _draw_sprite_silhouette(canvas, image.pixels, image.width, image.height, image.width, cx, cy, fill);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// TILESET OPS                                                                                                      ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct Tileset {
    int tileWidth;
    int tileHeight;
    int width;
    int height;
    float frame_time;
    Image image;
};

static inline Tileset load_tileset(const char * filepath, int tileWidth, int tileHeight, float frame_time = 10000.0f) {
    Image image = load_image(filepath);
    return { tileWidth, tileHeight, image.width / tileWidth, image.height / tileHeight, frame_time, image };
}

static inline void draw_tile(Canvas & canvas, Tileset & set, int tx, int ty, int cx, int cy) {
    _draw_sprite(canvas,
        &set.image[ty * set.tileHeight][tx * set.tileWidth], set.tileWidth, set.tileHeight, set.image.width, cx, cy);
}

static inline void draw_tile_a1(Canvas & canvas, Tileset & set, int tx, int ty, int cx, int cy) {
    _draw_sprite_a1(canvas,
        &set.image[ty * set.tileHeight][tx * set.tileWidth], set.tileWidth, set.tileHeight, set.image.width, cx, cy);
}

static inline void draw_animation(Canvas & canvas, Tileset & set, int cx, int cy, float anim_timer) {
    int index = anim_timer / set.frame_time;
    int tx = index % set.width;
    int ty = (index / set.width) % set.height;
    draw_tile(canvas, set, tx, ty, cx, cy);
}

static inline void draw_tile_silhouette(Canvas & canvas, Tileset & set, int tx, int ty, int cx, int cy, Color fill) {
    _draw_sprite_silhouette(canvas, &set.image[ty * set.tileHeight][tx * set.tileWidth],
        set.tileWidth, set.tileHeight, set.image.width, cx, cy, fill);
}

static inline void draw_animation_silhouette(Canvas & canvas,
    Tileset & set, int cx, int cy, Color fill, float anim_timer)
{
    int index = anim_timer / set.frame_time;
    int tx = index % set.width;
    int ty = (index / set.width) % set.height;
    draw_tile_silhouette(canvas, set, tx, ty, cx, cy, fill);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SHAPE OPS                                                                                                        ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static inline void draw_rect(Canvas & canvas, int x, int y, int w, int h, Color color) {
    int minx = imax(0, x);
    int miny = imax(0, y);
    int maxx = imin(canvas.width , x + w);
    int maxy = imin(canvas.height, y + h);
    for (int y = miny; y < maxy; ++y) {
        for (int x = minx; x < maxx; ++x) {
            unsafe_blend(canvas, x, y, color);
        }
    }
}

static inline void draw_line(Canvas & canvas, int x1, int y1, int x2, int y2, Color color) {
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);

    int sx = x1 < x2? 1 : -1;
    int sy = y1 < y2? 1 : -1;

    int err = dx - dy;

    int x = x1;
    int y = y1;

    while (true) {
        blend(canvas, x, y, color);

        if (x == x2 && y == y2) {
            return;
        }

        int e2 = 2 * err;

        if (e2 > -dy) { err -= dy; x += sx; }
        if (e2 <  dx) { err += dx; y += sy; }
    }
}

static inline void draw_oval_f(Canvas & canvas, float x0, float y0, float w, float h, Color color) {
    int minx = imax(0, lroundf(x0 - w));
    int miny = imax(0, lroundf(y0 - h));
    int maxx = imin(canvas.width  - 1, lroundf(x0 + w));
    int maxy = imin(canvas.height - 1, lroundf(y0 + h));
    if (maxx < minx || maxy < miny) return;
    float xfactor = 1.0f / w;
    float yfactor = 1.0f / h;
    if (color.a < 250) {
        for (int y = miny; y <= maxy; ++y) {
            for (int x = minx; x <= maxx; ++x) {
                float dx = (x - x0 + 0.5f) * xfactor;
                float dy = (y - y0 + 0.5f) * yfactor;
                if (dx * dx + dy * dy < 1) {
                    unsafe_blend(canvas, x, y, color);
                }
            }
        }
    } else {
        for (int y = miny; y <= maxy; ++y) {
            for (int x = minx; x <= maxx; ++x) {
                float dx = (x - x0 + 0.5f) * xfactor;
                float dy = (y - y0 + 0.5f) * yfactor;
                if (dx * dx + dy * dy < 1) {
                    canvas.pixels[y * canvas.pitch + x] = color;
                }
            }
        }
    }
}

static inline void draw_oval(Canvas & canvas, int x0, int y0, int w, int h, Color color) {
    int minx = imax(0, x0 - w);
    int miny = imax(0, y0 - h);
    int maxx = imin(canvas.width  - 1, x0 + w);
    int maxy = imin(canvas.height - 1, y0 + h);
    if (maxx < minx || maxy < miny) return;
    float xfactor = 1.0f / w;
    float yfactor = 1.0f / h;
    if (color.a < 250) {
        for (int y = miny; y <= maxy; ++y) {
            for (int x = minx; x <= maxx; ++x) {
                float dx = (x - x0 + 0.5f) * xfactor;
                float dy = (y - y0 + 0.5f) * yfactor;
                if (dx * dx + dy * dy < 1) {
                    unsafe_blend(canvas, x, y, color);
                }
            }
        }
    } else {
        for (int y = miny; y <= maxy; ++y) {
            for (int x = minx; x <= maxx; ++x) {
                float dx = (x - x0 + 0.5f) * xfactor;
                float dy = (y - y0 + 0.5f) * yfactor;
                if (dx * dx + dy * dy < 1) {
                    canvas.pixels[y * canvas.pitch + x] = color;
                }
            }
        }
    }
}

static inline void draw_oval_add(Canvas & canvas, int x0, int y0, int w, int h, Color color) {
    int minx = imax(0, x0 - w);
    int miny = imax(0, y0 - h);
    int maxx = imin(canvas.width  - 1, x0 + w);
    int maxy = imin(canvas.height - 1, y0 + h);
    if (maxx < minx || maxy < miny) return;
    float xfactor = 1.0f / w;
    float yfactor = 1.0f / h;
    for (int y = miny; y <= maxy; ++y) {
        for (int x = minx; x <= maxx; ++x) {
            float dx = (x - x0 + 0.5f) * xfactor;
            float dy = (y - y0 + 0.5f) * yfactor;
            if (dx * dx + dy * dy < 1) {
                unsafe_blend_add(canvas, x, y, color);
            }
        }
    }
}

void add_light(Canvas & canvas, int x0, int y0, int w, int h, Color color);

static inline void draw_triangle(Canvas & canvas, int x1, int y1, int x2, int y2, int x3, int y3, Color c) {
    int minx = imax(0, imin(x1, imin(x2, x3)));
    int miny = imax(0, imin(y1, imin(y2, y3)));
    int maxx = imin(canvas.width  - 1, imax(x1, imax(x2, x3)));
    int maxy = imin(canvas.height - 1, imax(y1, imax(y2, y3)));
    for (int y = miny; y <= maxy; ++y) {
        for (int x = minx; x <= maxx; ++x) {
            int w0 = cross(coord2(x, y) - coord2(x1, y1), coord2(x2, y2) - coord2(x1, y1));
            int w1 = cross(coord2(x, y) - coord2(x2, y2), coord2(x3, y3) - coord2(x2, y2));
            int w2 = cross(coord2(x, y) - coord2(x3, y3), coord2(x1, y1) - coord2(x3, y3));
            if ((w0 >= 0 && w1 >= 0 && w2 >= 0) || (w0 <= 0 && w1 <= 0 && w2 <= 0)) {
                canvas[y][x] = c;
            }
        }
    }
}

void draw_textured_triangle(Canvas & canvas, Image & tex,
    int x1, int y1, int u1, int v1,
    int x2, int y2, int u2, int v2,
    int x3, int y3, int u3, int v3);

static inline void draw_transformed_sprite(Canvas & canvas, Image & sprite, Vec2 pos, float rotation, Vec2 scale) {
    float hw = sprite.width * 0.5f;
    float hh = sprite.height * 0.5f;
    Vec2 verts[4] = { vec2(-hw, -hh), vec2(hw, -hh), vec2(hw, hh), vec2(-hw, hh) };
    Mat2 mat = rotate(rotation);
    for (Vec2 & v : verts) {
        v = mat * (v * scale) + pos;
    }
    draw_textured_triangle(canvas, sprite,
        verts[0].x, verts[0].y, 0, 0,
        verts[1].x, verts[1].y, sprite.width, 0,
        verts[2].x, verts[2].y, sprite.width, sprite.height);
    draw_textured_triangle(canvas, sprite,
        verts[0].x, verts[0].y, 0, 0,
        verts[2].x, verts[2].y, sprite.width, sprite.height,
        verts[3].x, verts[3].y, 0, sprite.height);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FONT OPS                                                                                                         ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//TODO: make font rendering use tileset ops?

struct MonoFont {
    u8 * pixels;
    int textureWidth, textureHeight;
    int glyphWidth, glyphHeight;
    int rows, columns;
};

static inline MonoFont load_mono_font(const char * filepath, int rows, int columns) {
    int w, h, n;
    Pixel * pixels = (Pixel *) stbi_load(filepath, &w, &h, &n, 4);
    assert(pixels);

    //extract only the alpha channel, becaues for fonts that's all we care about
    u8 * data = (u8 *) malloc(w * h * sizeof(u8));
    for (int i = 0; i < w * h; ++i) {
        data[i] = pixels[i].a;
    }
    stbi_image_free(pixels);

    MonoFont font = {};
    font.pixels = data;
    font.textureWidth = w;
    font.textureHeight = h;
    font.glyphWidth = w / columns;
    font.glyphHeight = h / rows;
    font.rows = rows;
    font.columns = columns;

    return font;
}

static inline void draw_glyph(Canvas & canvas, MonoFont & font, int cx, int cy, Color color, int glyph) {
    int row = glyph / font.columns;
    int col = glyph % font.columns;
    int srcx = col * font.glyphWidth;
    int srcy = row * font.glyphHeight;

    //"blit" glyph to the screen
    for (int y = 0; y < font.glyphHeight; ++y) {
        for (int x = 0; x < font.glyphWidth; ++x) {
            int idx = (srcy + y) * font.textureWidth + (srcx + x);
            if (font.pixels[idx]) {
                blend(canvas, cx + x, cy + y, color);
            }
        }
    }
}

static inline void draw_text(Canvas & canvas, MonoFont & font, int cx, int cy, Color color, const char * text) {
    for (int i = 0; text[i] != '\0'; ++i) {
        draw_glyph(canvas, font, cx + i * font.glyphWidth, cy, color, text[i]);

        //draw extenders
        if (text[i] == 'g' || text[i] == 'y') {
            draw_glyph(canvas, font, cx + i * font.glyphWidth, cy + font.glyphHeight, color, 16);
        } else if (text[i] == 'j') {
            draw_glyph(canvas, font, cx + i * font.glyphWidth, cy + font.glyphHeight, color, 19);
        } else if (text[i] == 'p') {
            draw_glyph(canvas, font, cx + i * font.glyphWidth, cy + font.glyphHeight, color, 17);
        } else if (text[i] == 'q') {
            draw_glyph(canvas, font, cx + i * font.glyphWidth, cy + font.glyphHeight, color, 18);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CANVAS OPS                                                                                                       ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static inline Canvas make_canvas(int width, int height, int margin) {
    //NOTE: we allocate extra buffer margin around the explicitly used canvas area
    //      so that operations can safely read from slightly outside the used area
    //      of the canvas. this simplifies and speeds up some operations because it
    //      eliminates the need for bounds checks and explicit handling of edge cases
    int canvasBytes = (width + 2 * margin) * (height + 2 * margin) * sizeof(Pixel);
    Pixel * canvasData = (Pixel *) malloc(canvasBytes);
    Canvas canvas = {
        canvasData + margin * (width + 2 * margin) + margin,
        width, height, width + 2 * margin, margin
    };
    //fill whole canvas, including margins, with solid black
    memset(canvasData, 0, canvasBytes);
    return canvas;
}

//TODO: improve pixel blending by using an SRGB texture
static inline void draw_canvas(int shader, Canvas & canvas, float ww, float wh) {
    uint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int w = canvas.width + canvas.margin * 2, h = canvas.height + canvas.margin * 2;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB_ALPHA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, canvas.basePointer());

    //NOTE: this still doesn't actually work to get rid of scaling artifacts
    //      except at 2x scale where it kinda sorta accidentally mostly works by happenstance
    float u1 = canvas.margin / (float) w - 0.5f / ww;
    float v1 = canvas.margin / (float) h - 0.5f / wh;
    float u2 = 1 - u1 - 1.0f / ww;
    float v2 = 1 - v1 - 1.0f / wh;

    float x = fminf(1, (wh / ww) / (canvas.height / (float) canvas.width));
    float y = fminf(1, (ww / wh) / (canvas.width / (float) canvas.height));

    struct TexVert { Vec2 pos, uv; };
    TexVert verts[6] = {
        { vec2(-x,  y), vec2(u1, v1) },
        { vec2( x,  y), vec2(u2, v1) },
        { vec2( x, -y), vec2(u2, v2) },
        { vec2(-x,  y), vec2(u1, v1) },
        { vec2( x, -y), vec2(u2, v2) },
        { vec2(-x, -y), vec2(u1, v2) },
    };

    uint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_DYNAMIC_DRAW);

    //setup vertex attributes
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(TexVert), (void *) offsetof(TexVert, pos));
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(TexVert), (void *) offsetof(TexVert, uv));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    //bind texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);

    //set uniforms and draw
    glUseProgram(shader);
    int texSizeLoc = glGetUniformLocation(shader, "texSize");
    glUniform2f(texSizeLoc, w, h);
    int scaleLoc = glGetUniformLocation(shader, "scale");
    float scale = fminf(ww / canvas.width, wh / canvas.height);
    glUniform1f(scaleLoc, scale);
    glDrawArrays(GL_TRIANGLES, 0, ARR_SIZE(verts));

    //delete temp resources
    glDeleteTextures(1, &tex);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);

    gl_error("draw_canvas()");
}

#endif //PIXEL_HPP
