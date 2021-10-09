/*
TODO:
- create and publish public repo
- make pitch be in bytes
- make single-header
- toggle-able perf instrumentation
- rename symbols
- documentation
- v0.1 release

- more comprehensive test suite (subrects, moire patterns, upscaling, bandwidth calculations)
- figure out what the best filters are for upscaling
- harden against negative filters
- v1.0 release

- allow providing memory externally
- preprocessor config to not include stdlib
- v1.1 release

- subrect API + test cases
- error-check args more thoroughly
- v1.2 release

- switch to floats and performance test
- custom filter API (incl. common ones by default?)
- v1.3 release

- test perf with prefaulted memory
- tweaky optimizations (filter/weights optimizations, unroll convolution loops)
- v1.3 release

- NEON SIMD
*/

#include "trace.hpp"
#include <stdlib.h>
#include <stdio.h> //for debug prints (to be removed eventually)
#include <math.h> //for sin()
#include <assert.h>

static const double hamming_filter_radius = 1.0;
static inline double hamming_filter(double x) {
    if (x < 0.0) x = -x;
    if (x == 0.0) return 1.0;
    if (x >= 1.0) return 0.0;
    x = x * 3.14159265358979323846; //pi
    return sin(x) / x * (0.54 + 0.46 * cos(x));
}

struct Range { int start, len; };
struct MsfWeights {
    Range * ranges;
    uint16_t * weights; //2D array of `size * stride` quantized to fixed point at scale `1.0 -> 2^15`
    int size, stride;
};

//NOTE: A note about coordinate systems:
//      In all coordinate calculations, pixels are treated as 1x1 unit squares with a color sample at their center.
//      So 0 = left edge of first pixel, 1 = right edge of first pixel and left edge of second pixel, etc.
//      and the area of the image spans [0, size] inclusive.
//      However, when calculating weights, we simply evaluate the filter function at the source pixel's center.
//      This makes some calculations confusing.
//      We do not (for now) integrate the filter function over the pixel's whole area,
//      even though I'm pretty sure that would be the maximally correct thing to do.
//      If we did integrate the filter function, we would need to provide an analytically integrated filter function,
//      but we could then do away with the need to sum the weights and normalize by the sum,
//      because we could simply calculate the analytic integral of the full sample range
//      (taking care to correctly handle samples that clip one or both edges of the image).
//      The main reason to use the integral is better accuracy.
//      The main reason to use discrete samples is slightly fewer samples (1 less sample per pixel on average, I think?)
//      are needed, and an analytic integral of the filter function is not needed.

static const int PRECISION = 15;
static const int BIAS = 1 << (PRECISION - 1);

//NOTE: `align` = 1 for scalar, 4 for SSE
//PERF TODO: optimize this function to get better speed for small images
//           (faster filter, floats instead of doubles, no out-of-range filter calls, SIMD, etc.)
static MsfWeights calculate_weights(int inSize, int outSize, int align) { TimeFunc //TODO: better understand this
    double rawScale = (double) inSize / outSize;
    double filterScale = fmax(1.0, rawScale);
    double inRadius = hamming_filter_radius * filterScale;
    int stride = (int) ceil(inRadius * 2); //the maximum number of samples per pixel
    stride = (stride + align - 1) & ~(align - 1); //round up to a multiple of a given power-of-two alignment, for SIMD

    //must be zeroed because the weight quantization algorithm takes max of ALL array elements, not just used ones
    double * weights = (double *) calloc(outSize * stride, sizeof(double));
    Range * ranges = (Range *) malloc(outSize * sizeof(Range));

    int lenSum = 0;
    int maxLen = 0;
    for (int dstx = 0; dstx < outSize; ++dstx) {
        //calculate the extents of the filter
        //`center` is where the center (sample point) of the destination pixel will be when mapped into the source range
        double center = (dstx + 0.5) * rawScale;
        //NOTE: if we switch to integrating over pixel area, these will need to be floor/ceil instead of round/round
        int first = lround(fmax(0, center - inRadius)); //inclusive
        int last = lround(fminf(inSize, center + inRadius)); //exclusive? I think???
        int len = (last - first + align - 1) & ~(align - 1); //round up len to alignment for SIMD
        assert(len <= stride);
        if (first + len > inSize) first = inSize - len; //push back the range to avoid running past the end of the array
        double * w = &weights[dstx * stride]; //(the above line is what breaks our code if `align > inSize`)

        //store and sum the weights
        //NOTE: we only need to compute `len` weights because the remaining values were already zeroed by calloc()
        double sum = 0.0;
        for (int offx = 0; offx < stride; ++offx) {
            double weight = hamming_filter((first + offx - center + 0.5) * 1.0 / filterScale);
            w[offx] = weight;
            sum += weight;
        }

        //normalize using the sum //TODO: is it valid for the sum to be 0?
        // assert(sum); //apparently sometimes it IS zero, although I haven't yet investigated when/why
        if (sum != 0) {
            for (int offx = 0; offx < stride; ++offx) { //yes, this should be `stride`, not `len` //why?
                w[offx] /= sum;
            }
        }

        ranges[dstx].start = first;
        ranges[dstx].len = len;
        lenSum += len;
        if (len > maxLen) maxLen = len;
    }
    // printf("%d (len %.2f avg, %d max)\n", stride, (double) lenSum / (double) outSize, maxLen); fflush(stdout);

    //quantize the results for faster SIMD later
    uint16_t * quantized = (uint16_t *) malloc(outSize * stride * sizeof(uint16_t));
    for (int i = 0; i < outSize * stride; ++i) {
        quantized[i] = lround(fmax(0, fmin((1 << PRECISION) - 1, weights[i] * (1 << PRECISION))));
    }

    free(weights);
    return { ranges, quantized, outSize, stride };
}

struct MsfPixel { uint8_t r, g, b, a; };
struct MsfImage {
    MsfPixel * p;
    int w, h, pitch;
};

#include <emmintrin.h> //SSE2
#include <tmmintrin.h> //SSSE3

//PERF TODOs:
//- multi-line unrolled version?
//- 8-wide loop that does two pixels in 16-bit??? (I don't think this will win because it needs two loads?)
//- AVX2 version???
//- optimize vertical filter for very large ranges (by doing full cache lines at a time?)
static void msf_resample_image(MsfImage src, MsfImage dst) { TimeFunc
    MsfImage tmp = { (MsfPixel *) malloc(dst.w * src.h * sizeof(MsfPixel)), dst.w, src.h, dst.w };

    //horizontally squash/stretch src->tmp
    //NOTE: if image is less than 4 pixels wide, aligning to 4 will break, and we can't use SIMD anyway, so don't align
    //TODO: align = 1 if we are compiling without SIMD
    MsfWeights horiz = calculate_weights(src.w, dst.w, dst.w < 4? 1 : 4);
    TimeLoop("horizontal") for (int y = 0; y < tmp.h; ++y) {
        for (int x = 0; x < tmp.w; x += 1) {
            if (horiz.ranges[x].len >= 4) { //for images smaller than 4 pixels wide, we can't use SIMD even if we want to
                #if 0 //SSSE3
                    __m128i sum = _mm_set1_epi32(BIAS);
                    for (int i = 0; i < horiz.ranges[x].len - 3; i += 4) {
                        //TODO: is this a strict aliasing violation?
                        __m128i p = _mm_loadu_si128((__m128i *) &src.p[y * src.pitch + horiz.ranges[x].start + i]);
                        __m128i w1 = _mm_set1_epi32(*(int32_t *) &horiz.weights[x * horiz.stride + i + 0]);
                        __m128i w2 = _mm_set1_epi32(*(int32_t *) &horiz.weights[x * horiz.stride + i + 2]);
                        __m128i swizzle1 = _mm_set_epi8(-1,  7, -1,  3, -1,  6, -1,  2, -1,  5, -1,  1, -1,  4, -1,  0);
                        __m128i swizzle2 = _mm_set_epi8(-1, 15, -1, 11, -1, 14, -1, 10, -1, 13, -1,  9, -1, 12, -1,  8);
                        sum = _mm_add_epi32(sum, _mm_madd_epi16(_mm_shuffle_epi8(p, swizzle1), w1));
                        sum = _mm_add_epi32(sum, _mm_madd_epi16(_mm_shuffle_epi8(p, swizzle2), w2));
                    }
                    sum = _mm_srli_epi32(sum, PRECISION);
                    sum = _mm_packs_epi32(sum, sum);
                    sum = _mm_packus_epi16(sum, sum);
                    _mm_storeu_si32(&tmp.p[y * tmp.pitch + x], sum);
                #else //SSE2
                    //TODO: why the fuck does this need to be `1 << 6` to look right (instead of the expected `1 << 7`)
                    //      but the vertical convolution looks correct with either `1 << 6` or `1 << 7` !?!?!?
                    __m128i rbsum = _mm_set1_epi16(1 << 6); //we shift the weights to be 16-bit before multiplication
                    __m128i gasum = _mm_set1_epi16(1 << 6); //so this should NOT be based on `PRECISION` or `BIAS`
                    for (int i = 0; i < horiz.ranges[x].len - 3; i += 4) {
                        __m128i p = _mm_loadu_si128((__m128i *) &src.p[y * src.pitch + horiz.ranges[x].start + i]);
                        __m128i ww = _mm_loadu_si64((__m128i *) &horiz.weights[x * horiz.stride + i]);
                        __m128i w = _mm_slli_epi16(_mm_unpacklo_epi16(ww, ww), 16 - PRECISION);
                        __m128i rb = _mm_slli_epi16(p, 8);
                        __m128i ga = _mm_and_si128(p, _mm_set1_epi32(0xFF00FF00));
                        rbsum = _mm_adds_epu16(rbsum, _mm_mulhi_epu16(rb, w));
                        gasum = _mm_adds_epu16(gasum, _mm_mulhi_epu16(ga, w));
                    }
                    __m128 rbrbgaga1 = _mm_shuffle_ps(_mm_castsi128_ps(rbsum), _mm_castsi128_ps(gasum), 0b01'00'01'00);
                    __m128 rbrbgaga2 = _mm_shuffle_ps(_mm_castsi128_ps(rbsum), _mm_castsi128_ps(gasum), 0b11'10'11'10);
                    __m128i rbrbgaga = _mm_adds_epi16(_mm_castps_si128(rbrbgaga1), _mm_castps_si128(rbrbgaga2));
                    //TODO: we can maybe get slightly higher efficiency here by doing two rows at once?
                    __m128i rbga1 = _mm_shuffle_epi32(rbrbgaga, 0b00'00'10'00);
                    __m128i rbga2 = _mm_shuffle_epi32(rbrbgaga, 0b00'00'11'01);
                    __m128i rbga = _mm_adds_epu16(rbga1, rbga2);
                    __m128i rb = _mm_srli_epi16(rbga, 8);
                    __m128i ga = _mm_and_si128(_mm_srli_epi64(rbga, 32), _mm_set1_epi32(0xFF00FF00));
                    __m128i out = _mm_or_si128(rb, ga);
                    _mm_storeu_si32(&tmp.p[y * tmp.pitch + x], out);
                #endif
            } else {
                int r = BIAS, g = BIAS, b = BIAS, a = BIAS;
                for (int i = 0; i < horiz.ranges[x].len; ++i) {
                    r += src.p[y * src.pitch + horiz.ranges[x].start + i].r * horiz.weights[x * horiz.stride + i];
                    g += src.p[y * src.pitch + horiz.ranges[x].start + i].g * horiz.weights[x * horiz.stride + i];
                    b += src.p[y * src.pitch + horiz.ranges[x].start + i].b * horiz.weights[x * horiz.stride + i];
                    a += src.p[y * src.pitch + horiz.ranges[x].start + i].a * horiz.weights[x * horiz.stride + i];
                }
                r = r > (1 << (PRECISION + 8)) - 1? (1 << (PRECISION + 8)) - 1 : r;
                g = g > (1 << (PRECISION + 8)) - 1? (1 << (PRECISION + 8)) - 1 : g;
                b = b > (1 << (PRECISION + 8)) - 1? (1 << (PRECISION + 8)) - 1 : b;
                a = a > (1 << (PRECISION + 8)) - 1? (1 << (PRECISION + 8)) - 1 : a;
                tmp.p[y * tmp.pitch + x] = {
                    (uint8_t) (r >> PRECISION),
                    (uint8_t) (g >> PRECISION),
                    (uint8_t) (b >> PRECISION),
                    (uint8_t) (a >> PRECISION),
                };
            }
        }
    }
    free(horiz.ranges); free(horiz.weights);

    //vertically squash/stretch tmp->dst
    MsfWeights vert = calculate_weights(src.h, dst.h, 1);
    TimeLoop("vertical") for (int y = 0; y < dst.h; ++y) {
        int x = 0;
        for (; x < dst.w - 3; x += 4) {
            __m128i rbsum = _mm_set1_epi16(1 << 6); //we shift the weights to be 16-bit before multiplication
            __m128i gasum = _mm_set1_epi16(1 << 6); //so this should NOT be based on `PRECISION` or `BIAS`
            for (int i = 0; i < vert.ranges[y].len; ++i) {
                __m128i p = _mm_loadu_si128((__m128i *) &tmp.p[(vert.ranges[y].start + i) * tmp.pitch + x]);
                __m128i w = _mm_set1_epi16(vert.weights[y * vert.stride + i] << (16 - PRECISION));
                __m128i rb = _mm_slli_epi16(p, 8);
                __m128i ga = _mm_and_si128(p, _mm_set1_epi32(0xFF00FF00));
                rbsum = _mm_adds_epu16(rbsum, _mm_mulhi_epu16(rb, w));
                gasum = _mm_adds_epu16(gasum, _mm_mulhi_epu16(ga, w));
            }
            __m128i out = _mm_or_si128(_mm_srli_epi16(rbsum, 8), _mm_and_si128(gasum, _mm_set1_epi32(0xFF00FF00)));
            _mm_storeu_si128((__m128i *) &dst.p[y * dst.pitch + x], out);
        }
        for (; x < dst.w; ++x) {
            int r = BIAS, g = BIAS, b = BIAS, a = BIAS;
            for (int i = 0; i < vert.ranges[y].len; ++i) {
                r += tmp.p[(vert.ranges[y].start + i) * tmp.pitch + x].r * vert.weights[y * vert.stride + i];
                g += tmp.p[(vert.ranges[y].start + i) * tmp.pitch + x].g * vert.weights[y * vert.stride + i];
                b += tmp.p[(vert.ranges[y].start + i) * tmp.pitch + x].b * vert.weights[y * vert.stride + i];
                a += tmp.p[(vert.ranges[y].start + i) * tmp.pitch + x].a * vert.weights[y * vert.stride + i];
            }
            r = r > (1 << (PRECISION + 8)) - 1? (1 << (PRECISION + 8)) - 1 : r;
            g = g > (1 << (PRECISION + 8)) - 1? (1 << (PRECISION + 8)) - 1 : g;
            b = b > (1 << (PRECISION + 8)) - 1? (1 << (PRECISION + 8)) - 1 : b;
            a = a > (1 << (PRECISION + 8)) - 1? (1 << (PRECISION + 8)) - 1 : a;
            dst.p[y * dst.pitch + x] = {
                (uint8_t) (r >> PRECISION),
                (uint8_t) (g >> PRECISION),
                (uint8_t) (b >> PRECISION),
                (uint8_t) (a >> PRECISION),
            };
        }
    }
    free(vert.ranges); free(vert.weights);

    free(tmp.p);
}

//pitch is in pixels (for now, before releasing the library I will change it to bytes for more flexibility)
void msf_resample_pitch(void * in, int inw, int inh, int inPitch, void * out, int outw, int outh, int outPitch) {
    msf_resample_image({ (MsfPixel *) in, inw, inh, inPitch }, { (MsfPixel *) out, outw, outh, outPitch });
}

//pitch is in pixels (for now, before releasing the library I will change it to bytes for more flexibility)
void msf_resample(void * in, int inw, int inh, void * out, int outw, int outh) {
    msf_resample_image({ (MsfPixel *) in, inw, inh, inw }, { (MsfPixel *) out, outw, outh, outw });
}
