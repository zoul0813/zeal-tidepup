# Editable TMX/TSX sources are authoritative; generated assets stay in the build tree.
find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(maps_dir "${CMAKE_CURRENT_BINARY_DIR}/maps")
set(map_tiles_dir "${maps_dir}/tiles")
file(GLOB map_tmx CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/assets/shores/*.tmx")
add_custom_target(${target_name}_map_tiles)
set(map_tile_outputs)
foreach(tmx IN LISTS map_tmx)
    get_filename_component(stem "${tmx}" NAME_WE)
    set(ztm "${map_tiles_dir}/${stem}.ztm")
    tiled2zeal(${target_name}_map_tiles FILES "${tmx}" OUTPUT "${ztm}")
    list(APPEND map_tile_outputs "${ztm}")
endforeach()
file(GLOB_RECURSE map_sources CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/*.tmx"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/*.tsx")
set(map_outputs
    "${maps_dir}/shore0.ztm"
    "${maps_dir}/shore0.spn"
    "${maps_dir}/shore1.ztm"
    "${maps_dir}/shore1.spn"
    "${maps_dir}/shore2.ztm"
    "${maps_dir}/shore2.spn"
    "${maps_dir}/shore3.ztm"
    "${maps_dir}/shore3.spn"
    "${maps_dir}/map_data.c"
    "${maps_dir}/map_routes.c"
)
add_custom_command(
    OUTPUT ${map_outputs}
    COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/tools/pack_maps.py"
        --out "${maps_dir}" --tiles "${map_tiles_dir}"
    COMMAND ${CMAKE_COMMAND} -E touch ${map_outputs}
    DEPENDS ${map_sources} ${map_tile_outputs}
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/pack_maps.py"
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/tmx_helpers.py"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/game.h"
    COMMENT "Packing Tiled maps for ${target_name}"
    VERBATIM
)
add_custom_target(${target_name}_maps DEPENDS ${map_outputs})
add_dependencies(${target_name}_maps ${target_name}_map_tiles)
add_dependencies(${target_name} ${target_name}_maps)
target_include_directories(${target_name} PRIVATE "${maps_dir}" "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_sources(${target_name} PRIVATE "${maps_dir}/map_data.c")
if(AUTOPILOT)
    target_sources(${target_name} PRIVATE "${maps_dir}/map_routes.c")
endif()
set(map_assets
    "${maps_dir}/shore0.ztm"
    "${maps_dir}/shore0.spn"
    "${maps_dir}/shore1.ztm"
    "${maps_dir}/shore1.spn"
    "${maps_dir}/shore2.ztm"
    "${maps_dir}/shore2.spn"
    "${maps_dir}/shore3.ztm"
    "${maps_dir}/shore3.spn"
)
