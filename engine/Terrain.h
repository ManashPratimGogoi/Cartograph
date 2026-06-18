#pragma once

#include "Noise.h"

#include <cstdint>
#include <string>
#include <vector>

constexpr int kMapWidth = 768;
constexpr int kMapHeight = 512;

struct Color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

enum class ViewMode {
    Biomes,
    Height,
    Temperature,
    Moisture
};

enum class TerrainTheme {
    Mixed,
    Desert,
    Forest,
    Alpine
};

struct TerrainSettings {
    int seed = 1337;
    double frequency = 1.0;
    int octaves = 5;
    double persistence = 0.50;
    double lacunarity = 2.0;
    double waterLevel = 0.36;
    double offsetX = 0.0;
    double offsetY = 0.0;
    ViewMode viewMode = ViewMode::Biomes;
    TerrainTheme theme = TerrainTheme::Mixed;
};

struct TerrainCell {
    double height = 0.0;
    double temperature = 0.0;
    double moisture = 0.0;
    double slope = 0.0;
    Color biomeColor{0, 0, 0};
    std::string biomeName = "Unknown";
    bool water = false;
    bool coast = false;
    bool river = false;
    bool road = false;
    bool forestDetail = false;
    bool mountainDetail = false;
};

struct MapFeature {
    int x = 0;
    int y = 0;
    std::string name;
    Color color{0, 0, 0};
    TerrainTheme theme = TerrainTheme::Mixed;
};

class TerrainGenerator {
public:
    explicit TerrainGenerator(const TerrainSettings& settings);

    void reseed(const TerrainSettings& settings);
    void generate(
        const TerrainSettings& settings,
        std::vector<TerrainCell>& cells,
        std::vector<uint32_t>& pixels,
        std::vector<MapFeature>& features
    );

private:
    PerlinNoise heightNoise;
    PerlinNoise temperatureNoise;
    PerlinNoise moistureNoise;
    PerlinNoise detailNoise;
    PerlinNoise riverNoise;
    PerlinNoise roadNoise;

    static double fractalNoise(const PerlinNoise& noise, const TerrainSettings& settings, double x, double y, double baseScale);
    static double singleNoise(const PerlinNoise& noise, const TerrainSettings& settings, double x, double y, double baseScale);
    static Color colorForCell(const TerrainCell& cell, const TerrainSettings& settings);
    static Color biomeBlend(double height, double temperature, double moisture, const TerrainSettings& settings, std::string& name);
    static void paintMapDetails(std::vector<TerrainCell>& cells, std::vector<uint32_t>& pixels, const TerrainSettings& settings);
    static void createFeatureMarkers(const std::vector<TerrainCell>& cells, const TerrainSettings& settings, std::vector<MapFeature>& features);
};

Color lerpColor(Color a, Color b, double t);
Color blendColor(Color base, Color overlay, double amount);
uint32_t packColor(Color color);
const char* viewModeName(ViewMode mode);
const char* terrainThemeName(TerrainTheme theme);
const char* terrainThemeTagline(TerrainTheme theme);
