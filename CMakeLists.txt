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




# --------------- ESHYUI -----------------

project(EshyUI)

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW REQUIRED IMPORTED_TARGET glfw3)
pkg_check_modules(GLEW REQUIRED IMPORTED_TARGET glew)

# Set source files
set(ESHYUI_SOURCE_FILES EshyUI.cpp Renderer.cpp Entity.cpp IndexBuffer.cpp Shader.cpp Texture.cpp VertexArray.cpp VertexBuffer.cpp stb/stb_image.cpp)
list(TRANSFORM ESHYUI_SOURCE_FILES PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/EshyUI/Source/)

add_library(EshyUI STATIC ${ESHYUI_SOURCE_FILES})
target_compile_options(EshyUI PRIVATE -g -Werror -DWLR_USE_UNSTABLE)
target_include_directories(EshyUI PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/EshyUI/Source)
target_link_libraries(EshyUI PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/EshyUI/EshyUI.lib PkgConfig::GLFW PkgConfig::GLEW)

# Fix function pointer type mismatch for wl_listener
target_compile_definitions(EshyUI PRIVATE -D__WAYLAND_INTERNAL_API)




# --------------- ESHYBAR -----------------

project(eshybar)

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW REQUIRED IMPORTED_TARGET glfw3)
pkg_check_modules(GLEW REQUIRED IMPORTED_TARGET glew)

# Set source files
set(ESHYBAR_SOURCE_FILES eshybar.cpp)
list(TRANSFORM ESHYBAR_SOURCE_FILES PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/source_eshybar/)

add_executable(eshybar ${ESHYBAR_SOURCE_FILES})
target_compile_options(eshybar PRIVATE -g -Werror -DWLR_USE_UNSTABLE)
target_include_directories(eshybar PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/source_eshybar/includes ${CMAKE_CURRENT_SOURCE_DIR}/EshyUI/Source)
target_link_libraries(eshybar PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/build/libEshyUI.a PkgConfig::GLFW PkgConfig::GLEW)

# Fix function pointer type mismatch for wl_listener
target_compile_definitions(eshybar PRIVATE -D__WAYLAND_INTERNAL_API)