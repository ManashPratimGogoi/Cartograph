#include "WorldStore.h"

#include "TerrainService.h"

#include <sqlite3.h>

namespace store {
namespace {

const char* kSchema =
    "CREATE TABLE IF NOT EXISTS worlds ("
    "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "  name TEXT NOT NULL,"
    "  seed INTEGER NOT NULL,"
    "  frequency REAL NOT NULL,"
    "  octaves INTEGER NOT NULL,"
    "  persistence REAL NOT NULL,"
    "  lacunarity REAL NOT NULL,"
    "  water_level REAL NOT NULL,"
    "  offset_x REAL NOT NULL,"
    "  offset_y REAL NOT NULL,"
    "  theme TEXT NOT NULL,"
    "  view_mode TEXT NOT NULL,"
    "  created_at TEXT NOT NULL DEFAULT (datetime('now'))"
    ");";

SavedWorld readRow(sqlite3_stmt* stmt) {
    SavedWorld world;
    world.id = sqlite3_column_int64(stmt, 0);
    world.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    world.settings.seed = sqlite3_column_int(stmt, 2);
    world.settings.frequency = sqlite3_column_double(stmt, 3);
    world.settings.octaves = sqlite3_column_int(stmt, 4);
    world.settings.persistence = sqlite3_column_double(stmt, 5);
    world.settings.lacunarity = sqlite3_column_double(stmt, 6);
    world.settings.waterLevel = sqlite3_column_double(stmt, 7);
    world.settings.offsetX = sqlite3_column_double(stmt, 8);
    world.settings.offsetY = sqlite3_column_double(stmt, 9);
    world.settings.theme =
        service::themeFromString(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 10)));
    world.settings.viewMode =
        service::viewModeFromString(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 11)));
    const unsigned char* created = sqlite3_column_text(stmt, 12);
    world.createdAt = created ? reinterpret_cast<const char*>(created) : "";
    return world;
}

const char* kSelectColumns =
    "id, name, seed, frequency, octaves, persistence, lacunarity, "
    "water_level, offset_x, offset_y, theme, view_mode, created_at";

} // namespace

WorldStore::~WorldStore() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool WorldStore::open(const std::string& path) {
    if (sqlite3_open(path.c_str(), &db_) != SQLITE_OK) {
        lastError_ = db_ ? sqlite3_errmsg(db_) : "unable to open database";
        return false;
    }

    char* errorMessage = nullptr;
    if (sqlite3_exec(db_, kSchema, nullptr, nullptr, &errorMessage) != SQLITE_OK) {
        lastError_ = errorMessage ? errorMessage : "schema creation failed";
        sqlite3_free(errorMessage);
        return false;
    }

    return true;
}

std::vector<SavedWorld> WorldStore::listWorlds() {
    std::vector<SavedWorld> worlds;
    const std::string sql =
        "SELECT " + std::string(kSelectColumns) + " FROM worlds ORDER BY datetime(created_at) DESC, id DESC;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        lastError_ = sqlite3_errmsg(db_);
        return worlds;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        worlds.push_back(readRow(stmt));
    }
    sqlite3_finalize(stmt);
    return worlds;
}

bool WorldStore::getWorld(long long id, SavedWorld& out) {
    const std::string sql =
        "SELECT " + std::string(kSelectColumns) + " FROM worlds WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        lastError_ = sqlite3_errmsg(db_);
        return false;
    }
    sqlite3_bind_int64(stmt, 1, id);

    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        out = readRow(stmt);
        found = true;
    }
    sqlite3_finalize(stmt);
    return found;
}

long long WorldStore::saveWorld(const std::string& name, const TerrainSettings& settings) {
    const char* sql =
        "INSERT INTO worlds "
        "(name, seed, frequency, octaves, persistence, lacunarity, water_level, "
        " offset_x, offset_y, theme, view_mode) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        lastError_ = sqlite3_errmsg(db_);
        return -1;
    }

    const std::string theme = service::themeToString(settings.theme);
    const std::string viewMode = service::viewModeToString(settings.viewMode);

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, settings.seed);
    sqlite3_bind_double(stmt, 3, settings.frequency);
    sqlite3_bind_int(stmt, 4, settings.octaves);
    sqlite3_bind_double(stmt, 5, settings.persistence);
    sqlite3_bind_double(stmt, 6, settings.lacunarity);
    sqlite3_bind_double(stmt, 7, settings.waterLevel);
    sqlite3_bind_double(stmt, 8, settings.offsetX);
    sqlite3_bind_double(stmt, 9, settings.offsetY);
    sqlite3_bind_text(stmt, 10, theme.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 11, viewMode.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        lastError_ = sqlite3_errmsg(db_);
        sqlite3_finalize(stmt);
        return -1;
    }
    sqlite3_finalize(stmt);
    return sqlite3_last_insert_rowid(db_);
}

bool WorldStore::deleteWorld(long long id) {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, "DELETE FROM worlds WHERE id = ?;", -1, &stmt, nullptr) != SQLITE_OK) {
        lastError_ = sqlite3_errmsg(db_);
        return false;
    }
    sqlite3_bind_int64(stmt, 1, id);
    const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    if (!ok) {
        lastError_ = sqlite3_errmsg(db_);
    }
    sqlite3_finalize(stmt);
    return ok && sqlite3_changes(db_) > 0;
}

} // namespace store
