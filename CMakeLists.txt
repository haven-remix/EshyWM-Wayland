cmake_minimum_required(VERSION 3.24.2)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(eshywm)

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(WLROOTS REQUIRED IMPORTED_TARGET wlroots)
pkg_check_modules(WAYLAND_SERVER REQUIRED IMPORTED_TARGET wayland-server)
pkg_check_modules(XKBCOMMON REQUIRED IMPORTED_TARGET xkbcommon)

# Set source files
set(SOURCE_FILES eshywm.cpp)
list(TRANSFORM SOURCE_FILES PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/source/)

# Generate xdg-shell-protocol.h using wayland-scanner
set(GENERATED_DIR ${CMAKE_CURRENT_SOURCE_DIR}/source/generated)
set(PROTOCOLS_XML /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml)
set(PROTOCOLS_H ${GENERATED_DIR}/xdg-shell-protocol.h)
set(WAYLAND_SCANNER /usr/bin/wayland-scanner)

add_custom_command(
    OUTPUT ${PROTOCOLS_H}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${GENERATED_DIR}
    COMMAND ${WAYLAND_SCANNER} server-header ${PROTOCOLS_XML} ${PROTOCOLS_H}
    DEPENDS ${PROTOCOLS_XML}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating xdg-shell-protocol.h"
)

# Create the executable
add_executable(eshywm ${SOURCE_FILES} ${PROTOCOLS_H})

# Set compiler flags
target_compile_options(eshywm PRIVATE -g -Werror -DWLR_USE_UNSTABLE)

# Include directories
target_include_directories(eshywm PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/source/includes ${CMAKE_CURRENT_SOURCE_DIR}/source/generated)

# Link against required libraries
target_link_libraries(eshywm PRIVATE PkgConfig::WLROOTS PkgConfig::WAYLAND_SERVER PkgConfig::XKBCOMMON)

# Fix function pointer type mismatch for wl_listener
target_compile_definitions(eshywm PRIVATE -D__WAYLAND_INTERNAL_API)

# Fix invalid conversion from ‘void (*)(wl_listener*, void*)’ to ‘wl_notify_func_t’ error
target_link_libraries(eshywm PRIVATE wayland-server)