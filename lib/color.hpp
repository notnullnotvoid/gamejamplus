#ifndef COLOR_HPP
#define COLOR_HPP

#include "math.hpp"
#include "types.hpp"

typedef struct Pixel { u8 r, g, b, a; } Color;

static inline bool operator==(Pixel a, Pixel b) {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

static inline bool operator!=(Pixel a, Pixel b) {
    return a.r != b.r || a.g != b.g || a.b != b.b || a.a != b.a;
}

static inline Vec4 vec4(Color c) {
    float f = 1.0f / 255;
    return { c.r * f, c.g * f, c.b * f, c.a * f };
}

static inline Color color(Vec4 v) {
    float f = 255;
    return { (u8) (v.x * f), (u8) (v.y * f), (u8) (v.z * f), (u8) (v.w * f) };
}

static inline Color lerp(Color a, float f, Color b) {
    return {
        lerp(a.r, f, b.r),
        lerp(a.g, f, b.g),
        lerp(a.b, f, b.b),
        lerp(a.a, f, b.a),
    };
}

static inline float linear_to_srgb(float f) {
    if (f >= 0.0031308f) return (1.055f) * powf(f, (1.0f / 2.4f)) - 0.055f;
    return 12.92f * f;
}

static inline float srgb_to_linear(float f) {
    if (f >= 0.04045f) return powf(((f + 0.055f) / (1 + 0.055f)), 2.4f);
    return f / 12.92f;
}

//TODO: optimize this with a lookup table (should be significantly faster than powf)
static inline Vec4 srgb_to_linear(Color c) {
    Vec4 n = vec4(c.r, c.g, c.b, c.a) * (1.0f / 255);
    return vec4(srgb_to_linear(n.x), srgb_to_linear(n.y), srgb_to_linear(n.z), n.w);
}

static inline Color linear_to_srgb(Vec4 v) {
    Vec4 n = vec4(linear_to_srgb(v.x), linear_to_srgb(v.y), linear_to_srgb(v.z), v.w);
    return { (u8) lroundf(n.x * 255), (u8) lroundf(n.y * 255), (u8) lroundf(n.z * 255), (u8) lroundf(n.w * 255) };
}

static inline Color desaturate(Color c) {
    Vec4 linear = srgb_to_linear(c);
    float midpoint = (linear.x * 1.0f + linear.y * 1.5f + linear.z * 0.5f) / 3;
    Vec4 adjusted = { (linear.x + midpoint) / 2, (linear.y + midpoint) / 2, (linear.z + midpoint) / 2, linear.w };
    return linear_to_srgb(adjusted);
}

#endif //COLOR_HPP
