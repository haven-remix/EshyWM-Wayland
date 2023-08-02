cmake_minimum_required(VERSION 3.24.2)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PROTOCOLS_XML /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml)
set(WAYLAND_SCANNER /usr/bin/wayland-scanner)

# --------------- ESHYWM -----------------

project(eshywm)

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(WLROOTS REQUIRED IMPORTED_TARGET wlroots)
pkg_check_modules(WAYLAND_SERVER REQUIRED IMPORTED_TARGET wayland-server)
pkg_check_modules(XKBCOMMON REQUIRED IMPORTED_TARGET xkbcommon)

# Set source files
set(ESHYWM_SOURCE_FILES eshywm.cpp server.cpp window.cpp)
list(TRANSFORM ESHYWM_SOURCE_FILES PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/source/)

# Generate xdg-shell-protocol.h using wayland-scanner
set(GENERATED_DIR ${CMAKE_CURRENT_SOURCE_DIR}/source/generated)
set(PROTOCOLS_H ${GENERATED_DIR}/xdg-shell-protocol.h)

add_custom_command(
    OUTPUT ${PROTOCOLS_H}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${GENERATED_DIR}
    COMMAND ${WAYLAND_SCANNER} server-header ${PROTOCOLS_XML} ${PROTOCOLS_H}
    DEPENDS ${PROTOCOLS_XML}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating xdg-shell-protocol.h"
)

add_executable(eshywm ${ESHYWM_SOURCE_FILES} ${PROTOCOLS_H})
target_compile_options(eshywm PRIVATE -g -Werror -DWLR_USE_UNSTABLE)
target_include_directories(eshywm PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/source/includes ${CMAKE_CURRENT_SOURCE_DIR}/source/generated)
target_link_libraries(eshywm PRIVATE PkgConfig::WLROOTS PkgConfig::WAYLAND_SERVER PkgConfig::XKBCOMMON)

# Fix function pointer type mismatch for wl_listener
target_compile_definitions(eshywm PRIVATE -D__WAYLAND_INTERNAL_API)

# Fix invalid conversion from ‘void (*)(wl_listener*, void*)’ to ‘wl_notify_func_t’ error
target_link_libraries(eshywm PRIVATE wayland-server)

# --------------- ESHYBAR -----------------

project(eshybar)

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(WAYLAND_CLIENT REQUIRED IMPORTED_TARGET wayland-client++)
pkg_check_modules(WAYLAND_CLIENT_EXTRA REQUIRED IMPORTED_TARGET wayland-client-extra++)
pkg_check_modules(WAYLAND_CURSOR REQUIRED IMPORTED_TARGET wayland-cursor++)
pkg_check_modules(WAYLAND_EGL REQUIRED IMPORTED_TARGET wayland-egl++)
pkg_check_modules(EGL REQUIRED IMPORTED_TARGET egl)
pkg_check_modules(GLESV2 REQUIRED IMPORTED_TARGET glesv2)
pkg_check_modules(GLFW REQUIRED IMPORTED_TARGET glfw3)
pkg_check_modules(GLEW REQUIRED IMPORTED_TARGET glew)

# Set source files
set(ESHYBAR_SOURCE_FILES eshybar.cpp eshyui.cpp /includes/stb_image.cpp)
list(TRANSFORM ESHYBAR_SOURCE_FILES PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/source_eshybar/)

add_executable(eshybar ${ESHYBAR_SOURCE_FILES})
target_compile_options(eshybar PRIVATE -g -Werror -DWLR_USE_UNSTABLE)
target_include_directories(eshybar PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/source_eshybar/includes)
target_link_libraries(eshybar PRIVATE PkgConfig::GLFW PkgConfig::GLEW PkgConfig::WAYLAND_EGL PkgConfig::WAYLAND_CLIENT PkgConfig::WAYLAND_CLIENT_EXTRA PkgConfig::WAYLAND_CURSOR PkgConfig::EGL PkgConfig::GLESV2)

# Fix function pointer type mismatch for wl_listener
target_compile_definitions(eshybar PRIVATE -D__WAYLAND_INTERNAL_API)