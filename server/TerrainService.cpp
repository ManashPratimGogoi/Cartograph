#include "TerrainService.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

using nlohmann::json;

namespace service {
namespace {

const char kBase64Alphabet[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// Encode a raw byte buffer to standard base64.
std::string encodeBase64(const std::vector<std::uint8_t>& data) {
    std::string out;
    out.reserve(((data.size() + 2) / 3) * 4);

    std::size_t i = 0;
    for (; i + 2 < data.size(); i += 3) {
        const std::uint32_t triple =
            (static_cast<std::uint32_t>(data[i]) << 16) |
            (static_cast<std::uint32_t>(data[i + 1]) << 8) |
            static_cast<std::uint32_t>(data[i + 2]);
        out.push_back(kBase64Alphabet[(triple >> 18) & 0x3F]);
        out.push_back(kBase64Alphabet[(triple >> 12) & 0x3F]);
        out.push_back(kBase64Alphabet[(triple >> 6) & 0x3F]);
        out.push_back(kBase64Alphabet[triple & 0x3F]);
    }

    const std::size_t remaining = data.size() - i;
    if (remaining == 1) {
        const std::uint32_t triple = static_cast<std::uint32_t>(data[i]) << 16;
        out.push_back(kBase64Alphabet[(triple >> 18) & 0x3F]);
        out.push_back(kBase64Alphabet[(triple >> 12) & 0x3F]);
        out.push_back('=');
        out.push_back('=');
    } else if (remaining == 2) {
        const std::uint32_t triple =
            (static_cast<std::uint32_t>(data[i]) << 16) |
            (static_cast<std::uint32_t>(data[i + 1]) << 8);
        out.push_back(kBase64Alphabet[(triple >> 18) & 0x3F]);
        out.push_back(kBase64Alphabet[(triple >> 12) & 0x3F]);
        out.push_back(kBase64Alphabet[(triple >> 6) & 0x3F]);
        out.push_back('=');
    }

    return out;
}

// Pack the engine's 0x00RRGGBB pixel buffer into a tight RGB byte stream.
std::vector<std::uint8_t> packRgb(const std::vector<std::uint32_t>& pixels) {
    std::vector<std::uint8_t> rgb;
    rgb.reserve(pixels.size() * 3);
    for (std::uint32_t packed : pixels) {
        rgb.push_back(static_cast<std::uint8_t>((packed >> 16) & 0xFF));
        rgb.push_back(static_cast<std::uint8_t>((packed >> 8) & 0xFF));
        rgb.push_back(static_cast<std::uint8_t>(packed & 0xFF));
    }
    return rgb;
}

double clampDouble(double value, double lo, double hi) {
    return std::max(lo, std::min(hi, value));
}

WorldStats computeStats(const std::vector<TerrainCell>& cells,
                        const std::vector<MapFeature>& features) {
    WorldStats stats;
    if (cells.empty()) {
        return stats;
    }

    std::size_t waterCells = 0;
    double temperatureSum = 0.0;
    double moistureSum = 0.0;
    double minHeight = 1.0;
    double maxHeight = 0.0;
    std::map<std::string, std::size_t> biomeCounts;

    for (const TerrainCell& cell : cells) {
        if (cell.water) {
            ++waterCells;
        }
        if (cell.river) {
            ++stats.riverCells;
        }
        temperatureSum += cell.temperature;
        moistureSum += cell.moisture;
        minHeight = std::min(minHeight, cell.height);
        maxHeight = std::max(maxHeight, cell.height);
        ++biomeCounts[cell.biomeName];
    }

    const double total = static_cast<double>(cells.size());
    stats.waterPercent = static_cast<double>(waterCells) / total * 100.0;
    stats.landPercent = 100.0 - stats.waterPercent;
    stats.minHeight = minHeight;
    stats.maxHeight = maxHeight;
    stats.meanTemperature = temperatureSum / total;
    stats.meanMoisture = moistureSum / total;
    stats.settlementCount = static_cast<int>(features.size());

    for (const auto& entry : biomeCounts) {
        stats.biomes.push_back({entry.first,
                                static_cast<double>(entry.second) / total * 100.0});
    }
    std::sort(stats.biomes.begin(), stats.biomes.end(),
              [](const BiomeShare& a, const BiomeShare& b) {
                  return a.percent > b.percent;
              });
    if (stats.biomes.size() > 8) {
        stats.biomes.resize(8);
    }

    return stats;
}

std::uint8_t quantize(double value) {
    return static_cast<std::uint8_t>(std::max(0.0, std::min(255.0, value * 255.0)));
}

// Build a downsampled grid of readings for the live cursor readout.
ReadingGrid buildGrid(const std::vector<TerrainCell>& cells) {
    ReadingGrid grid;
    grid.step = 6;
    grid.width = (kMapWidth + grid.step - 1) / grid.step;
    grid.height = (kMapHeight + grid.step - 1) / grid.step;

    std::vector<std::uint8_t> heightBytes;
    std::vector<std::uint8_t> tempBytes;
    std::vector<std::uint8_t> moistBytes;
    std::vector<std::uint8_t> biomeBytes;
    const std::size_t count = static_cast<std::size_t>(grid.width) * grid.height;
    heightBytes.reserve(count);
    tempBytes.reserve(count);
    moistBytes.reserve(count);
    biomeBytes.reserve(count);

    std::map<std::string, int> biomeIndex;

    for (int gy = 0; gy < grid.height; ++gy) {
        for (int gx = 0; gx < grid.width; ++gx) {
            const int sx = std::min(gx * grid.step, kMapWidth - 1);
            const int sy = std::min(gy * grid.step, kMapHeight - 1);
            const TerrainCell& cell = cells[static_cast<std::size_t>(sy) * kMapWidth + sx];

            heightBytes.push_back(quantize(cell.height));
            tempBytes.push_back(quantize(cell.temperature));
            moistBytes.push_back(quantize(cell.moisture));

            auto found = biomeIndex.find(cell.biomeName);
            int index;
            if (found == biomeIndex.end()) {
                index = static_cast<int>(grid.biomeNames.size());
                biomeIndex.emplace(cell.biomeName, index);
                grid.biomeNames.push_back(cell.biomeName);
            } else {
                index = found->second;
            }
            biomeBytes.push_back(static_cast<std::uint8_t>(std::min(index, 255)));
        }
    }

    grid.heightB64 = encodeBase64(heightBytes);
    grid.tempB64 = encodeBase64(tempBytes);
    grid.moistB64 = encodeBase64(moistBytes);
    grid.biomeB64 = encodeBase64(biomeBytes);
    return grid;
}

} // namespace

std::string viewModeToString(ViewMode mode) {
    switch (mode) {
    case ViewMode::Height: return "height";
    case ViewMode::Temperature: return "temperature";
    case ViewMode::Moisture: return "moisture";
    case ViewMode::Biomes:
    default: return "biomes";
    }
}

ViewMode viewModeFromString(const std::string& value) {
    if (value == "height") return ViewMode::Height;
    if (value == "temperature") return ViewMode::Temperature;
    if (value == "moisture") return ViewMode::Moisture;
    return ViewMode::Biomes;
}

std::string themeToString(TerrainTheme theme) {
    switch (theme) {
    case TerrainTheme::Desert: return "desert";
    case TerrainTheme::Forest: return "forest";
    case TerrainTheme::Alpine: return "alpine";
    case TerrainTheme::Mixed:
    default: return "mixed";
    }
}

TerrainTheme themeFromString(const std::string& value) {
    if (value == "desert") return TerrainTheme::Desert;
    if (value == "forest") return TerrainTheme::Forest;
    if (value == "alpine") return TerrainTheme::Alpine;
    return TerrainTheme::Mixed;
}

TerrainSettings settingsFromJson(const std::string& jsonBody) {
    TerrainSettings settings;
    if (jsonBody.empty()) {
        return settings;
    }

    json parsed = json::parse(jsonBody, nullptr, false);
    if (parsed.is_discarded() || !parsed.is_object()) {
        return settings;
    }

    if (parsed.contains("seed") && parsed["seed"].is_number()) {
        settings.seed = parsed["seed"].get<int>();
    }
    if (parsed.contains("frequency") && parsed["frequency"].is_number()) {
        settings.frequency = clampDouble(parsed["frequency"].get<double>(), 0.45, 3.20);
    }
    if (parsed.contains("octaves") && parsed["octaves"].is_number()) {
        settings.octaves = std::max(1, std::min(8, parsed["octaves"].get<int>()));
    }
    if (parsed.contains("persistence") && parsed["persistence"].is_number()) {
        settings.persistence = clampDouble(parsed["persistence"].get<double>(), 0.25, 0.80);
    }
    if (parsed.contains("lacunarity") && parsed["lacunarity"].is_number()) {
        settings.lacunarity = clampDouble(parsed["lacunarity"].get<double>(), 1.5, 3.0);
    }
    if (parsed.contains("waterLevel") && parsed["waterLevel"].is_number()) {
        settings.waterLevel = clampDouble(parsed["waterLevel"].get<double>(), 0.20, 0.55);
    }
    if (parsed.contains("offsetX") && parsed["offsetX"].is_number()) {
        settings.offsetX = parsed["offsetX"].get<double>();
    }
    if (parsed.contains("offsetY") && parsed["offsetY"].is_number()) {
        settings.offsetY = parsed["offsetY"].get<double>();
    }
    if (parsed.contains("theme") && parsed["theme"].is_string()) {
        settings.theme = themeFromString(parsed["theme"].get<std::string>());
    }
    if (parsed.contains("viewMode") && parsed["viewMode"].is_string()) {
        settings.viewMode = viewModeFromString(parsed["viewMode"].get<std::string>());
    }

    return settings;
}

GeneratedWorld generateWorld(const TerrainSettings& settings) {
    TerrainGenerator generator(settings);

    std::vector<TerrainCell> cells(static_cast<std::size_t>(kMapWidth) * kMapHeight);
    std::vector<std::uint32_t> pixels(static_cast<std::size_t>(kMapWidth) * kMapHeight, 0);
    std::vector<MapFeature> features;

    generator.generate(settings, cells, pixels, features);

    GeneratedWorld world;
    world.width = kMapWidth;
    world.height = kMapHeight;
    world.viewMode = viewModeToString(settings.viewMode);
    world.pixelsBase64 = encodeBase64(packRgb(pixels));
    world.stats = computeStats(cells, features);
    world.grid = buildGrid(cells);
    world.features = features;
    world.settings = settings;
    return world;
}

std::string worldToJson(const GeneratedWorld& world) {
    json out;
    out["width"] = world.width;
    out["height"] = world.height;
    out["viewMode"] = world.viewMode;
    out["pixels"] = world.pixelsBase64;

    json settings;
    settings["seed"] = world.settings.seed;
    settings["frequency"] = world.settings.frequency;
    settings["octaves"] = world.settings.octaves;
    settings["persistence"] = world.settings.persistence;
    settings["lacunarity"] = world.settings.lacunarity;
    settings["waterLevel"] = world.settings.waterLevel;
    settings["offsetX"] = world.settings.offsetX;
    settings["offsetY"] = world.settings.offsetY;
    settings["theme"] = themeToString(world.settings.theme);
    settings["viewMode"] = viewModeToString(world.settings.viewMode);
    out["settings"] = settings;

    json stats;
    stats["waterPercent"] = world.stats.waterPercent;
    stats["landPercent"] = world.stats.landPercent;
    stats["minHeight"] = world.stats.minHeight;
    stats["maxHeight"] = world.stats.maxHeight;
    stats["meanTemperature"] = world.stats.meanTemperature;
    stats["meanMoisture"] = world.stats.meanMoisture;
    stats["settlementCount"] = world.stats.settlementCount;
    stats["riverCells"] = world.stats.riverCells;
    json biomes = json::array();
    for (const BiomeShare& share : world.stats.biomes) {
        biomes.push_back({{"name", share.name}, {"percent", share.percent}});
    }
    stats["biomes"] = biomes;
    out["stats"] = stats;

    json grid;
    grid["step"] = world.grid.step;
    grid["width"] = world.grid.width;
    grid["height"] = world.grid.height;
    grid["heightData"] = world.grid.heightB64;
    grid["tempData"] = world.grid.tempB64;
    grid["moistData"] = world.grid.moistB64;
    grid["biomeData"] = world.grid.biomeB64;
    grid["biomeNames"] = world.grid.biomeNames;
    out["grid"] = grid;

    json features = json::array();
    for (const MapFeature& feature : world.features) {
        features.push_back({
            {"x", feature.x},
            {"y", feature.y},
            {"name", feature.name},
            {"theme", themeToString(feature.theme)},
        });
    }
    out["features"] = features;

    return out.dump();
}

} // namespace service
