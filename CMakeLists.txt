cmake_minimum_required(VERSION 3.16)

set(target_name tidepup)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(BIN_DIR "${CMAKE_SOURCE_DIR}/bin" CACHE PATH "Where the .bin goes")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BIN_DIR})

set(ZOS_TOOLCHAIN sdcc)
include($ENV{ZOS_PATH}/cmake/zos_init.cmake)

zos_use(zvb)
include($ENV{ZGDK_PATH}/cmake/zgdk_init.cmake)

project(tidepup C ASM)

add_executable(${target_name}
    src/controls.c
    src/main.c
    src/stream.c
    src/world.c
    src/pup.c
    src/actors.c
    src/hud.c
    src/sfx.c
    src/music.c
    src/assets.c
)
target_compile_options(${target_name} PRIVATE --opt-code-size)

# (minimal: the flip-screen room loader isn't used)
target_sources(${target_name} PRIVATE src/water.c)

# For recording videos: a bot plays (src/autopilot.c), and the game can start on a later shore
option(AUTOPILOT "Bot plays the game (video recording)" OFF)
if(AUTOPILOT)
    target_sources(${target_name} PRIVATE src/autopilot.c)
    target_compile_definitions(${target_name} PRIVATE AUTOPILOT=1)
endif()
set(START_SHORE 0 CACHE STRING "First shore, 0-based (video recording)")
target_compile_definitions(${target_name} PRIVATE START_SHORE=${START_SHORE})

find_package(ZVB_SDK REQUIRED CONFIG PATHS "$ENV{ZVB_SDK_PATH}/cmake" NO_DEFAULT_PATH)
find_package(ZGDK REQUIRED CONFIG PATHS "$ENV{ZGDK_PATH}/cmake" NO_DEFAULT_PATH)
target_link_libraries(${target_name} PUBLIC zgdk zvb_gfx zvb_sound)

# Native map bytes and spawn metadata are generated from assets/**/*.tmx.
# Tileset GIF and music remain tracked assets; map exports stay in the build tree.
include(cmake/maps.cmake)
include(cmake/assets.cmake)

zos_add_outputs(${target_name})

set_property(TARGET ${target_name} APPEND PROPERTY LINK_DEPENDS "$ENV{ZGDK_PATH}/lib/zgdk.lib")
