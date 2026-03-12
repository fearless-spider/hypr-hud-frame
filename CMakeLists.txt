cmake_minimum_required(VERSION 3.19)
project(hypr-hud-frame
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_SHARED_LIBRARY_PREFIX "")

find_package(PkgConfig REQUIRED)
pkg_check_modules(hyprland REQUIRED IMPORTED_TARGET hyprland)
pkg_check_modules(hyprlang REQUIRED IMPORTED_TARGET hyprlang)

add_library(hypr-hud-frame SHARED
    src/main.cpp
    src/HudFrameDecoration.cpp
    src/HudFramePassElement.cpp
    src/HudConfig.cpp
    src/HudAnimationState.cpp
)

target_include_directories(hypr-hud-frame PRIVATE
    ${hyprland_INCLUDE_DIRS}
)

target_compile_definitions(hypr-hud-frame PRIVATE
    WLR_USE_UNSTABLE
)

target_compile_options(hypr-hud-frame PRIVATE
    ${hyprland_CFLAGS}
    -Wall -Wextra -Wpedantic
    -fno-strict-aliasing
    -fPIC
)

target_link_libraries(hypr-hud-frame PRIVATE
    PkgConfig::hyprland
    OpenGL::GL
)

find_package(OpenGL REQUIRED)

install(TARGETS hypr-hud-frame
    LIBRARY DESTINATION lib/hyprland/plugins
)
