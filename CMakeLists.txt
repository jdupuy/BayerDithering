cmake_minimum_required (VERSION 3.1)

project(BayerDithering)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(dither dither.c)
add_executable(dither_tiled dither_tiled.c)
IF (NOT WIN32)
    target_link_libraries(dither m)
    target_link_libraries(dither_tiled m)
ENDIF()

