# Interactive Procedural Terrain Generation — Full-Stack

A procedural world generator built around a C++ noise-based terrain engine. The
engine produces reproducible game-style maps from a numeric seed using fractal
Perlin noise, temperature and moisture maps, biome classification, and smooth
biome blending. It is exposed through a Crow HTTP API, persists worlds in
SQLite, and is explored through a React frontend styled as a cartographer's
survey console.

> The project began as an SFML desktop app. That desktop UI has been retired;
> the same generation engine now powers the web stack. See **`FULLSTACK.md`**
> for the architecture, API reference, and full build/run instructions.

## Stack

| Layer    | Technology                                   |
| -------- | -------------------------------------------- |
| Engine   | C++17 (`engine/Noise.*`, `engine/Terrain.*`) |
| API      | Crow (single-header) + nlohmann/json         |
| Database | SQLite (saved worlds = seed + settings)      |
| Frontend | React + Vite + TypeScript                    |

## Engine features

- Fractal Perlin noise heightmap generation
- Temperature and moisture map generation
- Rule-based biome classification with smooth color blending
- Game-style detail: coastlines, rivers, trails, forest/mountain texture, and
  named settlements
- Environment presets: Mixed, Desert, Forest, Alpine
- Reproducible worlds from a numeric seed
- Visualization layers: biome atlas, height, temperature, moisture

## Quick start

Requires the MSYS2 UCRT64 toolchain (built with GCC 16.1.0) and Node.js.

```powershell
# 1. API server
cd server
.\build_server.bat
.\run_server.bat          # http://localhost:18080

# 2. Frontend (separate terminal)
cd web
npm install
npm run dev               # http://localhost:5173
```

Open http://localhost:5173 with the server running.

Full setup, dependencies, and API documentation are in
[`FULLSTACK.md`](FULLSTACK.md).

## Synopsis alignment

| Synopsis item                              | Implementation                                       |
| ------------------------------------------ | ---------------------------------------------------- |
| 2D terrain visualization                   | React canvas survey plate                            |
| Perlin/Simplex noise                       | Fractal Perlin noise (`engine/Noise.cpp`)            |
| Heightmap generation                       | `TerrainCell::height` per pixel                      |
| Temperature and moisture maps              | `TerrainCell::temperature` / `::moisture`            |
| Biome classification                       | Rule-based profiles in `engine/Terrain.cpp`          |
| Smooth biome blending                      | Weighted interpolation across biome profiles         |
| Frequency, octaves, water level, seed      | Interactive controls + REST settings                 |
| Save/export                                | SQLite-backed world atlas                            |
