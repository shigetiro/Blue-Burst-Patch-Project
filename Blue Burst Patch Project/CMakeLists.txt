cmake_minimum_required(VERSION 3.5)
project(BBPP)

cmake_policy(SET CMP0077 NEW)

# This macro defines a cmake option that can be used to enable a patch.
# ARGN can be used for additional defines.
macro(define_optional_patch patch_name)
    option(${patch_name} "" OFF)

    if(${patch_name})
        add_compile_definitions(${patch_name})

        foreach(additional_define ${ARGN})
            set(${additional_define} ON)
            add_compile_definitions(${additional_define})
        endforeach()
    endif()
endmacro()

# Define all patches
# NOTE: For a clean Extreme-difficulty-only build, everything except
# PATCH_FASTWARP and PATCH_EXTREME_DIFFICULTY is disabled.
# The set(... ON CACHE FORCE) lines below were the previous defaults;
# they are now OFF so only the two patches above are compiled in.
define_optional_patch(PATCH_BOX PATCH_ITEMS PATCH_FRAMERATE)
define_optional_patch(PATCH_CUSTOMIZE_MENU)
define_optional_patch(PATCH_DISABLE_GAMEGUARD)
define_optional_patch(PATCH_EARLY_WALK_FIX)
set(PATCH_FASTWARP ON CACHE BOOL "" FORCE)
define_optional_patch(PATCH_FASTWARP)
define_optional_patch(PATCH_FRAMERATE)
set(PATCH_IME ON CACHE BOOL "" FORCE)
define_optional_patch(PATCH_IME)
set(PATCH_ITEMS OFF CACHE BOOL "" FORCE)
define_optional_patch(PATCH_ITEMS)
define_optional_patch(PATCH_KEYBOARD_ALTERNATE_PALETTE)
define_optional_patch(PATCH_LARGE_ASSETS)
define_optional_patch(PATCH_NEWENEMY USE_NEWGFX PATCH_INITLISTS PATCH_ENEMY_CONSTRUCTOR_LISTS)
define_optional_patch(PATCH_OMNISPAWN PATCH_INITLISTS PATCH_ENEMY_CONSTRUCTOR_LISTS)
define_optional_patch(PATCH_SKIP_INTRO_CREDITS)
define_optional_patch(PATCH_SLOW_GIBBLES_FIX PATCH_INITLISTS)
define_optional_patch(PATCH_UNSELLABLE_RARES)
define_optional_patch(PATCH_HOOKS)
define_optional_patch(PATCH_KEYBOARD_HOOKS PATCH_HOOKS)
define_optional_patch(PATCH_EDITORS PATCH_KEYBOARD_HOOKS)
define_optional_patch(PATCH_NEWMAP PATCH_MAP_OBJECT_CONSTRUCTOR_LISTS PATCH_ENEMY_CONSTRUCTOR_LISTS PATCH_INITLISTS)
set(PATCH_NOFALL OFF CACHE BOOL "" FORCE)
define_optional_patch(PATCH_NOFALL PATCH_HOOKS)
set(PATCH_WEAPON_VTABLES OFF CACHE BOOL "" FORCE)
define_optional_patch(PATCH_WEAPON_VTABLES)
set(PATCH_EXTREME_DIFFICULTY ON CACHE BOOL "" FORCE)
define_optional_patch(PATCH_EXTREME_DIFFICULTY)
define_optional_patch(PATCH_ASSET_MANAGER)
define_optional_patch(ENABLE_DEVMOD)

# This makes the lpVtbl field of COM objects accessible in Wine's headers
add_compile_definitions(CINTERFACE)

# List all source files here
set(source_files
    battleparam.cpp
    common.cpp
    customize_menu.cpp
    earlywalk.cpp
    extreme_difficulty.cpp
    editors.cpp
    editors/TCameraEditor.cpp
    editors/TFreeCamera.cpp
    editors/TGroupEnemySetEditor.cpp
    editors/TGroupSetEditor.cpp
    editors/TParticleEditor.cpp
    editors/TQuestScriptChecker.cpp
    editors/TSetEvtScriptTest.cpp
    enemy.cpp
    entity.cpp
    entitylist.cpp
    fastwarp.cpp
    helpers.cpp
    hooking.cpp
    ime.cpp
    initlist.cpp
    keyboard.cpp
    large_assets.cpp
    map.cpp
    mathutil.cpp
    newenemy.cpp
    object_extension.cpp
    object_wrapper.cpp
    object.cpp
    omnispawn.cpp
    omnispawn_particles.cpp
    omnispawn_sound_effects.cpp
    palette.cpp
    patching.cpp
    psobb_functions.cpp
    shop.cpp
    slow_gibbles.cpp
    map_object.cpp
    nofall.cpp
    newmap/newmap.cpp
    newmap/map_object_cloud.cpp
    newmap/map_object_snowfall.cpp
    newmap/map_object_newdoor.cpp
    newmap/slbgm.cpp
    newmap/snow_map.cpp
    newmap/fog.cpp
    newmap/sunlight.cpp
    newmap/maploader.cpp
    newmap/setdata.cpp
    newmap/enemies_and_objects.cpp
    newmap/sound_effects.cpp
    newmap/particle_effects.cpp
    newmap/enemy_icecube.cpp
    newmap/footsteps.cpp
    newmap/map_object_payload.cpp
    newmap/map_object_turret.cpp
    newmap/map_object_slippery.cpp
    newmap/map_object_snowwisp.cpp
    devmod_interface.h
    ninja.cpp
    quest.cpp
    player.cpp
    
    newgfx/animation.cpp
    newgfx/bone.cpp
    newgfx/mesh.cpp
    newgfx/model.cpp

    # D3D8 hook + overlay system (used by PATCH_FRAMERATE / PATCH_BOX)
    d3d8_hook.cpp

    # TrinityDLL integration
    trinity_box.cpp
    trinity_encryption.cpp
    trinity_helpers.cpp
    trinity_items.cpp
    trinity_psobb.cpp
    trinity_resolution.cpp

    # Weapon specials and vtable fixes
    weapon_specials.cpp)

add_library(${PROJECT_NAME} SHARED dllmain.cpp psobb.cpp ${source_files})

# Link with assimp if using newgfx
if(USE_NEWGFX)
    target_link_libraries(${PROJECT_NAME} assimp)
endif()

find_package(minhook CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} minhook::minhook)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Remove "lib" prefix from output file
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")

# Enable warnings
if (MSVC)
    add_compile_options(/W4 /std:c++20)
else()
    add_compile_options(-Wall)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/devmod" "${CMAKE_CURRENT_SOURCE_DIR}/../include")

# Force 32-bit target
if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /std:c++20)
else()
    set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS -m32 LINK_FLAGS "-m32 -static -static-libgcc -static-libstdc++")
endif()

# Define DEBUG in debug mode
target_compile_definitions(${PROJECT_NAME} PUBLIC "$<$<CONFIG:DEBUG>:DEBUG>")

# Enable WIN32_LEAN_AND_MEAN
target_compile_definitions(${PROJECT_NAME} PUBLIC "WIN32_LEAN_AND_MEAN")

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:${PROJECT_NAME}>"
        "D:/Dev Stuff/Phantasy Star Online BB/bbpp.dll"
    COMMENT "Deploying BBPP.dll to game directory"
)

# Create a separate dll from files inside devmod dir
if (ENABLE_DEVMOD)
    file(GLOB devmod_source_files "${CMAKE_CURRENT_SOURCE_DIR}/devmod/*.cpp")
    add_library(devmod SHARED ${source_files} ${devmod_source_files})
    set_target_properties(devmod PROPERTIES PREFIX "")
    target_include_directories(devmod PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    set_target_properties(devmod PROPERTIES COMPILE_FLAGS -m32 LINK_FLAGS "-m32 -static -static-libgcc -static-libstdc++")
    target_compile_definitions(devmod PUBLIC "$<$<CONFIG:DEBUG>:DEBUG>")
    target_compile_definitions(devmod PUBLIC "WIN32_LEAN_AND_MEAN")
endif()
