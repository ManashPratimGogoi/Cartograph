#include "Terrain.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace {

struct BiomeProfile {
    const char* name;
    Color color;
    double elevation;
    double temperature;
    double moisture;
};

constexpr Color kDeepWater{19, 57, 112};
constexpr Color kShallowWater{54, 137, 174};
constexpr Color kFoam{220, 230, 207};
constexpr Color kRiver{45, 132, 190};
constexpr Color kRoad{154, 119, 68};
constexpr Color kRoadLight{213, 181, 116};

const BiomeProfile kBiomes[] = {
    {"Beach", {222, 199, 130}, 0.39, 0.62, 0.46},
    {"Desert", {207, 161, 82}, 0.51, 0.86, 0.14},
    {"Savanna", {166, 164, 82}, 0.53, 0.77, 0.32},
    {"Grassland", {101, 159, 76}, 0.55, 0.58, 0.43},
    {"Forest", {55, 128, 70}, 0.57, 0.50, 0.72},
    {"Rainforest", {34, 113, 82}, 0.53, 0.82, 0.88},
    {"Tundra", {147, 157, 134}, 0.64, 0.18, 0.48},
    {"Rock", {124, 116, 103}, 0.78, 0.45, 0.40},
    {"Snow", {234, 238, 230}, 0.89, 0.10, 0.48},
};

const char* kSettlementNames[] = {
    "Alderwatch", "Highmere", "Rivergate", "Stoneford", "Oakrest",
    "Mistvale", "Sunport", "Greenhollow", "Northpass", "Westhaven",
    "Ironhill", "Lakefall", "Brightwick", "Dawnfield", "Embercross"
};

const char* kDesertNames[] = {
    "Sunspire", "Amber Oasis", "Dunehold", "Mirage Gate", "Cinder Well",
    "Saffron Camp", "Glass Mesa", "Drywind Post", "Ember Dunes"
};

const char* kForestNames[] = {
    "Mossgrove", "Fernwatch", "Pinebarrow", "Elderwood", "Greenveil",
    "Rainroot", "Thornrest", "Wildbrook", "Canopy Hall"
};

const char* kAlpineNames[] = {
    "Frostpeak", "Snowgate", "White Pass", "Glacier Hut", "Highcrag",
    "Icefall", "Pinewatch", "North Ridge", "Stormcap"
};

double clamp01(double value) {
    return std::clamp(value, 0.0, 1.0);
}

double themeWaterLevel(const TerrainSettings& settings) {
    switch (settings.theme) {
    case TerrainTheme::Desert:
        return std::clamp(settings.waterLevel - 0.12, 0.12, 0.42);
    case TerrainTheme::Forest:
        return std::clamp(settings.waterLevel + 0.03, 0.24, 0.58);
    case TerrainTheme::Alpine:
        return std::clamp(settings.waterLevel - 0.05, 0.18, 0.48);
    case TerrainTheme::Mixed:
    default:
        return settings.waterLevel;
    }
}

double themedHeight(double height, double continentalNoise, const TerrainSettings& settings) {
    switch (settings.theme) {
    case TerrainTheme::Desert:
        return clamp01(height * 0.80 + continentalNoise * 0.08 + 0.10);
    case TerrainTheme::Forest:
        return clamp01(height * 0.86 + continentalNoise * 0.06 + 0.04);
    case TerrainTheme::Alpine:
        return clamp01(std::pow(height, 0.72) * 0.96 + continentalNoise * 0.08 + 0.06);
    case TerrainTheme::Mixed:
    default:
        return clamp01(height + continentalNoise * 0.06 + 0.02);
    }
}

double themedTemperature(double temperature, double height, const TerrainSettings& settings) {
    switch (settings.theme) {
    case TerrainTheme::Desert:
        return clamp01(temperature * 0.78 + 0.30 - height * 0.05);
    case TerrainTheme::Forest:
        return clamp01(temperature * 0.70 + 0.12 - height * 0.08);
    case TerrainTheme::Alpine:
        return clamp01(temperature * 0.45 + 0.06 - height * 0.22);
    case TerrainTheme::Mixed:
    default:
        return clamp01(temperature);
    }
}

double themedMoisture(double moisture, double height, const TerrainSettings& settings) {
    switch (settings.theme) {
    case TerrainTheme::Desert:
        return clamp01(moisture * 0.34 + (height < themeWaterLevel(settings) + 0.08 ? 0.22 : 0.02));
    case TerrainTheme::Forest:
        return clamp01(moisture * 0.55 + 0.40 + (1.0 - height) * 0.08);
    case TerrainTheme::Alpine:
        return clamp01(moisture * 0.48 + 0.22 + height * 0.18);
    case TerrainTheme::Mixed:
    default:
        return clamp01(moisture);
    }
}

double hash01(int x, int y, int seed) {
    uint32_t h = static_cast<uint32_t>(x) * 374761393u
        + static_cast<uint32_t>(y) * 668265263u
        + static_cast<uint32_t>(seed) * 1442695041u;
    h = (h ^ (h >> 13u)) * 1274126177u;
    h ^= h >> 16u;
    return static_cast<double>(h & 0x00FFFFFFu) / static_cast<double>(0x01000000u);
}

Color grayscale(double value) {
    const auto c = static_cast<uint8_t>(std::clamp(value * 255.0, 0.0, 255.0));
    return {c, c, c};
}

Color temperatureColor(double value) {
    return lerpColor({45, 90, 176}, {225, 89, 55}, value);
}

Color moistureColor(double value) {
    return lerpColor({215, 174, 89}, {42, 110, 184}, value);
}

Color multiplyColor(Color color, double amount) {
    return {
        static_cast<uint8_t>(std::clamp(color.r * amount, 0.0, 255.0)),
        static_cast<uint8_t>(std::clamp(color.g * amount, 0.0, 255.0)),
        static_cast<uint8_t>(std::clamp(color.b * amount, 0.0, 255.0)),
    };
}

bool isForestBiome(const std::string& name) {
    return name == "Forest" || name == "Rainforest" || name == "Highland Forest" || name == "Pine Forest" || name == "Moss Meadow";
}

bool isMountainBiome(const std::string& name) {
    return name == "Rock" || name == "Snow" || name == "Alpine Rock" || name == "Glacier" || name == "Mesa";
}

} // namespace

TerrainGenerator::TerrainGenerator(const TerrainSettings& settings)
    : heightNoise(settings.seed),
      temperatureNoise(settings.seed + 1013),
      moistureNoise(settings.seed + 7919),
      detailNoise(settings.seed + 2711),
      riverNoise(settings.seed + 6151),
      roadNoise(settings.seed + 9857) {
}

void TerrainGenerator::reseed(const TerrainSettings& settings) {
    heightNoise.reseed(settings.seed);
    temperatureNoise.reseed(settings.seed + 1013);
    moistureNoise.reseed(settings.seed + 7919);
    detailNoise.reseed(settings.seed + 2711);
    riverNoise.reseed(settings.seed + 6151);
    roadNoise.reseed(settings.seed + 9857);
}

void TerrainGenerator::generate(
    const TerrainSettings& settings,
    std::vector<TerrainCell>& cells,
    std::vector<uint32_t>& pixels,
    std::vector<MapFeature>& features
) {
    cells.assign(kMapWidth * kMapHeight, {});
    pixels.assign(kMapWidth * kMapHeight, 0);
    features.clear();

    double minHeight = std::numeric_limits<double>::max();
    double maxHeight = std::numeric_limits<double>::lowest();

    for (int y = 0; y < kMapHeight; ++y) {
        for (int x = 0; x < kMapWidth; ++x) {
            const size_t index = static_cast<size_t>(y) * kMapWidth + x;
            const double rawHeight = fractalNoise(heightNoise, settings, x, y, 178.0);
            cells[index].height = rawHeight;
            minHeight = std::min(minHeight, rawHeight);
            maxHeight = std::max(maxHeight, rawHeight);
        }
    }

    const double heightRange = std::max(0.0001, maxHeight - minHeight);

    for (int y = 0; y < kMapHeight; ++y) {
        for (int x = 0; x < kMapWidth; ++x) {
            const size_t index = static_cast<size_t>(y) * kMapWidth + x;
            TerrainCell& cell = cells[index];

            double height = (cell.height - minHeight) / heightRange;
            const double dx = (x - kMapWidth * 0.5) / (kMapWidth * 0.5);
            const double dy = (y - kMapHeight * 0.5) / (kMapHeight * 0.5);
            const double islandMask = clamp01(1.15 - std::sqrt(dx * dx + dy * dy) * 0.72);
            const double continentalNoise = singleNoise(detailNoise, settings, x + 1100.0, y - 700.0, 420.0);
            height = themedHeight(height * islandMask, continentalNoise, settings);

            const double latitude = static_cast<double>(y) / static_cast<double>(kMapHeight - 1);
            const double temperatureNoiseValue = fractalNoise(temperatureNoise, settings, x + 4000.0, y - 1300.0, 260.0);
            const double moistureNoiseValue = fractalNoise(moistureNoise, settings, x - 2200.0, y + 5100.0, 230.0);

            const double latitudeHeat = 0.24 + latitude * 0.72;
            const double elevationCooling = height * 0.30;
            const double waterLevel = themeWaterLevel(settings);
            const double waterMoisture = height < waterLevel + 0.05 ? 0.20 : 0.0;

            cell.height = height;
            cell.temperature = themedTemperature(temperatureNoiseValue * 0.55 + latitudeHeat * 0.45 - elevationCooling, height, settings);
            cell.moisture = themedMoisture(moistureNoiseValue * 0.75 + (1.0 - height) * 0.12 + waterMoisture, height, settings);
            cell.water = cell.height < waterLevel;
            cell.biomeColor = biomeBlend(cell.height, cell.temperature, cell.moisture, settings, cell.biomeName);
        }
    }

    for (int y = 0; y < kMapHeight; ++y) {
        for (int x = 0; x < kMapWidth; ++x) {
            const size_t index = static_cast<size_t>(y) * kMapWidth + x;
            TerrainCell& cell = cells[index];

            const int left = std::max(0, x - 1);
            const int right = std::min(kMapWidth - 1, x + 1);
            const int up = std::max(0, y - 1);
            const int down = std::min(kMapHeight - 1, y + 1);

            const double heightDx = cells[static_cast<size_t>(y) * kMapWidth + right].height
                - cells[static_cast<size_t>(y) * kMapWidth + left].height;
            const double heightDy = cells[static_cast<size_t>(down) * kMapWidth + x].height
                - cells[static_cast<size_t>(up) * kMapWidth + x].height;
            cell.slope = std::sqrt(heightDx * heightDx + heightDy * heightDy);

            bool touchesWater = false;
            for (int oy = -1; oy <= 1; ++oy) {
                for (int ox = -1; ox <= 1; ++ox) {
                    const int nx = std::clamp(x + ox, 0, kMapWidth - 1);
                    const int ny = std::clamp(y + oy, 0, kMapHeight - 1);
                    touchesWater = touchesWater || cells[static_cast<size_t>(ny) * kMapWidth + nx].water;
                }
            }

            const double waterLevel = themeWaterLevel(settings);
            cell.coast = !cell.water && touchesWater && cell.height < waterLevel + 0.055;

            const double riverField = singleNoise(riverNoise, settings, x + 9000.0, y - 3000.0, 104.0);
            const double riverBand = 1.0 - std::abs(riverField - 0.50) * 34.0;
            const double riverMoistureNeed = settings.theme == TerrainTheme::Desert ? 0.28 : (settings.theme == TerrainTheme::Forest ? 0.38 : 0.46);
            const double riverBandNeed = settings.theme == TerrainTheme::Desert ? 0.58 : (settings.theme == TerrainTheme::Forest ? 0.24 : 0.34);
            cell.river = !cell.water
                && !cell.coast
                && cell.height > waterLevel + 0.035
                && cell.height < (settings.theme == TerrainTheme::Alpine ? 0.86 : 0.80)
                && cell.moisture > riverMoistureNeed
                && riverBand > riverBandNeed;

            const double roadField = singleNoise(roadNoise, settings, x - 2700.0, y + 2200.0, 155.0);
            const double roadBand = 1.0 - std::abs(roadField - 0.50) * 44.0;
            const double roadBandNeed = settings.theme == TerrainTheme::Desert ? 0.18 : (settings.theme == TerrainTheme::Alpine ? 0.38 : 0.30);
            cell.road = !cell.water
                && !cell.river
                && cell.height > waterLevel + 0.04
                && cell.height < 0.70
                && cell.slope < (settings.theme == TerrainTheme::Alpine ? 0.026 : 0.020)
                && roadBand > roadBandNeed;

            const double detail = hash01(x, y, settings.seed);
            const double forestCutoff = settings.theme == TerrainTheme::Forest ? 0.24 : 0.48;
            const double mountainCutoff = settings.theme == TerrainTheme::Alpine ? 0.42 : 0.65;
            cell.forestDetail = !cell.water && isForestBiome(cell.biomeName) && detail > forestCutoff;
            cell.mountainDetail = !cell.water && isMountainBiome(cell.biomeName) && (cell.slope > 0.006 || detail > mountainCutoff);
        }
    }

    paintMapDetails(cells, pixels, settings);
    createFeatureMarkers(cells, settings, features);
}

double TerrainGenerator::fractalNoise(const PerlinNoise& noise, const TerrainSettings& settings, double x, double y, double baseScale) {
    double amplitude = 1.0;
    double frequency = settings.frequency;
    double value = 0.0;
    double maxValue = 0.0;

    for (int octave = 0; octave < settings.octaves; ++octave) {
        const double sampleX = (x + settings.offsetX) / baseScale * frequency;
        const double sampleY = (y + settings.offsetY) / baseScale * frequency;
        value += noise.noise(sampleX, sampleY) * amplitude;
        maxValue += amplitude;
        amplitude *= settings.persistence;
        frequency *= settings.lacunarity;
    }

    return std::pow(clamp01(value / maxValue), 1.12);
}

double TerrainGenerator::singleNoise(const PerlinNoise& noise, const TerrainSettings& settings, double x, double y, double baseScale) {
    const double sampleX = (x + settings.offsetX) / baseScale * settings.frequency;
    const double sampleY = (y + settings.offsetY) / baseScale * settings.frequency;
    return noise.noise(sampleX, sampleY);
}

Color TerrainGenerator::biomeBlend(double height, double temperature, double moisture, const TerrainSettings& settings, std::string& name) {
    const double waterLevel = themeWaterLevel(settings);

    if (height < waterLevel) {
        const double t = clamp01(height / waterLevel);
        name = t < 0.55 ? "Deep Water" : "Shallow Water";
        if (settings.theme == TerrainTheme::Desert) {
            name = t < 0.65 ? "Oasis Water" : "Oasis Shore";
            return lerpColor({30, 96, 124}, {77, 176, 169}, t);
        }
        if (settings.theme == TerrainTheme::Alpine) {
            name = t < 0.65 ? "Glacial Lake" : "Ice Shore";
            return lerpColor({28, 82, 132}, {112, 186, 207}, t);
        }
        return lerpColor(kDeepWater, kShallowWater, t);
    }

    if (settings.theme == TerrainTheme::Desert) {
        if (height < waterLevel + 0.055 && moisture > 0.18) {
            name = "Oasis";
            return {95, 147, 99};
        }
        if (height > 0.78) {
            name = "Mesa";
            return {151, 91, 55};
        }
        if (height > 0.64) {
            name = "Rocky Desert";
            return {177, 122, 68};
        }
        if (moisture > 0.30) {
            name = "Dry Scrub";
            return {164, 151, 84};
        }
        name = "Dunes";
        return moisture < 0.12 ? Color{224, 181, 96} : Color{205, 158, 79};
    }

    if (settings.theme == TerrainTheme::Forest) {
        if (height > 0.80 && temperature < 0.35) {
            name = "Misty Peak";
            return {160, 169, 150};
        }
        if (height > 0.72) {
            name = "Highland Forest";
            return {60, 107, 71};
        }
        if (moisture > 0.78) {
            name = "Rainforest";
            return {25, 105, 70};
        }
        if (moisture > 0.52) {
            name = "Forest";
            return {45, 126, 68};
        }
        name = "Moss Meadow";
        return {103, 164, 84};
    }

    if (settings.theme == TerrainTheme::Alpine) {
        if (height > 0.83 || temperature < 0.16) {
            name = "Glacier";
            return {232, 239, 235};
        }
        if (height > 0.66) {
            name = "Alpine Rock";
            return {118, 123, 120};
        }
        if (moisture > 0.52 && temperature > 0.20) {
            name = "Pine Forest";
            return {45, 105, 82};
        }
        name = "Tundra";
        return {139, 153, 133};
    }

    double blendedR = 0.0;
    double blendedG = 0.0;
    double blendedB = 0.0;
    double totalWeight = 0.0;
    double bestWeight = -1.0;
    const char* bestName = "Land";

    for (const auto& biome : kBiomes) {
        const double elevationDistance = (height - biome.elevation) * 1.25;
        const double temperatureDistance = (temperature - biome.temperature) * 1.05;
        const double moistureDistance = (moisture - biome.moisture) * 1.05;
        const double distance = elevationDistance * elevationDistance
            + temperatureDistance * temperatureDistance
            + moistureDistance * moistureDistance;
        const double weight = 1.0 / (distance + 0.015);

        blendedR += biome.color.r * weight;
        blendedG += biome.color.g * weight;
        blendedB += biome.color.b * weight;
        totalWeight += weight;

        if (weight > bestWeight) {
            bestWeight = weight;
            bestName = biome.name;
        }
    }

    name = bestName;
    return {
        static_cast<uint8_t>(std::clamp(blendedR / totalWeight, 0.0, 255.0)),
        static_cast<uint8_t>(std::clamp(blendedG / totalWeight, 0.0, 255.0)),
        static_cast<uint8_t>(std::clamp(blendedB / totalWeight, 0.0, 255.0)),
    };
}

Color TerrainGenerator::colorForCell(const TerrainCell& cell, const TerrainSettings& settings) {
    switch (settings.viewMode) {
    case ViewMode::Height:
        return grayscale(cell.height);
    case ViewMode::Temperature:
        return temperatureColor(cell.temperature);
    case ViewMode::Moisture:
        return moistureColor(cell.moisture);
    case ViewMode::Biomes:
    default:
        return cell.biomeColor;
    }
}

void TerrainGenerator::paintMapDetails(std::vector<TerrainCell>& cells, std::vector<uint32_t>& pixels, const TerrainSettings& settings) {
    for (int y = 0; y < kMapHeight; ++y) {
        for (int x = 0; x < kMapWidth; ++x) {
            const size_t index = static_cast<size_t>(y) * kMapWidth + x;
            const TerrainCell& cell = cells[index];

            const int left = std::max(0, x - 1);
            const int right = std::min(kMapWidth - 1, x + 1);
            const int up = std::max(0, y - 1);
            const int down = std::min(kMapHeight - 1, y + 1);

            const double dx = cells[static_cast<size_t>(y) * kMapWidth + right].height
                - cells[static_cast<size_t>(y) * kMapWidth + left].height;
            const double dy = cells[static_cast<size_t>(down) * kMapWidth + x].height
                - cells[static_cast<size_t>(up) * kMapWidth + x].height;

            Color color = colorForCell(cell, settings);
            const double hillLight = std::clamp(0.94 + (dy - dx) * 2.4, 0.64, 1.24);
            color = multiplyColor(color, hillLight);

            if (settings.viewMode == ViewMode::Biomes) {
                const double grain = hash01(x / 2, y / 2, settings.seed) - 0.5;
                const double grainStrength = settings.theme == TerrainTheme::Desert ? 0.17 : (settings.theme == TerrainTheme::Forest ? 0.08 : 0.11);
                color = multiplyColor(color, 1.0 + grain * grainStrength);

                if (cell.water) {
                    const bool ripple = (hash01(x / 5, y / 3, settings.seed + 40) > 0.72);
                    if (ripple) {
                        color = blendColor(color, settings.theme == TerrainTheme::Alpine ? Color{174, 224, 232} : Color{111, 177, 202}, 0.12);
                    }
                }

                if (cell.coast) {
                    const Color coast = settings.theme == TerrainTheme::Desert ? Color{237, 211, 143} : (settings.theme == TerrainTheme::Alpine ? Color{221, 242, 244} : kFoam);
                    color = blendColor(color, coast, 0.60);
                }

                if (cell.forestDetail) {
                    const bool darkLeaf = hash01(x, y, settings.seed + 7) > 0.62;
                    const Color dark = settings.theme == TerrainTheme::Alpine ? Color{31, 83, 74} : Color{26, 86, 49};
                    const Color light = settings.theme == TerrainTheme::Forest ? Color{82, 168, 82} : Color{75, 147, 77};
                    color = blendColor(color, darkLeaf ? dark : light, settings.theme == TerrainTheme::Forest ? 0.38 : 0.30);
                }

                if (settings.theme == TerrainTheme::Desert && !cell.water && hash01(x / 3, y / 2, settings.seed + 18) > 0.66) {
                    color = blendColor(color, {241, 204, 122}, 0.18);
                }

                if (cell.mountainDetail) {
                    const bool snowLine = cell.height > 0.82 || cell.temperature < 0.24;
                    const Color rock = settings.theme == TerrainTheme::Desert ? Color{126, 70, 48} : Color{77, 73, 69};
                    const Color snow = settings.theme == TerrainTheme::Alpine ? Color{246, 250, 247} : Color{239, 241, 231};
                    color = blendColor(color, snowLine ? snow : rock, snowLine ? 0.42 : 0.28);
                }
            }

            pixels[index] = packColor(color);
        }
    }

    if (settings.viewMode != ViewMode::Biomes) {
        return;
    }

    std::vector<uint32_t> layered = pixels;
    auto setPixel = [&](int x, int y, Color color) {
        if (x < 0 || x >= kMapWidth || y < 0 || y >= kMapHeight) {
            return;
        }
        layered[static_cast<size_t>(y) * kMapWidth + x] = packColor(color);
    };

    for (int y = 1; y < kMapHeight - 1; ++y) {
        for (int x = 1; x < kMapWidth - 1; ++x) {
            const TerrainCell& cell = cells[static_cast<size_t>(y) * kMapWidth + x];

            if (cell.river) {
                setPixel(x, y, kRiver);
                setPixel(x + 1, y, kRiver);
                setPixel(x, y + 1, blendColor(kRiver, {215, 238, 238}, 0.22));
            }

            if (cell.road && ((x + y) % 3 != 0)) {
                setPixel(x, y, kRoad);
                if ((x + y) % 5 == 0) {
                    setPixel(x, y + 1, kRoadLight);
                }
            }

            if (cell.coast && hash01(x, y, settings.seed + 11) > 0.40) {
                setPixel(x, y, kFoam);
            }
        }
    }

    pixels.swap(layered);
}

void TerrainGenerator::createFeatureMarkers(const std::vector<TerrainCell>& cells, const TerrainSettings& settings, std::vector<MapFeature>& features) {
    if (settings.viewMode != ViewMode::Biomes) {
        return;
    }

    const int grid = 96;
    const char** names = kSettlementNames;
    size_t nameCount = sizeof(kSettlementNames) / sizeof(kSettlementNames[0]);
    Color markerColor{236, 220, 161};

    if (settings.theme == TerrainTheme::Desert) {
        names = kDesertNames;
        nameCount = sizeof(kDesertNames) / sizeof(kDesertNames[0]);
        markerColor = {245, 199, 108};
    } else if (settings.theme == TerrainTheme::Forest) {
        names = kForestNames;
        nameCount = sizeof(kForestNames) / sizeof(kForestNames[0]);
        markerColor = {153, 222, 139};
    } else if (settings.theme == TerrainTheme::Alpine) {
        names = kAlpineNames;
        nameCount = sizeof(kAlpineNames) / sizeof(kAlpineNames[0]);
        markerColor = {225, 241, 244};
    }

    int nameIndex = std::abs(settings.seed) % static_cast<int>(nameCount);
    const double waterLevel = themeWaterLevel(settings);

    for (int gy = grid / 2; gy < kMapHeight; gy += grid) {
        for (int gx = grid / 2; gx < kMapWidth; gx += grid) {
            int bestX = gx;
            int bestY = gy;
            double bestScore = -1.0;

            for (int y = std::max(8, gy - 28); y < std::min(kMapHeight - 8, gy + 28); y += 4) {
                for (int x = std::max(8, gx - 28); x < std::min(kMapWidth - 8, gx + 28); x += 4) {
                    const TerrainCell& cell = cells[static_cast<size_t>(y) * kMapWidth + x];
                    if (cell.water || cell.height < waterLevel + 0.04 || cell.slope > (settings.theme == TerrainTheme::Alpine ? 0.032 : 0.020)) {
                        continue;
                    }
                    if (settings.theme != TerrainTheme::Alpine && cell.height > 0.72) {
                        continue;
                    }

                    bool nearWater = false;
                    for (int oy = -10; oy <= 10; oy += 5) {
                        for (int ox = -10; ox <= 10; ox += 5) {
                            const int nx = std::clamp(x + ox, 0, kMapWidth - 1);
                            const int ny = std::clamp(y + oy, 0, kMapHeight - 1);
                            nearWater = nearWater || cells[static_cast<size_t>(ny) * kMapWidth + nx].water || cells[static_cast<size_t>(ny) * kMapWidth + nx].river;
                        }
                    }

                    double score = (nearWater ? 0.35 : 0.0)
                        + (cell.road ? 0.28 : 0.0)
                        + (1.0 - std::abs(cell.moisture - 0.55)) * 0.18
                        + hash01(x, y, settings.seed + 90) * 0.24;

                    if (settings.theme == TerrainTheme::Desert) {
                        score += (cell.biomeName == "Oasis" ? 0.34 : 0.0)
                            + (cell.biomeName == "Mesa" ? 0.16 : 0.0)
                            + (1.0 - cell.moisture) * 0.08;
                    } else if (settings.theme == TerrainTheme::Forest) {
                        score += (isForestBiome(cell.biomeName) ? 0.25 : 0.0)
                            + cell.moisture * 0.16;
                    } else if (settings.theme == TerrainTheme::Alpine) {
                        score += (isMountainBiome(cell.biomeName) ? 0.30 : 0.0)
                            + cell.height * 0.20;
                    }

                    if (score > bestScore) {
                        bestScore = score;
                        bestX = x;
                        bestY = y;
                    }
                }
            }

            if (bestScore > 0.48 && features.size() < 9) {
                features.push_back({
                    bestX,
                    bestY,
                    names[nameIndex % nameCount],
                    markerColor,
                    settings.theme
                });
                ++nameIndex;
            }
        }
    }
}

Color lerpColor(Color a, Color b, double t) {
    t = std::clamp(t, 0.0, 1.0);
    return {
        static_cast<uint8_t>(a.r + (b.r - a.r) * t),
        static_cast<uint8_t>(a.g + (b.g - a.g) * t),
        static_cast<uint8_t>(a.b + (b.b - a.b) * t)
    };
}

Color blendColor(Color base, Color overlay, double amount) {
    return lerpColor(base, overlay, amount);
}

uint32_t packColor(Color color) {
    return (static_cast<uint32_t>(color.r) << 16)
        | (static_cast<uint32_t>(color.g) << 8)
        | static_cast<uint32_t>(color.b);
}

const char* viewModeName(ViewMode mode) {
    switch (mode) {
    case ViewMode::Height: return "Height";
    case ViewMode::Temperature: return "Temperature";
    case ViewMode::Moisture: return "Moisture";
    case ViewMode::Biomes:
    default: return "Game Map";
    }
}

const char* terrainThemeName(TerrainTheme theme) {
    switch (theme) {
    case TerrainTheme::Desert: return "Desert";
    case TerrainTheme::Forest: return "Forest";
    case TerrainTheme::Alpine: return "Alpine";
    case TerrainTheme::Mixed:
    default: return "Mixed";
    }
}

const char* terrainThemeTagline(TerrainTheme theme) {
    switch (theme) {
    case TerrainTheme::Desert:
        return "Dunes, mesas, dry trails, and rare oases";
    case TerrainTheme::Forest:
        return "Dense canopy, wet valleys, rivers, and groves";
    case TerrainTheme::Alpine:
        return "Snowy ridges, glacial lakes, pines, and passes";
    case TerrainTheme::Mixed:
    default:
        return "Balanced biomes with varied climates and settlements";
    }
}
