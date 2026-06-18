// Crow HTTP API for the procedural terrain engine.
//
// Endpoints:
//   GET    /api/health            -> { status: "ok" }
//   POST   /api/generate          -> generate a world from settings (JSON body)
//   GET    /api/worlds            -> list saved worlds
//   POST   /api/worlds            -> save a world { name, settings... }
//   GET    /api/worlds/<id>       -> fetch one saved world (settings)
//   DELETE /api/worlds/<id>       -> delete a saved world
//
// The C++ terrain engine (engine/Noise, engine/Terrain) is the single source of
// truth; this layer only translates HTTP <-> engine and persists settings in
// SQLite.

#include "TerrainService.h"
#include "WorldStore.h"

#include "third_party/crow_all.h"
#include <nlohmann/json.hpp>

#include <string>

using nlohmann::json;

namespace {

constexpr int kPort = 18080;
const char* kDatabasePath = "worlds.db";

// CORS middleware so the Vite dev server (a different origin) can call the API.
// In dev, Vite proxies /api so preflight is rarely hit; in production the
// frontend may be a separate origin, so we answer OPTIONS preflight directly.
struct Cors {
    struct context {};

    void before_handle(crow::request& req, crow::response& res, context&) {
        if (req.method == crow::HTTPMethod::Options) {
            res.code = 204;
            res.add_header("Access-Control-Allow-Origin", "*");
            res.add_header("Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS");
            res.add_header("Access-Control-Allow-Headers", "Content-Type");
            res.end();
        }
    }

    void after_handle(crow::request&, crow::response& res, context&) {
        res.add_header("Access-Control-Allow-Origin", "*");
        res.add_header("Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS");
        res.add_header("Access-Control-Allow-Headers", "Content-Type");
    }
};

crow::response jsonResponse(int code, const std::string& body) {
    crow::response res(code, body);
    res.set_header("Content-Type", "application/json");
    return res;
}

crow::response errorResponse(int code, const std::string& message) {
    json error;
    error["error"] = message;
    return jsonResponse(code, error.dump());
}

} // namespace

int main() {
    store::WorldStore worldStore;
    if (!worldStore.open(kDatabasePath)) {
        CROW_LOG_ERROR << "Failed to open database: " << worldStore.lastError();
        return 1;
    }

    crow::App<Cors> app;

    CROW_ROUTE(app, "/api/health")
    ([] {
        return jsonResponse(200, R"({"status":"ok"})");
    });

    // Generate a world from posted settings.
    CROW_ROUTE(app, "/api/generate").methods(crow::HTTPMethod::Post)
    ([](const crow::request& req) {
        try {
            const TerrainSettings settings = service::settingsFromJson(req.body);
            const service::GeneratedWorld world = service::generateWorld(settings);
            return jsonResponse(200, service::worldToJson(world));
        } catch (const std::exception& ex) {
            return errorResponse(400, std::string("generation failed: ") + ex.what());
        }
    });

    // List saved worlds.
    CROW_ROUTE(app, "/api/worlds").methods(crow::HTTPMethod::Get)
    ([&worldStore]() {
        json list = json::array();
        for (const store::SavedWorld& world : worldStore.listWorlds()) {
            list.push_back({
                {"id", world.id},
                {"name", world.name},
                {"createdAt", world.createdAt},
                {"seed", world.settings.seed},
                {"theme", service::themeToString(world.settings.theme)},
            });
        }
        return jsonResponse(200, list.dump());
    });

    // Save a world: { "name": "...", "settings": { ... } } or flat settings + name.
    CROW_ROUTE(app, "/api/worlds").methods(crow::HTTPMethod::Post)
    ([&worldStore](const crow::request& req) {
        json parsed = json::parse(req.body, nullptr, false);
        if (parsed.is_discarded() || !parsed.is_object()) {
            return errorResponse(400, "invalid JSON body");
        }

        std::string name = "Untitled World";
        if (parsed.contains("name") && parsed["name"].is_string()) {
            name = parsed["name"].get<std::string>();
        }
        if (name.empty()) {
            return errorResponse(400, "name is required");
        }

        // Settings may be nested under "settings" or sent at the top level.
        std::string settingsBody = req.body;
        if (parsed.contains("settings") && parsed["settings"].is_object()) {
            settingsBody = parsed["settings"].dump();
        }

        const TerrainSettings settings = service::settingsFromJson(settingsBody);
        const long long id = worldStore.saveWorld(name, settings);
        if (id < 0) {
            return errorResponse(500, "could not save world: " + worldStore.lastError());
        }

        json result;
        result["id"] = id;
        result["name"] = name;
        return jsonResponse(201, result.dump());
    });

    // Fetch one saved world's settings.
    CROW_ROUTE(app, "/api/worlds/<int>").methods(crow::HTTPMethod::Get)
    ([&worldStore](int id) {
        store::SavedWorld world;
        if (!worldStore.getWorld(id, world)) {
            return errorResponse(404, "world not found");
        }

        json result;
        result["id"] = world.id;
        result["name"] = world.name;
        result["createdAt"] = world.createdAt;
        json settings;
        settings["seed"] = world.settings.seed;
        settings["frequency"] = world.settings.frequency;
        settings["octaves"] = world.settings.octaves;
        settings["persistence"] = world.settings.persistence;
        settings["lacunarity"] = world.settings.lacunarity;
        settings["waterLevel"] = world.settings.waterLevel;
        settings["offsetX"] = world.settings.offsetX;
        settings["offsetY"] = world.settings.offsetY;
        settings["theme"] = service::themeToString(world.settings.theme);
        settings["viewMode"] = service::viewModeToString(world.settings.viewMode);
        result["settings"] = settings;
        return jsonResponse(200, result.dump());
    });

    // Delete a saved world.
    CROW_ROUTE(app, "/api/worlds/<int>").methods(crow::HTTPMethod::Delete)
    ([&worldStore](int id) {
        if (!worldStore.deleteWorld(id)) {
            return errorResponse(404, "world not found");
        }
        return jsonResponse(200, R"({"deleted":true})");
    });

    CROW_LOG_INFO << "Terrain API listening on http://localhost:" << kPort;
    app.port(kPort).multithreaded().run();
    return 0;
}
