#include "Noise.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <random>

PerlinNoise::PerlinNoise(int seed) {
    reseed(seed);
}

void PerlinNoise::reseed(int seed) {
    std::array<int, 256> base{};
    for (int i = 0; i < 256; ++i) {
        base[i] = i;
    }

    std::mt19937 rng(static_cast<uint32_t>(seed));
    std::shuffle(base.begin(), base.end(), rng);

    permutation.resize(512);
    for (int i = 0; i < 512; ++i) {
        permutation[i] = base[i & 255];
    }
}

double PerlinNoise::noise(double x, double y) const {
    const int xi = static_cast<int>(std::floor(x)) & 255;
    const int yi = static_cast<int>(std::floor(y)) & 255;
    const double xf = x - std::floor(x);
    const double yf = y - std::floor(y);

    const double u = fade(xf);
    const double v = fade(yf);

    const int aa = permutation[permutation[xi] + yi];
    const int ab = permutation[permutation[xi] + yi + 1];
    const int ba = permutation[permutation[xi + 1] + yi];
    const int bb = permutation[permutation[xi + 1] + yi + 1];

    const double x1 = lerp(grad(aa, xf, yf), grad(ba, xf - 1.0, yf), u);
    const double x2 = lerp(grad(ab, xf, yf - 1.0), grad(bb, xf - 1.0, yf - 1.0), u);

    return (lerp(x1, x2, v) + 1.0) * 0.5;
}

double PerlinNoise::fade(double t) {
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}

double PerlinNoise::lerp(double a, double b, double t) {
    return a + t * (b - a);
}

double PerlinNoise::grad(int hash, double x, double y) {
    switch (hash & 7) {
    case 0: return x + y;
    case 1: return -x + y;
    case 2: return x - y;
    case 3: return -x - y;
    case 4: return x;
    case 5: return -x;
    case 6: return y;
    default: return -y;
    }
}
