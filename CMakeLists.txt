cmake_minimum_required(VERSION 3.10)

project(game)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")

add_subdirectory(3rdparty)
add_subdirectory(game)

if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
    add_subdirectory(tools)
endif()
