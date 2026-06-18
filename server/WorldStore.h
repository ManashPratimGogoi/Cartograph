#pragma once

// WorldStore is a thin SQLite-backed repository for saved worlds. Because the
// terrain engine is fully deterministic from its settings, a saved world is
// just a name plus the TerrainSettings that reproduce it exactly. No images
// are stored.

#include "../engine/Terrain.h"

#include <string>
#include <vector>

struct sqlite3;

namespace store {

struct SavedWorld {
    long long id = 0;
    std::string name;
    std::string createdAt;
    TerrainSettings settings;
};

class WorldStore {
public:
    WorldStore() = default;
    ~WorldStore();

    WorldStore(const WorldStore&) = delete;
    WorldStore& operator=(const WorldStore&) = delete;

    // Open (and create if needed) the database file and ensure the schema.
    bool open(const std::string& path);

    std::vector<SavedWorld> listWorlds();
    bool getWorld(long long id, SavedWorld& out);
    long long saveWorld(const std::string& name, const TerrainSettings& settings);
    bool deleteWorld(long long id);

    const std::string& lastError() const { return lastError_; }

private:
    sqlite3* db_ = nullptr;
    std::string lastError_;
};

} // namespace store
