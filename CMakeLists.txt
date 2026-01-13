cmake_minimum_required(VERSION 3.16)
project(Cubica VERSION 0.2 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(include)

file(GLOB SOURCES
    src/*.cpp
    src/**/*.cpp
    src/*.c
    src/**/*.c
    src/menu.cpp
    src/player_renderer.cpp
    src/net_server.cpp
    src/net_client.cpp
    src/font.cpp
)

find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)

if (GLFW_FOUND)
    include_directories(${GLFW_INCLUDE_DIRS})
    link_libraries(${GLFW_LIBRARIES})
else()
    message(FATAL_ERROR "GLFW not found! Install glfw3 development package.")
endif()

add_executable(${PROJECT_NAME} ${SOURCES})

find_package(OpenGL REQUIRED)
target_link_libraries(${PROJECT_NAME} ${OPENGL_gl_LIBRARY} m dl)
