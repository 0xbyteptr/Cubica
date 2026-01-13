#pragma once
#include <cstdint>
#include <cmath>

namespace Noise {

static inline float lerp(float a, float b, float t) {
    return a + t * (b - a);
}

static inline float fade(float t) {
    // 6t^5 - 15t^4 + 10t^3
    return t * t * t * (t * (t * 6 - 15) + 10);
}

static inline uint32_t hash32(uint32_t x, uint32_t y) {
    // simple integer hash; deterministic
    uint32_t h = x * 374761393u + y * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h;
}

static inline float grad(int ix, int iy, float x, float y) {
    uint32_t h = hash32(static_cast<uint32_t>(ix), static_cast<uint32_t>(iy));
    // map hash to angle
    float angle = (h & 0xFFFFu) * (2.0f * 3.14159265358979323846f / 65536.0f);
    float gx = std::cos(angle);
    float gy = std::sin(angle);
    return gx * x + gy * y;
}

static inline float perlin2d(float x, float y) {
    int xi = static_cast<int>(std::floor(x));
    int yi = static_cast<int>(std::floor(y));
    float xf = x - xi;
    float yf = y - yi;

    float n00 = grad(xi, yi, xf, yf);
    float n10 = grad(xi + 1, yi, xf - 1.0f, yf);
    float n01 = grad(xi, yi + 1, xf, yf - 1.0f);
    float n11 = grad(xi + 1, yi + 1, xf - 1.0f, yf - 1.0f);

    float u = fade(xf);
    float v = fade(yf);

    float nx0 = lerp(n00, n10, u);
    float nx1 = lerp(n01, n11, u);
    float nxy = lerp(nx0, nx1, v);

    // Perlin returns roughly in range [-sqrt(2)/2, sqrt(2)/2]; scale to approx [-1,1]
    return nxy * 1.41421356237f;
}

static inline float fbm2d(float x, float y, int octaves = 4, float lacunarity = 2.0f, float gain = 0.5f) {
    float sum = 0.0f;
    float amp = 1.0f;
    float freq = 1.0f;
    float maxAmp = 0.0f;
    for (int i = 0; i < octaves; ++i) {
        sum += perlin2d(x * freq, y * freq) * amp;
        maxAmp += amp;
        amp *= gain;
        freq *= lacunarity;
    }
    if (maxAmp == 0.0f) return 0.0f;
    return sum / maxAmp; // normalized to roughly [-1,1]
}

} // namespace Noise
