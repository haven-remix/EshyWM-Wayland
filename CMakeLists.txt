cmake_minimum_required(VERSION 3.24.2)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(XDG_SHELL_PROTOCOLS_XML /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml)
set(WLR_LAYER_PROTOCOLS_XML ${CMAKE_CURRENT_SOURCE_DIR}/Protocols/wlr-layer-shell-unstable-v1.xml)
set(WAYLAND_SCANNER /usr/bin/wayland-scanner)

set(ESHYIPC_PROJECT_NAME EshyIPC)
set(ESHYUI_PROJECT_NAME EshyUI)
set(ESHYWM_PROJECT_NAME eshywm)
set(ESHYBAR_PROJECT_NAME eshybar)

# --------------- ESHYIPC -----------------

project(${ESHYIPC_PROJECT_NAME})

# Set source files
set(ESHYIPC_SOURCE_FILES EshyIPC.cpp)
list(TRANSFORM ESHYIPC_SOURCE_FILES PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/EshyIPC/)

add_library(${ESHYIPC_PROJECT_NAME} STATIC ${ESHYIPC_SOURCE_FILES})
target_include_directories(${ESHYIPC_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/EshyIPC/)

# --------------- ESHYUI -----------------

project(${ESHYUI_PROJECT_NAME})

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW REQUIRED IMPORTED_TARGET glfw3)
pkg_check_modules(GLEW REQUIRED IMPORTED_TARGET glew)

# Set source files
set(ESHYUI_SOURCE_FILES EshyUI.cpp Renderer.cpp Entity.cpp IndexBuffer.cpp Shader.cpp Texture.cpp VertexArray.cpp VertexBuffer.cpp stb/stb_image.cpp)
list(TRANSFORM ESHYUI_SOURCE_FILES PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/EshyUI/)

add_library(${ESHYUI_PROJECT_NAME} STATIC ${ESHYUI_SOURCE_FILES})
target_include_directories(${ESHYUI_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/EshyUI/)
target_link_libraries(${ESHYUI_PROJECT_NAME} PRIVATE PkgConfig::GLFW PkgConfig::GLEW)

# --------------- ESHYWM -----------------

project(${ESHYWM_PROJECT_NAME})

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(WLROOTS REQUIRED IMPORTED_TARGET wlroots)
pkg_check_modules(WAYLAND_SERVER REQUIRED IMPORTED_TARGET wayland-server)
pkg_check_modules(XKBCOMMON REQUIRED IMPORTED_TARGET xkbcommon)
pkg_check_modules(XCB REQUIRED IMPORTED_TARGET xcb)
pkg_check_modules(NLOHMANNJSON REQUIRED IMPORTED_TARGET nlohmann_json)

# Set source files
set(ESHYWM_SOURCE_FILES EshyWM.cpp Server.cpp Window.cpp SpecialWindow.cpp Output.cpp Keyboard.cpp Config.cpp)
list(TRANSFORM ESHYWM_SOURCE_FILES PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/source/)

# Generate xdg-shell-protocol.h using wayland-scanner
set(GENERATED_DIR ${CMAKE_CURRENT_SOURCE_DIR}/source/generated)
set(XDG_SHELL_PROTOCOLS_H ${GENERATED_DIR}/xdg-shell-protocol.h)
set(WLR_LAYER_PROTOCOLS_H ${GENERATED_DIR}/wlr-layer-shell-unstable-v1-protocol.h)

add_custom_command(
    OUTPUT ${XDG_SHELL_PROTOCOLS_H}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${GENERATED_DIR}
    COMMAND ${WAYLAND_SCANNER} server-header ${XDG_SHELL_PROTOCOLS_XML} ${XDG_SHELL_PROTOCOLS_H}
    DEPENDS ${XDG_SHELL_PROTOCOLS_XML}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating xdg-shell-protocol.h"
)

# add_custom_command(
#     OUTPUT ${WLR_LAYER_PROTOCOLS_H}
#     COMMAND ${CMAKE_COMMAND} -E make_directory ${GENERATED_DIR}
#     COMMAND ${WAYLAND_SCANNER} server-header ${WLR_LAYER_PROTOCOLS_XML} ${WLR_LAYER_PROTOCOLS_H}
#     DEPENDS ${WLR_LAYER_PROTOCOLS_XML}
#     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
#     COMMENT "Generating wlr-layer-shell-unstable-v1-protocol.h"
# )

add_executable(${ESHYWM_PROJECT_NAME} ${ESHYWM_SOURCE_FILES} ${XDG_SHELL_PROTOCOLS_H})
target_compile_options(${ESHYWM_PROJECT_NAME} PRIVATE -g -Werror -DWLR_USE_UNSTABLE)
target_include_directories(
    ${ESHYWM_PROJECT_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/source/includes
    ${CMAKE_CURRENT_SOURCE_DIR}/source/generated
    ${CMAKE_CURRENT_SOURCE_DIR}/EshyIPC/
    ${CMAKE_CURRENT_SOURCE_DIR}/Shared/)
target_link_libraries(${ESHYWM_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/build/libEshyIPC.a PkgConfig::GLFW PkgConfig::GLEW PkgConfig::NLOHMANNJSON PkgConfig::WLROOTS PkgConfig::WAYLAND_SERVER PkgConfig::XKBCOMMON PkgConfig::XCB)

# Fix function pointer type mismatch for wl_listener
target_compile_definitions(${ESHYWM_PROJECT_NAME} PRIVATE -D__WAYLAND_INTERNAL_API)

# Fix invalid conversion from ‘void (*)(wl_listener*, void*)’ to ‘wl_notify_func_t’ error
target_link_libraries(${ESHYWM_PROJECT_NAME} PRIVATE wayland-server)

# --------------- ESHYBAR -----------------

project(${ESHYBAR_PROJECT_NAME})

# Set source files
set(ESHYBAR_SOURCE_FILES eshybar.cpp)
list(TRANSFORM ESHYBAR_SOURCE_FILES PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/source_eshybar/)

add_executable(${ESHYBAR_PROJECT_NAME} ${ESHYBAR_SOURCE_FILES})
target_include_directories(
    ${ESHYBAR_PROJECT_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/source_eshybar/includes
    ${CMAKE_CURRENT_SOURCE_DIR}/EshyUI/
    ${CMAKE_CURRENT_SOURCE_DIR}/EshyIPC/
    ${CMAKE_CURRENT_SOURCE_DIR}/Shared/)
target_link_libraries(
    ${ESHYBAR_PROJECT_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/build/libEshyIPC.a
    ${CMAKE_CURRENT_SOURCE_DIR}/build/libEshyUI.a
    PkgConfig::NLOHMANNJSON
    PkgConfig::GLFW
    PkgConfig::GLEW)