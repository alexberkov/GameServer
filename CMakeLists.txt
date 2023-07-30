cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
set(CONAN_DISABLE_CHECK_COMPILER True)
conan_basic_setup(TARGETS)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_library(Model STATIC
        src/sdk.h
        src/geom.h
        src/model.h src/model.cpp
        src/loot_generator.h src/loot_generator.cpp
        src/collision_detector.h src/collision_detector.cpp
        src/model_serialization.h)
target_link_libraries(Model PUBLIC CONAN_PKG::zlib CONAN_PKG::boost Threads::Threads CONAN_PKG::libpq CONAN_PKG::libpqxx)

add_executable(game_server
        src/main.cpp
        src/http_server.cpp src/http_server.h
        src/boost_json.cpp
        src/json_loader.h src/json_loader.cpp
        src/request_handler.cpp src/request_handler.h
        src/logger.cpp src/logger.h
        src/app.h src/app.cpp
        src/api_handler.cpp src/api_handler.h
        src/ticker.cpp src/ticker.h src/app_serialization.h src/postgres.h src/postgres.cpp)
target_link_libraries(game_server PRIVATE Model)

add_executable(serialization_tests
        tests/loot_generator_tests.cpp
        tests/collision-detector-tests.cpp
        tests/state-serialization-tests.cpp src/app_serialization.h)
target_link_libraries(serialization_tests PRIVATE CONAN_PKG::catch2 Model)