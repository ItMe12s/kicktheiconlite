cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "iOS" OR IOS)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
else()
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
endif()
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(frickyou VERSION 1.0.0)

set(SOURCES
  src/main.cpp
  src/OverlayRendering.cpp
  src/physics-overlay/PhysicsOverlay.cpp
  src/physics-overlay/PhysicsOverlayTouch.cpp
  src/physics-overlay/PhysicsOverlayShatter.cpp
  src/physics-overlay/PhysicsOverlayDebug.cpp
  src/PhysicsMenu.cpp
  src/PhysicsWorld.cpp
  src/PlayerVisual.cpp
  src/RuntimeRestart.cpp
  src/vfx/ImpactFlash.cpp
  src/vfx/ImpactNoise.cpp
  src/vfx/ObjectMotionBlurPipeline.cpp
  src/vfx/SandevistanTrail.cpp
  src/vfx/StarBurst.cpp
  src/events/ClickEvents.cpp
  src/box2d-lite/Arbiter.cpp
  src/box2d-lite/Body.cpp
  src/box2d-lite/Collide.cpp
  src/box2d-lite/Joint.cpp
  src/box2d-lite/World.cpp
)
add_library(${PROJECT_NAME} SHARED ${SOURCES})

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/motion_blur.vert"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/motion_blur.frag"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/white_flash.frag"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/color_invert.frag"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/fire_aura.frag"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/impact_noise.frag"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/OverlayShaders_embed.cpp.in"
)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/motion_blur.vert" MOTION_BLUR_VERT)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/motion_blur.frag" MOTION_BLUR_FRAG)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/white_flash.frag" WHITE_FLASH_FRAG)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/color_invert.frag" COLOR_INVERT_FRAG)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/fire_aura.frag" FIRE_AURA_FRAG)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/impact_noise.frag" IMPACT_NOISE_FRAG)
set(SHADER_EMBED_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${SHADER_EMBED_GEN_DIR}")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/src/OverlayShaders_embed.cpp.in"
  "${SHADER_EMBED_GEN_DIR}/OverlayShaders_embed.cpp"
  @ONLY
)
target_sources(${PROJECT_NAME} PRIVATE "${SHADER_EMBED_GEN_DIR}/OverlayShaders_embed.cpp")

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory("$ENV{GEODE_SDK}" ${CMAKE_CURRENT_BINARY_DIR}/geode)

target_include_directories(${PROJECT_NAME} PRIVATE src)

# target_compile_options(${PROJECT_NAME} PRIVATE
#   $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra -Wpedantic>
#   $<$<CXX_COMPILER_ID:MSVC>:/W4>)

setup_geode_mod(${PROJECT_NAME})
