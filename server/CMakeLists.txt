cmake_minimum_required(VERSION 3.16)
project(TerrainServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Reuse the existing terrain engine as a static library.
add_library(terrain_engine STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../engine/Noise.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../engine/Terrain.cpp
)
target_include_directories(terrain_engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../engine
)

add_executable(TerrainServer
    main.cpp
    TerrainService.cpp
    WorldStore.cpp
)

# Dependencies. On MSYS2 UCRT64 install:
#   pacman -S mingw-w64-ucrt-x86_64-crow mingw-w64-ucrt-x86_64-asio \
#            mingw-w64-ucrt-x86_64-nlohmann-json mingw-w64-ucrt-x86_64-sqlite3
find_package(Crow QUIET)
find_package(nlohmann_json QUIET)

find_path(SQLITE3_INCLUDE_DIR sqlite3.h)
find_library(SQLITE3_LIBRARY NAMES sqlite3)

target_link_libraries(TerrainServer PRIVATE terrain_engine ${SQLITE3_LIBRARY})
target_include_directories(TerrainServer PRIVATE ${SQLITE3_INCLUDE_DIR})

if(Crow_FOUND)
    target_link_libraries(TerrainServer PRIVATE Crow::Crow)
endif()
if(nlohmann_json_FOUND)
    target_link_libraries(TerrainServer PRIVATE nlohmann_json::nlohmann_json)
endif()

if(WIN32)
    target_link_libraries(TerrainServer PRIVATE ws2_32 wsock32)
endif()
