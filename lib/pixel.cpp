#include "pixel.hpp"

//U and V are in pixel coordinates, not normalize [0,1] coordinates
void draw_textured_triangle(Canvas & canvas, Image & tex,
    int x1, int y1, int u1, int v1,
    int x2, int y2, int u2, int v2,
    int x3, int y3, int u3, int v3)
{
    int area = cross(coord2(x1, y1) - coord2(x2, y2), coord2(x3, y3) - coord2(x2, y2));
    if (area == 0) return;

    int minx = imax(0, imin(x1, imin(x2, x3)));
    int miny = imax(0, imin(y1, imin(y2, y3)));
    int maxx = imin(canvas.width - 1, imax(x1, imax(x2, x3)));
    int maxy = imin(canvas.height - 1, imax(y1, imax(y2, y3)));
    for (int y = miny; y <= maxy; ++y) {
        for (int x = minx; x <= maxx; ++x) {
            int w1 = cross(coord2(x, y) - coord2(x2, y2), coord2(x3, y3) - coord2(x2, y2));
            int w2 = cross(coord2(x, y) - coord2(x3, y3), coord2(x1, y1) - coord2(x3, y3));
            int w3 = cross(coord2(x, y) - coord2(x1, y1), coord2(x2, y2) - coord2(x1, y1));


            if ((w1 >= 0 && w2 >= 0 && w3 >= 0) || (w1 <= 0 && w2 <= 0 && w3 <= 0)) {
                int u = w1 * u1 + w2 * u2 + w3 * u3;
                int v = w1 * v1 + w2 * v2 + w3 * v3;

                u /= area;
                v /= area;

                //TODO: option to wrap texture using euclidean modulus?
                if (u >= 0 && u < tex.width && v >= 0 && v < tex.height) {
                    unsafe_blend(canvas, x, y, tex[v][u]);
                }
            }
        }
    }
}

void add_light(Canvas & canvas, int x0, int y0, int w, int h, Color color) {
    int minx = imax(0, x0 - w);
    int miny = imax(0, y0 - h);
    int maxx = imin(canvas.width, x0 + w);
    int maxy = imin(canvas.height, y0 + h);
    if (maxx < minx || maxy < miny) return;
    float xfactor = 1.0f / w;
    float yfactor = 1.0f / h;
    for (int y = miny; y <= maxy; ++y) {
        for (int x = minx; x <= maxx; ++x) {
            float dx = (x - x0 + 0.5f) * xfactor;
            float dy = (y - y0 + 0.5f) * yfactor;
            if (dx * dx + dy * dy < 1) {
                Color c = color;
                float dist = (dx * dx + dy * dy);
                float factor = fminf(255, (1 / dist - 1) * 64);
                // c.a = (1 - sqrtf(dx * dx + dy * dy)) * 255;
                c.a = (c.a * (u8)factor) >> 8;
                unsafe_blend_add(canvas, x, y, c);
            }
        }
    }
}

void _draw_sprite_a1(Canvas & canvas, Pixel * pixels, int width, int height, int pitch, int cx, int cy) {
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

    #ifdef SSE_BLIT
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
                __m128i c = _mm_blendv_epi8(src, img, alphaMask); //@SSE4.1
                _mm_storeu_si128((__m128i *) canvSrc, c);
            }
        }
    #else
        for (int y = 0; y < srch; ++y) {
            for (int x = 0; x < srcw; ++x) {
                Color c = pixels[(srcy + y) * pitch + srcx + x];
                if (c.a > 127) {
                    canvas[miny + y][minx + x] = c;
                }
            }
        }
    #endif
}
