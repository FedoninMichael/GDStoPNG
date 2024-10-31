#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include "external/gdstk/include/gdstk/gdstk.hpp"
#include "external/gdstk/include/gdstk/clipper_tools.hpp"
#include "external/gdstk/external/clipper/clipper.hpp"
#include <vector>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "external/stb/stb_image_write.h"

using namespace std;

// Преобразование строки с координатами в пару чисел
pair<double, double> parse_coordinates(const string& coord) {
    stringstream ss(coord);
    string item;
    vector<double> coords;

    while (getline(ss, item, ',')) {
        coords.push_back(stod(item));
    }
    if (coords.size() != 2) {
        throw invalid_argument("Координаты должны быть в формате x,y");
    }
    return {coords[0], coords[1]};
}

// Функция для рисования прямоугольников
void draw_rectangle(vector<unsigned char>& image, int width, int x1, int y1, int x2, int y2) {
    for (int y = y1; y <= y2; ++y) {
        for (int x = x1; x <= x2; ++x) {
            if (x >= 0 && x < width && y >= 0 && y < image.size() / width) {
                image[y * width + x] = 0; // Чёрный цвет
            }
        }
    }
}

// Функция для установки пикселя
void set_pixel(vector<unsigned char>& image, int width, int x, int y) {
    if (x >= 0 && x < width && y >= 0 && y < image.size() / width) {
        image[y * width + x] = 0; // Чёрный цвет
    }
}

// Преобразование gdstk::Polygon в Clipper Path
ClipperLib::Path convert_to_clipper(const gdstk::Polygon* polygon) {
    ClipperLib::Path path;
    for (size_t i = 0; i < polygon->point_array.count; ++i) {
        path << ClipperLib::IntPoint(
                static_cast<int64_t>(polygon->point_array[i].x * 1e2),  // Масштабирование для Clipper
                static_cast<int64_t>(polygon->point_array[i].y * 1e2)
        );
    }
    return path;
}

// Преобразование Clipper Path обратно в gdstk::Polygon
gdstk::Polygon convert_from_clipper(const ClipperLib::Path& path) {
    gdstk::Polygon result;
    for (const auto& point : path) {
        result.point_array.append({static_cast<double>(point.X) * 1e-2, static_cast<double>(point.Y) * 1e-2});  // Обратное масштабирование
    }
    return result;
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        cerr << "Использование: " << argv[0] << " <input.gds> <x1,y1> <x2,y2> <output.png>" << endl;
        return 1;
    }

    string input_gds = argv[1];
    pair<double, double> lowleft_coord, upright_coord;
    try {
        lowleft_coord = parse_coordinates(argv[2]);
        upright_coord = parse_coordinates(argv[3]);
    } catch (const invalid_argument& e) {
        cerr << "Ошибка: " << e.what() << endl;
        return 1;
    }
    string output_png = argv[4];

    // Чтение GDSII-файла
    gdstk::Library lib = gdstk::read_gds(input_gds.c_str(), 1e-6, 10e-8, nullptr, nullptr);

    gdstk::Cell* top_cell = lib.cell_array[0];
    gdstk::Array<gdstk::Reference*> removed_references = {0};
    top_cell->flatten(true, removed_references);

    // Определяем ограничивающий прямоугольник для Clipper
    ClipperLib::Path rect = {
            ClipperLib::IntPoint(lowleft_coord.first * 1e2, lowleft_coord.second * 1e2),
            ClipperLib::IntPoint(upright_coord.first * 1e2, lowleft_coord.second * 1e2),
            ClipperLib::IntPoint(upright_coord.first * 1e2, upright_coord.second * 1e2),
            ClipperLib::IntPoint(lowleft_coord.first * 1e2, upright_coord.second * 1e2)
    };
    cout << "Ограничивающий прямоугольник: " << rect << endl;

    // Обрезка полигонов с использованием Clipper
    ClipperLib::Clipper clipper;
    clipper.AddPath(rect, ClipperLib::ptClip, true);  // Прямоугольник для обрезки

    gdstk::Array<gdstk::Polygon*> polygons;
    top_cell->get_polygons(true, true, 0, false, gdstk::Tag(), polygons);

    ClipperLib::Paths clipped_polygons;
    for (size_t i = 0; i < polygons.count; ++i) {
        ClipperLib::Path poly_path = convert_to_clipper(polygons[i]);
        clipper.AddPath(poly_path, ClipperLib::ptSubject, true);
    }

    clipper.Execute(ClipperLib::ctIntersection, clipped_polygons);

    // Преобразование обратно в gdstk::Polygon
    gdstk::Array<gdstk::Polygon*> final_polygons;
    for (const auto& path : clipped_polygons) {
        final_polygons.append(new gdstk::Polygon(convert_from_clipper(path)));
    }
    cout << "Новые полигоны: " << clipped_polygons << endl;

    // Очищаем старые полигоны и заменяем новыми
    top_cell -> clear();
    for (size_t i = 0; i < final_polygons.count; ++i) {
        top_cell->polygon_array.append(final_polygons[i]);
    }


    int width = upright_coord.first - lowleft_coord.first;
    int height = upright_coord.second - lowleft_coord.second;
    cout << "width: " << width << endl;
    cout << "height: " << height << endl;

    // Создание пустого изображения с белым фоном
    std::vector<unsigned char> image(height * width, 255);

    // Рисование полигонов на изображении
    for (size_t i = 0; i < final_polygons.count; ++i) {
        gdstk::Polygon* polygon = final_polygons[i];
        for (size_t j = 0; j < polygon->point_array.count; ++j) {
//            int x = static_cast<int>(polygon->point_array[j].x) - min_x;
//            int y = static_cast<int>(polygon->point_array[j].y) - min_y;
            int x = static_cast<int>(polygon->point_array[j].x) - lowleft_coord.first;
            int y = upright_coord.second - static_cast<int>(polygon->point_array[j].y);
            if (x >= 0 && x < width && y >= 0 && y < height) {
                image[y * width + x] = 0;  // Установка черного пикселя
            }
        }
    }

    // Сохранение изображения в PNG файл
    stbi_write_png(output_png.c_str(), width, height, 1, image.data(), width);

    cout << "Изображение сохранено: " << output_png << endl;


    // Create a GDSII library and cell for output
    gdstk::Library library = {};
    library.init("output_library", 1e-6, 1e-8);
    gdstk::Cell output_cell = {};
    library.cell_array.append(&output_cell);

    // Add polygons to the cell
    for (size_t i = 0; i < final_polygons.count; ++i) {
        output_cell.polygon_array.append(final_polygons[i]);
    }

    // Write library to GDS file
    std::string output_gds = "output.gds";
    library.write_gds(output_gds.c_str(), 100, NULL);

    cout << "GDS file saved: " << output_gds << endl;

    // Free cell and library resources
    output_cell.clear();
    library.clear();


    for (size_t i = 0; i < final_polygons.count; ++i) {
        delete final_polygons[i];
    }

    return 0;
}

