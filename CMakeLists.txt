cmake_minimum_required(VERSION 3.26)
project(GDStoPNG)

set(CMAKE_CXX_STANDARD 17)

#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -fsanitize=address -fno-omit-frame-pointer")
#set(CMAKE_BUILD_TYPE Debug)

add_executable(GDStoGDS gds_to_png.cpp)

set(GDSTK_DIR "${CMAKE_SOURCE_DIR}/external/gdstk")

include_directories(${GDSTK_DIR}/include/gdstk)
include_directories(${CMAKE_SOURCE_DIR}/external/stb)
include_directories(${GDSTK_DIR}/external/clipper)
include_directories(/usr/include/libqhull_r)

find_package(ZLIB REQUIRED)
find_package(Qhull REQUIRED)

find_library(ZLIB_LIB z)

target_link_libraries(GDStoGDS PRIVATE /usr/local/lib/libclipper.a gdstk qhullstatic_r ${ZLIB_LIB})

