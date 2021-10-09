#ifndef MATH_HPP
#define MATH_HPP

#include <math.h>

//constants
static const float PI = 3.1415926535;
static const float TWO_PI = PI * 2;
static const float HALF_PI = PI / 2;
static const float QUARTER_PI = PI / 4;
static const float TAU = TWO_PI;

static inline float to_degrees(float radians) { return radians * (1.0f / PI * 180.0f); }
static inline float to_radians(float degrees) { return degrees * (1.0f / 180.0f * PI); }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// MISCELLANEOUS                                                                                                    ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//NOTE: (supposedly) works with any divisor != 0
static inline int floor_div(int a, int b) {
    int rem = a % b;
    int div = a / b;
    if (!rem) a = b;
    int sub = a ^ b;
    sub >>= 31;
    return div + sub;
}

//NOTE: only works if the divisor is > 0
static inline int floor_div2(int n, int d) {
    return (n / d) + ((n % d) >> 31);
}

//NOTE: (supposedly) works with any divisor != 0
static inline int floor_mod(int a, int b) {
    int m = a % b;
    if (m < 0) { m = (b < 0) ? m - b : m + b; }
    return m;
}

//NOTE: only works if the divisor is > 0
static inline int floor_mod2(int a, int b) {
    int m = a % b;
    if (m < 0) { m += b; }
    return m;
}

static inline float euclidean_mod(float x, float y) { return x - floorf(x / y) * y; };

static inline int iabs(int i) { return i >= 0? i : -i; }
static inline int imax(int a, int b) { return a > b? a : b; }
static inline int imin(int a, int b) { return a > b? b : a; }
static inline float len2(float x, float y) { return       x * x + y * y ; }
static inline float len (float x, float y) { return sqrtf(x * x + y * y); }

static inline float clamp(float min, float f, float max) {
    return f < min? min : f > max? max : f;
}

template <typename TYPE>
static inline TYPE sq(TYPE t) {
    return t * t;
}

template <typename TYPE>
static inline TYPE cube(TYPE t) {
    return t * t * t;
}

static inline float dist(float x1, float y1, float x2, float y2) {
    float dx = x2 - x1, dy = y2 - y1;
    return sqrtf(dx * dx + dy * dy);
}

static inline float lerp(float a, float f, float b) {
    return b * f + a * (1 - f);
}

static inline float inv_lerp(float a, float f, float b) {
    return (f - a) / (b - a);
}

template <typename TYPE>
static inline TYPE lerp(TYPE a, float f, TYPE b) {
    return b * f + a * (1 - f);
}

static inline float signed_sqrt(float f) {
    return copysignf(sqrtf(fabsf(f)), f);
}

static inline int popcount32(unsigned int i) {
     i = i - ((i >> 1) & 0x55555555);
     i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
     return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// VECTOR AND MATRIX TYPES                                                                                          ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//column vectors
struct Vec2 { float x, y      ; };
struct Vec3 { float x, y, z   ; };
struct Vec4 { float x, y, z, w; };

//row-major matrices
struct Mat2 {
    float   m00, m01,
            m10, m11;
};

struct Mat3 {
    float   m00, m01, m02,
            m10, m11, m12,
            m20, m21, m22;
};

struct Mat4 {
    float   m00, m01, m02, m03,
            m10, m11, m12, m13,
            m20, m21, m22, m23,
            m30, m31, m32, m33;
};

static inline Mat3 transpose(Mat3 m) {
    return { m.m00, m.m10, m.m20,
             m.m01, m.m11, m.m21,
             m.m02, m.m12, m.m22, };
}

static inline Mat4 transpose(Mat4 m) {
  return { m.m00, m.m10, m.m20, m.m30,
           m.m01, m.m11, m.m21, m.m31,
           m.m02, m.m12, m.m22, m.m32,
           m.m03, m.m13, m.m23, m.m33, };
}

static const Mat2 IDENTITY_2 = {
    1, 0,
    0, 1,
};

static const Mat3 IDENTITY_3 = {
    1, 0, 0,
    0, 1, 0,
    0, 0, 1,
};

static const Mat4 IDENTITY_4 = {
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1,
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// QUATERNIONS                                                                                                      ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct Quat { float w, x, y, z; };

static inline Quat quat(float w, float x, float y, float z) { return { w, x, y, z }; }

static inline Quat nor(Quat q) {
    float f = 1 / sqrtf(q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z);
    return { q.w * f, q.x * f, q.y * f, q.z * f };
}

static inline Quat lerp(Quat l, float f, Quat r) {
    return nor(quat(lerp(l.w, f, r.w), lerp(l.x, f, r.x), lerp(l.y, f, r.y), lerp(l.z, f, r.z)));
}

static inline float dot(Quat l, Quat r) { return l.w * r.w + l.x * r.x + l.y * r.y + l.z * r.z; }

static inline Quat operator-(Quat q) { return { -q.w, -q.x, -q.y, -q.z }; }

static inline Quat short_lerp(Quat l, float f, Quat r) {
    if (dot(l, r) < 0) {
        return lerp(l, f, -r);
    }
    return lerp(l, f, r);
}

//TODO: define quaternion NOZ?
// static inline Vec3 noz(Vec3 v) { return v.x == 0 && v.y == 0 && v.z == 0? vec3(0, 0, 0) : nor(v); }

//NOTE: implementation stolen from HandmadeMath.h:HMM_QuaternionToMat4()
static inline Mat3 mat3(Quat q) {
    Quat n = nor(q);

    float xx = n.x * n.x;
    float yy = n.y * n.y;
    float zz = n.z * n.z;
    float xy = n.x * n.y;
    float xz = n.x * n.z;
    float yz = n.y * n.z;
    float wx = n.w * n.x;
    float wy = n.w * n.y;
    float wz = n.w * n.z;

    return { 1.0f - 2.0f * (yy + zz),        2.0f * (xy - wz),        2.0f * (xz + wy),
                    2.0f * (xy + wz), 1.0f - 2.0f * (xx + zz),        2.0f * (yz - wx),
                    2.0f * (xz - wy),        2.0f * (yz + wx), 1.0f - 2.0f * (xx + yy) };
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// RECTS                                                                                                            ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct Rect {
    float x, y, w, h;
};

static inline Rect rect(float x, float y, float w, float h) {
    return { x, y, w, h };
}

static inline Rect operator+(Rect a, Rect b) {
    return { a.x + b.x, a.y + b.y, a.w + b.w, a.h + b.h };
}

static inline bool contains(Rect r, Vec2 v) {
    return v.x > r.x && v.y > r.y && v.x < r.x + r.w && v.y < r.y + r.h;
}

static inline bool intersects(Rect a, Rect b) {
    return a.x < b.x + b.w && b.x < a.x + a.w && a.y < b.y + b.h && b.y < a.y + a.h;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// COORDS                                                                                                           ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct Coord2 { int x, y      ; };
struct Coord3 { int x, y, z   ; };
struct Coord4 { int x, y, z, w; };

//homogeneous constructors
static inline Coord2 coord2() { return {}; };
static inline Coord3 coord3() { return {}; };
static inline Coord4 coord4() { return {}; };
static inline Coord2 coord2(int i) { return { i, i }; };
static inline Coord3 coord3(int i) { return { i, i, i }; };
static inline Coord4 coord4(int i) { return { i, i, i, i }; };

//rote constructors
static inline Coord2 coord2(int x, int y              ) { return { x, y       }; };
static inline Coord3 coord3(int x, int y, int z       ) { return { x, y, z    }; };
static inline Coord4 coord4(int x, int y, int z, int w) { return { x, y, z, w }; };

//downcast constructors
static inline Coord2 coord2(Coord3 c) { return { c.x, c.y      }; }
static inline Coord2 coord2(Coord4 c) { return { c.x, c.y      }; }
static inline Coord3 coord3(Coord4 c) { return { c.x, c.y, c.z }; }

//upcast constructors
static inline Coord3 coord3(Coord2 c, int z       ) { return { c.x, c.y,   z    }; }
static inline Coord4 coord4(Coord2 c, int z, int w) { return { c.x, c.y,   z, w }; }
static inline Coord4 coord4(Coord3 c,        int w) { return { c.x, c.y, c.z, w }; }

//cast constructors
static inline Vec2 vec2(Coord2 c) { return { (float) c.x, (float) c.y                           }; }
static inline Vec3 vec3(Coord3 c) { return { (float) c.x, (float) c.y, (float) c.z              }; }
static inline Vec4 vec4(Coord4 c) { return { (float) c.x, (float) c.y, (float) c.z, (float) c.w }; }

static inline Coord2 coord2(Vec2 v) { return { lroundf(v.x), lroundf(v.y)                             }; }
static inline Coord3 coord2(Vec3 v) { return { lroundf(v.x), lroundf(v.y), lroundf(v.z)               }; }
static inline Coord4 coord2(Vec4 v) { return { lroundf(v.x), lroundf(v.y), lroundf(v.z), lroundf(v.w) }; }

//comparators
static inline bool operator==(Coord2 l, Coord2 r) { return l.x == r.x && l.y == r.y                            ; }
static inline bool operator==(Coord3 l, Coord3 r) { return l.x == r.x && l.y == r.y && l.z == r.z              ; }
static inline bool operator==(Coord4 l, Coord4 r) { return l.x == r.x && l.y == r.y && l.z == r.z && l.w == r.w; }
static inline bool operator!=(Coord2 l, Coord2 r) { return l.x != r.x || l.y != r.y                            ; }
static inline bool operator!=(Coord3 l, Coord3 r) { return l.x != r.x || l.y != r.y || l.z != r.z              ; }
static inline bool operator!=(Coord4 l, Coord4 r) { return l.x != r.x || l.y != r.y || l.z != r.z || l.w != r.w; }

//arithmetic operators
static inline Coord2 operator-(Coord2 c) { return { -c.x, -c.y             }; }
static inline Coord3 operator-(Coord3 c) { return { -c.x, -c.y, -c.z       }; }
static inline Coord4 operator-(Coord4 c) { return { -c.x, -c.y, -c.z, -c.w }; }
static inline Coord2 operator+(Coord2 l, Coord2 r) { return { l.x + r.x, l.y + r.y                       }; }
static inline Coord3 operator+(Coord3 l, Coord3 r) { return { l.x + r.x, l.y + r.y, l.z + r.z            }; }
static inline Coord4 operator+(Coord4 l, Coord4 r) { return { l.x + r.x, l.y + r.y, l.z + r.z, l.w + r.w }; }
static inline Coord2 operator-(Coord2 l, Coord2 r) { return { l.x - r.x, l.y - r.y                       }; }
static inline Coord3 operator-(Coord3 l, Coord3 r) { return { l.x - r.x, l.y - r.y, l.z - r.z            }; }
static inline Coord4 operator-(Coord4 l, Coord4 r) { return { l.x - r.x, l.y - r.y, l.z - r.z, l.w - r.w }; }
static inline Coord2 operator*(int    i, Coord2 c) { return { i * c.x, i * c.y                   }; }
static inline Coord2 operator*(Coord2 c, int    i) { return { i * c.x, i * c.y                   }; }
static inline Coord3 operator*(int    i, Coord3 c) { return { i * c.x, i * c.y, i * c.z          }; }
static inline Coord3 operator*(Coord3 c, int    i) { return { i * c.x, i * c.y, i * c.z          }; }
static inline Coord4 operator*(int    i, Coord4 c) { return { i * c.x, i * c.y, i * c.z, i * c.w }; }
static inline Coord4 operator*(Coord4 c, int    i) { return { i * c.x, i * c.y, i * c.z, i * c.w }; }
static inline Coord2 operator*(Coord2 l, Coord2 r) { return { l.x * r.x, l.y * r.y                       }; }
static inline Coord3 operator*(Coord3 l, Coord3 r) { return { l.x * r.x, l.y * r.y, l.z * r.z            }; }
static inline Coord4 operator*(Coord4 l, Coord4 r) { return { l.x * r.x, l.y * r.y, l.z * r.z, l.w * r.w }; }
static inline Coord2 operator/(Coord2 c, int    i) { return { c.x / i, c.y / i                   }; }
static inline Coord3 operator/(Coord3 c, int    i) { return { c.x / i, c.y / i, c.z / i          }; }
static inline Coord4 operator/(Coord4 c, int    i) { return { c.x / i, c.y / i, c.z / i, c.w / i }; }
static inline Coord2 operator/(Coord2 l, Coord2 r) { return { l.x / r.x, l.y / r.y                       }; }
static inline Coord3 operator/(Coord3 l, Coord3 r) { return { l.x / r.x, l.y / r.y, l.z / r.z            }; }
static inline Coord4 operator/(Coord4 l, Coord4 r) { return { l.x / r.x, l.y / r.y, l.z / r.z, l.x / r.w }; }

static inline Coord2 & operator+=(Coord2 & l, Coord2 r) { return l = l + r; }
static inline Coord3 & operator+=(Coord3 & l, Coord3 r) { return l = l + r; }
static inline Coord4 & operator+=(Coord4 & l, Coord4 r) { return l = l + r; }
static inline Coord2 & operator-=(Coord2 & l, Coord2 r) { return l = l - r; }
static inline Coord3 & operator-=(Coord3 & l, Coord3 r) { return l = l - r; }
static inline Coord4 & operator-=(Coord4 & l, Coord4 r) { return l = l - r; }
static inline Coord2 & operator*=(Coord2 & c, int    i) { return c = c * i; }
static inline Coord3 & operator*=(Coord3 & c, int    i) { return c = c * i; }
static inline Coord4 & operator*=(Coord4 & c, int    i) { return c = c * i; }
static inline Coord2 & operator*=(Coord2 & l, Coord2 r) { return l = l * r; }
static inline Coord3 & operator*=(Coord3 & l, Coord3 r) { return l = l * r; }
static inline Coord4 & operator*=(Coord4 & l, Coord4 r) { return l = l * r; }
static inline Coord2 & operator/=(Coord2 & c, int    i) { return c = c / i; }
static inline Coord3 & operator/=(Coord3 & c, int    i) { return c = c / i; }
static inline Coord4 & operator/=(Coord4 & c, int    i) { return c = c / i; }
static inline Coord2 & operator/=(Coord2 & l, Coord2 r) { return l = l / r; }
static inline Coord3 & operator/=(Coord3 & l, Coord3 r) { return l = l / r; }
static inline Coord4 & operator/=(Coord4 & l, Coord4 r) { return l = l / r; }

static inline int dot(Coord2 l, Coord2 r) { return l.x * r.x + l.y * r.y; }
static inline int dot(Coord3 l, Coord3 r) { return l.x * r.x + l.y * r.y + l.z * r.z; }

static inline int cross(Coord2 l, Coord2 r) { return l.x * r.y - l.y * r.x; }
static inline Coord3 cross(Coord3 a, Coord3 b) {
    return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// INTEGER MATRICES                                                                                                 ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//TODO: organize all this integer matrix stuf !!!!!!

struct Mat3i {
    int m00, m01, m02,
        m10, m11, m12,
        m20, m21, m22;
};

static const Mat3i IDENTITY_3I = {
    1, 0, 0,
    0, 1, 0,
    0, 0, 1,
};

static inline Mat3i operator*(Mat3i a, Mat3i b) {
    return { a.m00 * b.m00 + a.m01 * b.m10 + a.m02 * b.m20,
             a.m00 * b.m01 + a.m01 * b.m11 + a.m02 * b.m21,
             a.m00 * b.m02 + a.m01 * b.m12 + a.m02 * b.m22,
             a.m10 * b.m00 + a.m11 * b.m10 + a.m12 * b.m20,
             a.m10 * b.m01 + a.m11 * b.m11 + a.m12 * b.m21,
             a.m10 * b.m02 + a.m11 * b.m12 + a.m12 * b.m22,
             a.m20 * b.m00 + a.m21 * b.m10 + a.m22 * b.m20,
             a.m20 * b.m01 + a.m21 * b.m11 + a.m22 * b.m21,
             a.m20 * b.m02 + a.m21 * b.m12 + a.m22 * b.m22, };
}

static inline Coord3 operator*(Mat3i m, Coord3 v) {
    return { m.m00 * v.x + m.m01 * v.y + m.m02 * v.z,
             m.m10 * v.x + m.m11 * v.y + m.m12 * v.z,
             m.m20 * v.x + m.m21 * v.y + m.m22 * v.z, };
}

static inline Mat3i translate(Mat3i m, Coord2 v) {
    m.m02 += v.x;
    m.m12 += v.y;
    return m;
}

struct Mat4i {
    int m00, m01, m02, m03,
        m10, m11, m12, m13,
        m20, m21, m22, m23,
        m30, m31, m32, m33;
};

static const Mat4i IDENTITY_4I = {
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1,
};

static inline Mat4i operator*(Mat4i a, Mat4i b) {
    return { a.m00 * b.m00 + a.m01 * b.m10 + a.m02 * b.m20 + a.m03 * b.m30,
             a.m00 * b.m01 + a.m01 * b.m11 + a.m02 * b.m21 + a.m03 * b.m31,
             a.m00 * b.m02 + a.m01 * b.m12 + a.m02 * b.m22 + a.m03 * b.m32,
             a.m00 * b.m03 + a.m01 * b.m13 + a.m02 * b.m23 + a.m03 * b.m33,
             a.m10 * b.m00 + a.m11 * b.m10 + a.m12 * b.m20 + a.m13 * b.m30,
             a.m10 * b.m01 + a.m11 * b.m11 + a.m12 * b.m21 + a.m13 * b.m31,
             a.m10 * b.m02 + a.m11 * b.m12 + a.m12 * b.m22 + a.m13 * b.m32,
             a.m10 * b.m03 + a.m11 * b.m13 + a.m12 * b.m23 + a.m13 * b.m33,
             a.m20 * b.m00 + a.m21 * b.m10 + a.m22 * b.m20 + a.m23 * b.m30,
             a.m20 * b.m01 + a.m21 * b.m11 + a.m22 * b.m21 + a.m23 * b.m31,
             a.m20 * b.m02 + a.m21 * b.m12 + a.m22 * b.m22 + a.m23 * b.m32,
             a.m20 * b.m03 + a.m21 * b.m13 + a.m22 * b.m23 + a.m23 * b.m33,
             a.m30 * b.m00 + a.m31 * b.m10 + a.m32 * b.m20 + a.m33 * b.m30,
             a.m30 * b.m01 + a.m31 * b.m11 + a.m32 * b.m21 + a.m33 * b.m31,
             a.m30 * b.m02 + a.m31 * b.m12 + a.m32 * b.m22 + a.m33 * b.m32,
             a.m30 * b.m03 + a.m31 * b.m13 + a.m32 * b.m23 + a.m33 * b.m33, };
}

static inline Coord4 operator*(Mat4i m, Coord4 v) {
    return { m.m00 * v.x + m.m01 * v.y + m.m02 * v.z + m.m03 * v.w,
             m.m10 * v.x + m.m11 * v.y + m.m12 * v.z + m.m13 * v.w,
             m.m20 * v.x + m.m21 * v.y + m.m22 * v.z + m.m23 * v.w,
             m.m30 * v.x + m.m31 * v.y + m.m32 * v.z + m.m33 * v.w, };
}

static inline Mat4i translate(Mat4i m, Coord3 v) {
    m.m03 += v.x;
    m.m13 += v.y;
    m.m23 += v.z;
    return m;
}

static inline Mat4i mat4i(Mat3i m) {
    return { m.m00, m.m01, m.m02, 0,
             m.m10, m.m11, m.m12, 0,
             m.m20, m.m21, m.m22, 0,
                 0,     0,     0, 1 };
}

static inline Mat4 mat4(Mat4i m) {
    return { (float) m.m00, (float) m.m01, (float) m.m02, (float) m.m03,
             (float) m.m10, (float) m.m11, (float) m.m12, (float) m.m13,
             (float) m.m20, (float) m.m21, (float) m.m22, (float) m.m23,
             (float) m.m30, (float) m.m31, (float) m.m32, (float) m.m33 };
}

static inline Mat3i mat3i(Mat4i m) {
    return { m.m00, m.m01, m.m02,
             m.m10, m.m11, m.m12,
             m.m20, m.m21, m.m22 };
}

static inline Mat3 mat3(Mat3i m) {
    return { (float) m.m00, (float) m.m01, (float) m.m02,
             (float) m.m10, (float) m.m11, (float) m.m12,
             (float) m.m20, (float) m.m21, (float) m.m22 };
}

static inline Mat3i transpose(Mat3i m) {
    return { m.m00, m.m10, m.m20,
             m.m01, m.m11, m.m21,
             m.m02, m.m12, m.m22, };
}

//PRECONDITION: `axis` must be normalized
//NOTE: copied from wikipedia like a professional
static inline Mat3i rotate(Coord3 axis, int angle) {
    int c = iabs(floor_mod2((angle + 3), 4) - 1) - 1;
    int s = iabs(floor_mod2((angle + 2), 4) - 1) - 1;
    int x = axis.x, y = axis.y, z = axis.z;
    return {
        c + x * x * (1 - c), x * y * (1 - c) - z * s, x * z * (1 - c) + y * s,
        y * x * (1 - c) + z * s, c + y * y * (1 - c), y * z * (1 - c) - x * s,
        z * x * (1 - c) - y * s, z * y * (1 - c) + x * s, c + z * z * (1 - c),
    };
}

static inline bool operator==(Mat3i a, Mat3i b) {
    return a.m00 == b.m00 && a.m01 == b.m01 && a.m02 == b.m02 &&
           a.m10 == b.m10 && a.m11 == b.m11 && a.m12 == b.m12 &&
           a.m20 == b.m20 && a.m21 == b.m21 && a.m22 == b.m22;
}

//TODO: should these simply reference the operator==?
static inline bool operator!=(Mat3i a, Mat3i b) {
    return a.m00 != b.m00 || a.m01 != b.m01 || a.m02 != b.m02 ||
           a.m10 != b.m10 || a.m11 != b.m11 || a.m12 != b.m12 ||
           a.m20 != b.m20 || a.m21 != b.m21 || a.m22 != b.m22;
}

static inline bool operator==(Mat4i a, Mat4i b) {
    return a.m00 == b.m00 && a.m01 == b.m01 && a.m02 == b.m02 && a.m03 == b.m03 &&
           a.m10 == b.m10 && a.m11 == b.m11 && a.m12 == b.m12 && a.m13 == b.m13 &&
           a.m20 == b.m20 && a.m21 == b.m21 && a.m22 == b.m22 && a.m23 == b.m23 &&
           a.m30 == b.m30 && a.m31 == b.m31 && a.m32 == b.m32 && a.m33 == b.m33;
}

static inline bool operator!=(Mat4i a, Mat4i b) {
    return a.m00 != b.m00 || a.m01 != b.m01 || a.m02 != b.m02 || a.m03 != b.m03 ||
           a.m10 != b.m10 || a.m11 != b.m11 || a.m12 != b.m12 || a.m13 != b.m13 ||
           a.m20 != b.m20 || a.m21 != b.m21 || a.m22 != b.m22 || a.m23 != b.m23 ||
           a.m30 != b.m30 || a.m31 != b.m31 || a.m32 != b.m32 || a.m33 != b.m33;
}

//from https://stackoverflow.com/a/44446912/3064745
static inline Mat4i inverse(Mat4i m) {
    int a2323 = m.m22 * m.m33 - m.m23 * m.m32;
    int a1323 = m.m21 * m.m33 - m.m23 * m.m31;
    int a1223 = m.m21 * m.m32 - m.m22 * m.m31;
    int a0323 = m.m20 * m.m33 - m.m23 * m.m30;
    int a0223 = m.m20 * m.m32 - m.m22 * m.m30;
    int a0123 = m.m20 * m.m31 - m.m21 * m.m30;
    int a2313 = m.m12 * m.m33 - m.m13 * m.m32;
    int a1313 = m.m11 * m.m33 - m.m13 * m.m31;
    int a1213 = m.m11 * m.m32 - m.m12 * m.m31;
    int a2312 = m.m12 * m.m23 - m.m13 * m.m22;
    int a1312 = m.m11 * m.m23 - m.m13 * m.m21;
    int a1212 = m.m11 * m.m22 - m.m12 * m.m21;
    int a0313 = m.m10 * m.m33 - m.m13 * m.m30;
    int a0213 = m.m10 * m.m32 - m.m12 * m.m30;
    int a0312 = m.m10 * m.m23 - m.m13 * m.m20;
    int a0212 = m.m10 * m.m22 - m.m12 * m.m20;
    int a0113 = m.m10 * m.m31 - m.m11 * m.m30;
    int a0112 = m.m10 * m.m21 - m.m11 * m.m20;

    int det = + m.m00 * ( m.m11 * a2323 - m.m12 * a1323 + m.m13 * a1223)
              - m.m01 * ( m.m10 * a2323 - m.m12 * a0323 + m.m13 * a0223)
              + m.m02 * ( m.m10 * a1323 - m.m11 * a0323 + m.m13 * a0123)
              - m.m03 * ( m.m10 * a1223 - m.m11 * a0223 + m.m12 * a0123);

    return { + (m.m11 * a2323 - m.m12 * a1323 + m.m13 * a1223) / det,
             - (m.m01 * a2323 - m.m02 * a1323 + m.m03 * a1223) / det,
             + (m.m01 * a2313 - m.m02 * a1313 + m.m03 * a1213) / det,
             - (m.m01 * a2312 - m.m02 * a1312 + m.m03 * a1212) / det,
             - (m.m10 * a2323 - m.m12 * a0323 + m.m13 * a0223) / det,
             + (m.m00 * a2323 - m.m02 * a0323 + m.m03 * a0223) / det,
             - (m.m00 * a2313 - m.m02 * a0313 + m.m03 * a0213) / det,
             + (m.m00 * a2312 - m.m02 * a0312 + m.m03 * a0212) / det,
             + (m.m10 * a1323 - m.m11 * a0323 + m.m13 * a0123) / det,
             - (m.m00 * a1323 - m.m01 * a0323 + m.m03 * a0123) / det,
             + (m.m00 * a1313 - m.m01 * a0313 + m.m03 * a0113) / det,
             - (m.m00 * a1312 - m.m01 * a0312 + m.m03 * a0112) / det,
             - (m.m10 * a1223 - m.m11 * a0223 + m.m12 * a0123) / det,
             + (m.m00 * a1223 - m.m01 * a0223 + m.m02 * a0123) / det,
             - (m.m00 * a1213 - m.m01 * a0213 + m.m02 * a0113) / det,
             + (m.m00 * a1212 - m.m01 * a0212 + m.m02 * a0112) / det, };
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// MATRIX AND VECTOR BOILERPLATE                                                                                    ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//homogeneous constructor

static inline Vec2 vec2() { return {}; }
static inline Vec3 vec3() { return {}; }
static inline Vec4 vec4() { return {}; }
static inline Vec2 vec2(float f) { return { f, f       }; }
static inline Vec3 vec3(float f) { return { f, f, f    }; }
static inline Vec4 vec4(float f) { return { f, f, f, f }; }

//rote constructors

static inline Vec2 vec2(float x, float y                  ) { return { x, y       }; }
static inline Vec3 vec3(float x, float y, float z         ) { return { x, y, z    }; }
static inline Vec4 vec4(float x, float y, float z, float w) { return { x, y, z, w }; }

//TODO: not sure if this should be transposed relative to how it is now
static inline Mat3 mat3(Vec3 x, Vec3 y, Vec3 z) {
    return { x.x, x.y, x.z,
             y.x, y.y, y.z,
             z.x, z.y, z.z, };
}

//downcast constructors

static inline Vec2 vec2(Vec3 v) { return { v.x, v.y      }; }
static inline Vec2 vec2(Vec4 v) { return { v.x, v.y      }; }
static inline Vec3 vec3(Vec4 v) { return { v.x, v.y, v.z }; }

static inline Mat2 mat2(Mat3 m) {
    return { m.m00, m.m01,
             m.m10, m.m11, };
}

static inline Mat2 mat2(Mat4 m) {
    return { m.m00, m.m01,
             m.m10, m.m11, };
}

static inline Mat3 mat3(Mat4 m) {
    return { m.m00, m.m01, m.m02,
             m.m10, m.m11, m.m12,
             m.m20, m.m21, m.m22, };
}

//upcast constructors

static inline Vec3 vec3(Vec2 v, float z         ) { return { v.x, v.y,   z    }; }
static inline Vec4 vec4(Vec2 v, float z, float w) { return { v.x, v.y,   z, w }; }
static inline Vec4 vec4(Vec3 v,          float w) { return { v.x, v.y, v.z, w }; }

static inline Mat3 mat3(Mat2 m) {
    return { m.m00, m.m01, 0,
             m.m10, m.m11, 0,
                 0,     0, 1 };
}

static inline Mat4 mat4(Mat2 m) {
    return { m.m00, m.m01, 0, 0,
             m.m10, m.m11, 0, 0,
                 0,     0, 1, 0,
                 0,     0, 0, 1 };
}

static inline Mat4 mat4(Mat3 m) {
    return { m.m00, m.m01, m.m02, 0,
             m.m10, m.m11, m.m12, 0,
             m.m20, m.m21, m.m22, 0,
                 0,     0,     0, 1 };
}

//comparators

static inline bool operator==(Vec2 l, Vec2 r) { return l.x == r.x && l.y == r.y                            ; }
static inline bool operator==(Vec3 l, Vec3 r) { return l.x == r.x && l.y == r.y && l.z == r.z              ; }
static inline bool operator==(Vec4 l, Vec4 r) { return l.x == r.x && l.y == r.y && l.z == r.z && l.w == r.w; }
static inline bool operator!=(Vec2 l, Vec2 r) { return l.x != r.x || l.y != r.y                            ; }
static inline bool operator!=(Vec3 l, Vec3 r) { return l.x != r.x || l.y != r.y || l.z != r.z              ; }
static inline bool operator!=(Vec4 l, Vec4 r) { return l.x != r.x || l.y != r.y || l.z != r.z || l.w != r.w; }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// VECTOR OPERATIONRS                                                                                               ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static inline Vec2 nor(Vec2 v) {
    float f = 1 / sqrtf(v.x * v.x + v.y * v.y);
    return { v.x * f, v.y * f };
}

static inline Vec3 nor(Vec3 v) {
    float f = 1 / sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    return { v.x * f, v.y * f, v.z * f };
}

static inline Vec2 setlen(Vec2 v, float len) {
    float f = len / sqrtf(v.x * v.x + v.y * v.y);
    return { v.x * f, v.y * f };
}

static inline Vec3 setlen(Vec3 v, float len) {
    float f = len / sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    return { v.x * f, v.y * f, v.z * f };
}

//TODO: can I only do sqrt after the check? is this a perf matter?
static inline Vec2 noz(Vec2 v) {
    float denom = sqrtf(v.x * v.x + v.y * v.y);
    if (denom == 0) return {};
    float f = 1 / denom;
    return { v.x * f, v.y * f };
}

static inline Vec3 noz(Vec3 v) {
    float denom = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    if (denom == 0) return {};
    float f = 1 / denom;
    return { v.x * f, v.y * f, v.z * f };
}

static inline Vec2 operator-(Vec2 v) { return { -v.x, -v.y             }; }
static inline Vec3 operator-(Vec3 v) { return { -v.x, -v.y, -v.z       }; }
static inline Vec4 operator-(Vec4 v) { return { -v.x, -v.y, -v.z, -v.w }; }
static inline Vec2 operator+(Vec2 l, Vec2 r) { return { l.x + r.x, l.y + r.y                       }; }
static inline Vec3 operator+(Vec3 l, Vec3 r) { return { l.x + r.x, l.y + r.y, l.z + r.z            }; }
static inline Vec4 operator+(Vec4 l, Vec4 r) { return { l.x + r.x, l.y + r.y, l.z + r.z, l.w + r.w }; }
static inline Vec2 operator-(Vec2 l, Vec2 r) { return { l.x - r.x, l.y - r.y                       }; }
static inline Vec3 operator-(Vec3 l, Vec3 r) { return { l.x - r.x, l.y - r.y, l.z - r.z            }; }
static inline Vec4 operator-(Vec4 l, Vec4 r) { return { l.x - r.x, l.y - r.y, l.z - r.z, l.w - r.w }; }
static inline Vec2 operator*(float f, Vec2  v) { return { f * v.x, f * v.y                   }; }
static inline Vec2 operator*(Vec2  v, float f) { return { f * v.x, f * v.y                   }; }
static inline Vec3 operator*(float f, Vec3  v) { return { f * v.x, f * v.y, f * v.z          }; }
static inline Vec3 operator*(Vec3  v, float f) { return { f * v.x, f * v.y, f * v.z          }; }
static inline Vec4 operator*(float f, Vec4  v) { return { f * v.x, f * v.y, f * v.z, f * v.w }; }
static inline Vec4 operator*(Vec4  v, float f) { return { f * v.x, f * v.y, f * v.z, f * v.w }; }
static inline Vec2 operator*(Vec2  l, Vec2  r) { return { l.x * r.x, l.y * r.y                       }; }
static inline Vec3 operator*(Vec3  l, Vec3  r) { return { l.x * r.x, l.y * r.y, l.z * r.z            }; }
static inline Vec4 operator*(Vec4  l, Vec4  r) { return { l.x * r.x, l.y * r.y, l.z * r.z, l.w * r.w }; }
static inline Vec2 operator/(Vec2  v, float f) { return { v.x / f, v.y / f                   }; }
static inline Vec3 operator/(Vec3  v, float f) { return { v.x / f, v.y / f, v.z / f          }; }
static inline Vec4 operator/(Vec4  v, float f) { return { v.x / f, v.y / f, v.z / f, v.w / f }; }
static inline Vec2 operator/(Vec2  l, Vec2  r) { return { l.x / r.x, l.y / r.y                       }; }
static inline Vec3 operator/(Vec3  l, Vec3  r) { return { l.x / r.x, l.y / r.y, l.z / r.z            }; }
static inline Vec4 operator/(Vec4  l, Vec4  r) { return { l.x / r.x, l.y / r.y, l.z / r.z, l.w / r.w }; }

static inline Vec2 & operator+=(Vec2 & l, Vec2  r) { return l = l + r; }
static inline Vec3 & operator+=(Vec3 & l, Vec3  r) { return l = l + r; }
static inline Vec4 & operator+=(Vec4 & l, Vec4  r) { return l = l + r; }
static inline Vec2 & operator-=(Vec2 & l, Vec2  r) { return l = l - r; }
static inline Vec3 & operator-=(Vec3 & l, Vec3  r) { return l = l - r; }
static inline Vec4 & operator-=(Vec4 & l, Vec4  r) { return l = l - r; }
static inline Vec2 & operator*=(Vec2 & v, float f) { return v = v * f; }
static inline Vec3 & operator*=(Vec3 & v, float f) { return v = v * f; }
static inline Vec4 & operator*=(Vec4 & v, float f) { return v = v * f; }
static inline Vec2 & operator*=(Vec2 & l, Vec2  r) { return l = l * r; }
static inline Vec3 & operator*=(Vec3 & l, Vec3  r) { return l = l * r; }
static inline Vec4 & operator*=(Vec4 & l, Vec4  r) { return l = l * r; }
static inline Vec2 & operator/=(Vec2 & v, float f) { return v = v / f; }
static inline Vec3 & operator/=(Vec3 & v, float f) { return v = v / f; }
static inline Vec4 & operator/=(Vec4 & v, float f) { return v = v / f; }
static inline Vec2 & operator/=(Vec2 & l, Vec2  r) { return l = l / r; }
static inline Vec3 & operator/=(Vec3 & l, Vec3  r) { return l = l / r; }
static inline Vec4 & operator/=(Vec4 & l, Vec4  r) { return l = l / r; }

static inline float dot(Vec2 l, Vec2 r) { return l.x * r.x + l.y * r.y; }
static inline float dot(Vec3 l, Vec3 r) { return l.x * r.x + l.y * r.y + l.z * r.z; }

//NOTE: l -> r clockwise = negative
static inline float cross(Vec2 l, Vec2 r) { return l.x * r.y - l.y * r.x; }
static inline Vec3  cross(Vec3 a, Vec3 b) { return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x }; }

static inline float len2(Vec2 v) { return       v.x * v.x + v.y * v.y             ; }
static inline float len (Vec2 v) { return sqrtf(v.x * v.x + v.y * v.y            ); }
static inline float len2(Vec3 v) { return       v.x * v.x + v.y * v.y + v.z * v.z ; }
static inline float len (Vec3 v) { return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z); }

static inline float angle_diff(Vec2 from, Vec2 to) { return atan2f(cross(from, to), dot(from, to)); }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// MATRIX OPERATIONS                                                                                                ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static inline Mat2 inverse(Mat2 m) {
    float det = 1 / (m.m00 * m.m11 - m.m01 * m.m10);

    return {
        + m.m11 * det,
        - m.m01 * det,
        - m.m10 * det,
        + m.m00 * det,
    };
}

static inline Mat3 inverse(Mat3 m) {
    float det = 1 / (+ m.m00 * (m.m11 * m.m22 - m.m12 * m.m21)
                     - m.m01 * (m.m10 * m.m22 - m.m12 * m.m20)
                     + m.m02 * (m.m10 * m.m21 - m.m11 * m.m20));

    return { + (m.m11 * m.m22 - m.m21 * m.m12) * det,
             - (m.m01 * m.m22 - m.m21 * m.m02) * det,
             + (m.m01 * m.m12 - m.m11 * m.m02) * det,
             - (m.m10 * m.m22 - m.m20 * m.m12) * det,
             + (m.m00 * m.m22 - m.m20 * m.m02) * det,
             - (m.m00 * m.m12 - m.m10 * m.m02) * det,
             + (m.m10 * m.m21 - m.m20 * m.m11) * det,
             - (m.m00 * m.m21 - m.m20 * m.m01) * det,
             + (m.m00 * m.m11 - m.m10 * m.m01) * det, };
}

//from https://stackoverflow.com/a/44446912/3064745
static inline Mat4 inverse(Mat4 m) {
    float a2323 = m.m22 * m.m33 - m.m23 * m.m32;
    float a1323 = m.m21 * m.m33 - m.m23 * m.m31;
    float a1223 = m.m21 * m.m32 - m.m22 * m.m31;
    float a0323 = m.m20 * m.m33 - m.m23 * m.m30;
    float a0223 = m.m20 * m.m32 - m.m22 * m.m30;
    float a0123 = m.m20 * m.m31 - m.m21 * m.m30;
    float a2313 = m.m12 * m.m33 - m.m13 * m.m32;
    float a1313 = m.m11 * m.m33 - m.m13 * m.m31;
    float a1213 = m.m11 * m.m32 - m.m12 * m.m31;
    float a2312 = m.m12 * m.m23 - m.m13 * m.m22;
    float a1312 = m.m11 * m.m23 - m.m13 * m.m21;
    float a1212 = m.m11 * m.m22 - m.m12 * m.m21;
    float a0313 = m.m10 * m.m33 - m.m13 * m.m30;
    float a0213 = m.m10 * m.m32 - m.m12 * m.m30;
    float a0312 = m.m10 * m.m23 - m.m13 * m.m20;
    float a0212 = m.m10 * m.m22 - m.m12 * m.m20;
    float a0113 = m.m10 * m.m31 - m.m11 * m.m30;
    float a0112 = m.m10 * m.m21 - m.m11 * m.m20;

    float det = 1 / (+ m.m00 * ( m.m11 * a2323 - m.m12 * a1323 + m.m13 * a1223)
                     - m.m01 * ( m.m10 * a2323 - m.m12 * a0323 + m.m13 * a0223)
                     + m.m02 * ( m.m10 * a1323 - m.m11 * a0323 + m.m13 * a0123)
                     - m.m03 * ( m.m10 * a1223 - m.m11 * a0223 + m.m12 * a0123));

    return { + (m.m11 * a2323 - m.m12 * a1323 + m.m13 * a1223) * det,
             - (m.m01 * a2323 - m.m02 * a1323 + m.m03 * a1223) * det,
             + (m.m01 * a2313 - m.m02 * a1313 + m.m03 * a1213) * det,
             - (m.m01 * a2312 - m.m02 * a1312 + m.m03 * a1212) * det,
             - (m.m10 * a2323 - m.m12 * a0323 + m.m13 * a0223) * det,
             + (m.m00 * a2323 - m.m02 * a0323 + m.m03 * a0223) * det,
             - (m.m00 * a2313 - m.m02 * a0313 + m.m03 * a0213) * det,
             + (m.m00 * a2312 - m.m02 * a0312 + m.m03 * a0212) * det,
             + (m.m10 * a1323 - m.m11 * a0323 + m.m13 * a0123) * det,
             - (m.m00 * a1323 - m.m01 * a0323 + m.m03 * a0123) * det,
             + (m.m00 * a1313 - m.m01 * a0313 + m.m03 * a0113) * det,
             - (m.m00 * a1312 - m.m01 * a0312 + m.m03 * a0112) * det,
             - (m.m10 * a1223 - m.m11 * a0223 + m.m12 * a0123) * det,
             + (m.m00 * a1223 - m.m01 * a0223 + m.m02 * a0123) * det,
             - (m.m00 * a1213 - m.m01 * a0213 + m.m02 * a0113) * det,
             + (m.m00 * a1212 - m.m01 * a0212 + m.m02 * a0112) * det, };
}

static inline Mat2 operator*(Mat2 a, Mat2 b) {
    return { a.m00 * b.m00 + a.m01 * b.m10,
             a.m00 * b.m01 + a.m01 * b.m11,
             a.m10 * b.m00 + a.m11 * b.m10,
             a.m10 * b.m01 + a.m11 * b.m11, };
}

static inline Mat3 operator*(Mat3 a, Mat3 b) {
    return { a.m00 * b.m00 + a.m01 * b.m10 + a.m02 * b.m20,
             a.m00 * b.m01 + a.m01 * b.m11 + a.m02 * b.m21,
             a.m00 * b.m02 + a.m01 * b.m12 + a.m02 * b.m22,
             a.m10 * b.m00 + a.m11 * b.m10 + a.m12 * b.m20,
             a.m10 * b.m01 + a.m11 * b.m11 + a.m12 * b.m21,
             a.m10 * b.m02 + a.m11 * b.m12 + a.m12 * b.m22,
             a.m20 * b.m00 + a.m21 * b.m10 + a.m22 * b.m20,
             a.m20 * b.m01 + a.m21 * b.m11 + a.m22 * b.m21,
             a.m20 * b.m02 + a.m21 * b.m12 + a.m22 * b.m22, };
}

//NOTE: marking this function inline has proven to help code gen on clang
static inline Mat4 operator*(Mat4 a, Mat4 b) {
    return { a.m00 * b.m00 + a.m01 * b.m10 + a.m02 * b.m20 + a.m03 * b.m30,
             a.m00 * b.m01 + a.m01 * b.m11 + a.m02 * b.m21 + a.m03 * b.m31,
             a.m00 * b.m02 + a.m01 * b.m12 + a.m02 * b.m22 + a.m03 * b.m32,
             a.m00 * b.m03 + a.m01 * b.m13 + a.m02 * b.m23 + a.m03 * b.m33,
             a.m10 * b.m00 + a.m11 * b.m10 + a.m12 * b.m20 + a.m13 * b.m30,
             a.m10 * b.m01 + a.m11 * b.m11 + a.m12 * b.m21 + a.m13 * b.m31,
             a.m10 * b.m02 + a.m11 * b.m12 + a.m12 * b.m22 + a.m13 * b.m32,
             a.m10 * b.m03 + a.m11 * b.m13 + a.m12 * b.m23 + a.m13 * b.m33,
             a.m20 * b.m00 + a.m21 * b.m10 + a.m22 * b.m20 + a.m23 * b.m30,
             a.m20 * b.m01 + a.m21 * b.m11 + a.m22 * b.m21 + a.m23 * b.m31,
             a.m20 * b.m02 + a.m21 * b.m12 + a.m22 * b.m22 + a.m23 * b.m32,
             a.m20 * b.m03 + a.m21 * b.m13 + a.m22 * b.m23 + a.m23 * b.m33,
             a.m30 * b.m00 + a.m31 * b.m10 + a.m32 * b.m20 + a.m33 * b.m30,
             a.m30 * b.m01 + a.m31 * b.m11 + a.m32 * b.m21 + a.m33 * b.m31,
             a.m30 * b.m02 + a.m31 * b.m12 + a.m32 * b.m22 + a.m33 * b.m32,
             a.m30 * b.m03 + a.m31 * b.m13 + a.m32 * b.m23 + a.m33 * b.m33, };
}

static inline Vec2 operator*(Mat2 m, Vec2 v) {
    return { m.m00 * v.x + m.m01 * v.y,
             m.m10 * v.x + m.m11 * v.y, };
}

static inline Vec3 operator*(Mat3 m, Vec3 v) {
    return { m.m00 * v.x + m.m01 * v.y + m.m02 * v.z,
             m.m10 * v.x + m.m11 * v.y + m.m12 * v.z,
             m.m20 * v.x + m.m21 * v.y + m.m22 * v.z, };
}

static inline Vec4 operator*(Mat4 m, Vec4 v) {
    return { m.m00 * v.x + m.m01 * v.y + m.m02 * v.z + m.m03 * v.w,
             m.m10 * v.x + m.m11 * v.y + m.m12 * v.z + m.m13 * v.w,
             m.m20 * v.x + m.m21 * v.y + m.m22 * v.z + m.m23 * v.w,
             m.m30 * v.x + m.m31 * v.y + m.m32 * v.z + m.m33 * v.w, };
}

static inline Mat2 scale(Mat2 m, float s) {
    m.m00 *= s; m.m01 *= s;
    m.m10 *= s; m.m11 *= s;
    return m;
}

static inline Mat2 scale(Mat2 m, float x, float y) {
    m.m00 *= x; m.m01 *= x;
    m.m10 *= y; m.m11 *= y;
    return m;
}

static inline Mat3 scale(Mat3 m, float s) {
    m.m00 *= s; m.m01 *= s; m.m02 *= s;
    m.m10 *= s; m.m11 *= s; m.m12 *= s;
    return m;
}

static inline Mat3 scale(Mat3 m, float x, float y) {
    m.m00 *= x; m.m01 *= x; m.m02 *= x;
    m.m10 *= y; m.m11 *= y; m.m12 *= y;
    return m;
}

static inline Mat4 scale(Mat4 m, float s) {
    m.m00 *= s; m.m01 *= s; m.m02 *= s; m.m03 *= s;
    m.m10 *= s; m.m11 *= s; m.m12 *= s; m.m13 *= s;
    m.m20 *= s; m.m21 *= s; m.m22 *= s; m.m23 *= s;
    return m;
}

static inline Mat4 scale(Mat4 m, float x, float y, float z) {
    m.m00 *= x; m.m01 *= x; m.m02 *= x; m.m03 *= x;
    m.m10 *= y; m.m11 *= y; m.m12 *= y; m.m13 *= y;
    m.m20 *= z; m.m21 *= z; m.m22 *= z; m.m23 *= z;
    return m;
}

static inline Mat3 translate(Mat3 m, Vec2 v) {
    m.m02 += v.x;
    m.m12 += v.y;
    return m;
}

static inline Mat4 translate(Mat4 m, Vec3 v) {
    m.m03 += v.x;
    m.m13 += v.y;
    m.m23 += v.z;
    return m;
}

//TODO: functions for rotating vectors directly?

static inline Mat2 rotate(float angle) {
    float c = cosf(angle);
    float s = sinf(angle);
    return { c, -s,
             s,  c, };
}

static inline Mat3 rotateX(float angle) {
    float c = cosf(angle);
    float s = sinf(angle);
    return { 1, 0,  0,
             0, c, -s,
             0, s,  c, };
}

static inline Mat3 rotateY(float angle) {
    float c = cosf(angle);
    float s = sinf(angle);
    return { c, 0, s,
             0, 1, 0,
            -s, 0, c, };
}

static inline Mat3 rotateZ(float angle) {
    float c = cosf(angle);
    float s = sinf(angle);
    return { c, -s, 0,
             s,  c, 0,
             0,  0, 1, };
}

//PRECONDITION: `axis` must be normalized
//NOTE: copied from wikipedia like a professional
static inline Mat3 rotate(Vec3 axis, float angle) {
    float c = cosf(angle);
    float s = sinf(angle);
    float x = axis.x, y = axis.y, z = axis.z;
    return {
        c + x * x * (1 - c), x * y * (1 - c) - z * s, x * z * (1 - c) + y * s,
        y * x * (1 - c) + z * s, c + y * y * (1 - c), y * z * (1 - c) - x * s,
        z * x * (1 - c) - y * s, z * y * (1 - c) + x * s, c + z * z * (1 - c),
    };
}

//logic shamelessly copied from GLM's implementation because I don't understand the math myself
static inline Mat4 look_at(Vec3 eye, Vec3 target, Vec3 up) {
    Vec3 f = nor(target - eye);
    Vec3 s = nor(cross(f, up));
    Vec3 u = cross(s, f);

    Mat4 ret = IDENTITY_4;
    ret.m00 =  s.x;
    ret.m01 =  s.y;
    ret.m02 =  s.z;
    ret.m10 =  u.x;
    ret.m11 =  u.y;
    ret.m12 =  u.z;
    ret.m20 = -f.x;
    ret.m21 = -f.y;
    ret.m22 = -f.z;
    ret.m03 = -dot(s, eye);
    ret.m13 = -dot(u, eye);
    ret.m23 =  dot(f, eye);
    return ret;
}

//logic shamelessly copied from GLM's implementation because I don't understand the math myself
static inline Mat4 perspective(float fovy, float aspect, float nearPlane, float farPlane) {
    float tanHalfFovy = tanf(fovy * 0.5f);

    Mat4 m = {};
    m.m00 = 1 / (aspect * tanHalfFovy);
    m.m11 = 1 / tanHalfFovy;
    m.m22 = -(farPlane + nearPlane) / (farPlane - nearPlane);
    m.m32 = -1;
    m.m23 = -2 * farPlane * nearPlane / (farPlane - nearPlane);
    return m;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ORIENTED BOUNDING BOXES                                                                                          ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct OBB { Vec2 p[4]; }; //wound clockwise

static inline bool contains(OBB o, Vec2 p) {
    auto a = cross(o.p[1] - o.p[0], p - o.p[0]);
    auto b = cross(o.p[2] - o.p[1], p - o.p[1]);
    auto c = cross(o.p[3] - o.p[2], p - o.p[2]);
    auto d = cross(o.p[0] - o.p[3], p - o.p[3]);

    return a < 0.0f && b < 0.0f && c < 0.0f && d < 0.0f;
}

static inline bool intersects(OBB a, OBB b) {
    for (int i = 0; i < 4; ++i) {
        Vec2 p0 = a.p[i], p1 = a.p[(i + 1) % 4];
        if (cross(p1 - p0, b.p[0] - p0) > 0 &&
            cross(p1 - p0, b.p[1] - p0) > 0 &&
            cross(p1 - p0, b.p[2] - p0) > 0 &&
            cross(p1 - p0, b.p[3] - p0) > 0) return false;
    }
    for (int i = 0; i < 4; ++i) {
        Vec2 p0 = b.p[i], p1 = b.p[(i + 1) % 4];
        if (cross(p1 - p0, a.p[0] - p0) > 0 &&
            cross(p1 - p0, a.p[1] - p0) > 0 &&
            cross(p1 - p0, a.p[2] - p0) > 0 &&
            cross(p1 - p0, a.p[3] - p0) > 0) return false;
    }
    return true;
}

//TODO: function to correct the winding of points in an OBB

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// RANDOM NUMBER GENERATION                                                                                         ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//TODO: move this stuff into its own file?
#include <assert.h>

inline int global_pcg_state = 0;
static inline int rand_int() {
    global_pcg_state = global_pcg_state * 0xdb429a1d + 1;
    return global_pcg_state & 0x00FFFFFF; //lop off the top bits because iirc LCGs are better distributed that way? -m
}

static inline float rand_float(void) { return rand_int() * 5.9604644775390625e-8f; }
static inline float rand_float(float max) { return rand_float() * max; }
static inline float rand_float(float min, float max) { return rand_float() * (max - min) + min; }
static inline int rand_int(int max) { return rand_int() % max; }
static inline int rand_int(int min, int max) { assert(max > min); return rand_int() % (max - min) + min; }

//TODO: make this not be biased toward corners
static inline Vec3 rand_dir() {
    return noz(vec3(rand_float(), rand_float(), rand_float()) - vec3(0.5f));
}

#endif //MATH_HPP
