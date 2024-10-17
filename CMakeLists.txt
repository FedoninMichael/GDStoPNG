cmake_minimum_required(VERSION 3.26)
project(GDStoPNG)

set(CMAKE_CXX_STANDARD 17)

add_executable(GDStoPNG gds_to_png.cpp)

# Укажите путь к директории gdstk
set(GDSTK_DIR "${CMAKE_SOURCE_DIR}/external/gdstk")

# Добавьте каталог с заголовочными файлами gdstk
include_directories(${GDSTK_DIR}/include/gdstk)
include_directories(${CMAKE_SOURCE_DIR}/external/stb)

# Подключите библиотеку gdstk к вашему проекту
target_link_libraries(GDStoPNG PRIVATE gdstk)