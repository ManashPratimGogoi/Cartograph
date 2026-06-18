#pragma once

// TerrainService wraps the existing C++ terrain engine (engine/Noise, engine/Terrain)
// and turns its output into transport-ready data for the web API:
//   - a base64-encoded raw RGB buffer the browser renders onto a <canvas>
//   - world statistics (water/land split, biome distribution, settlement count)
//   - feature markers in pixel coordinates
//
// The engine stays the single source of truth. Nothing here re-implements
// generation; it only serializes what the engine produces.

#include "../engine/Terrain.h"

#include <string>
#include <vector>

namespace service {

struct BiomeShare {
    std::string name;
    double percent = 0.0;
};

struct WorldStats {
    double waterPercent = 0.0;
    double landPercent = 0.0;
    double minHeight = 0.0;
    double maxHeight = 0.0;
    double meanTemperature = 0.0;
    double meanMoisture = 0.0;
    int settlementCount = 0;
    int riverCells = 0;
    std::vector<BiomeShare> biomes; // sorted, largest share first
};

// A downsampled grid of per-cell readings so the browser can show a live
// cursor readout (biome + height/temperature/moisture) without shipping the
// full 768x512 cell array on every request.
struct ReadingGrid {
    int step = 6;
    int width = 0;
    int height = 0;
    std::string heightB64;   // width * height bytes, height quantized 0-255
    std::string tempB64;     // width * height bytes, temperature quantized 0-255
    std::string moistB64;    // width * height bytes, moisture quantized 0-255
    std::string biomeB64;    // width * height bytes, index into biomeNames
    std::vector<std::string> biomeNames;
};

struct GeneratedWorld {
    int width = kMapWidth;
    int height = kMapHeight;
    std::string viewMode;
    std::string pixelsBase64; // width * height * 3 bytes (RGB), base64 encoded
    WorldStats stats;
    ReadingGrid grid;
    std::vector<MapFeature> features;
    TerrainSettings settings;
};

// Parse a TerrainSettings struct from incoming JSON-ish key/values.
TerrainSettings settingsFromJson(const std::string& jsonBody);

// Run the engine for the given settings and package the result.
GeneratedWorld generateWorld(const TerrainSettings& settings);

// Serialize a GeneratedWorld to a JSON string for the HTTP response.
std::string worldToJson(const GeneratedWorld& world);

// Helpers shared with the server layer.
std::string viewModeToString(ViewMode mode);
ViewMode viewModeFromString(const std::string& value);
std::string themeToString(TerrainTheme theme);
TerrainTheme themeFromString(const std::string& value);

} // namespace service
