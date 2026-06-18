#pragma once

#include <vector>

class PerlinNoise {
public:
    explicit PerlinNoise(int seed = 0);

    void reseed(int seed);
    double noise(double x, double y) const;

private:
    std::vector<int> permutation;

    static double fade(double t);
    static double lerp(double a, double b, double t);
    static double grad(int hash, double x, double y);
};
