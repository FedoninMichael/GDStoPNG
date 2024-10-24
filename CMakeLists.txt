cmake_minimum_required(VERSION 3.26)
project(GDStoPNG)

set(CMAKE_CXX_STANDARD 17)

add_executable(GDStoPNG gds_to_png.cpp)

set(GDSTK_DIR "${CMAKE_SOURCE_DIR}/external/gdstk")

include_directories(${GDSTK_DIR}/include/gdstk)
include_directories(${CMAKE_SOURCE_DIR}/external/stb)
include_directories(${GDSTK_DIR}/external/clipper)
include_directories(/usr/include/libqhull_r)

target_link_libraries(GDStoPNG PRIVATE /usr/local/lib/libclipper.a)

find_package(ZLIB REQUIRED)
find_package(Qhull REQUIRED)

find_library(ZLIB_LIB z)

target_link_libraries(GDStoPNG PRIVATE gdstk qhullstatic_r ${ZLIB_LIB})

