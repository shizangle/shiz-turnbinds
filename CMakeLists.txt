cmake_minimum_required(VERSION 3.20)
project(Shiz-Turnbinds LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Static CRT so the exe needs no extra installs (MSVC)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(FetchContent)
FetchContent_Declare(json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_MakeAvailable(json)

add_executable(Shiz-Turnbinds
    main.cpp
    ConsoleUI.cpp
    FloatingUI.cpp
    MouseSimulation.cpp
    CS2Monitor.cpp
    Settings.cpp
    Utils.cpp)

if(MSVC)
    target_compile_options(Shiz-Turnbinds PRIVATE /utf-8)
endif()

target_compile_definitions(Shiz-Turnbinds PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
target_link_libraries(Shiz-Turnbinds PRIVATE nlohmann_json::nlohmann_json winmm user32 gdi32)
