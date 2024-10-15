#include <iostream>
#include <vector>
#include <string>
#include "external/gdstk/include/gdstk/gdstk.hpp"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "external/stb/stb_image_write.h"

using namespace std;

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

int main() {
    // Чтение GDSII-файлов
    gdstk::Library library = gdstk::Library::create();
    library.read("die5_from_topleft_layer4_slits_shown.GDS");
    library.read("die5_from_topleft_layer2_shown.GDS");

    // Получение слоёв из библиотеки
    gdstk::Cell* cell_layer4 = library.cell("layer4");
    gdstk::Cell* cell_layer2 = library.cell("layer2");

    // Получение элементов слоёв
    auto elements_layer4 = cell_layer4->elements;
    auto elements_layer2 = cell_layer2->elements;

    // Вычисление размеров изображения
    gdstk::Array<gdstk::Polygon*> polygons;
    cell_layer4->get_polygons(polygons);
    gdstk::Box bbox_layer4 = polygons.bounding_box();
    int min_x = bbox_layer4.min.x;
    int min_y = bbox_layer4.min.y;
    int max_x = bbox_layer4.max.x;
    int max_y = bbox_layer4.max.y;
    int width = max_x - min_x + 1;
    int height = max_y - min_y + 1;

    // Создание изображений
    vector<unsigned char> l4_output(width * height, 255);  // Белый фон
    vector<unsigned char> l2_output(width * height, 255);  // Белый фон
    vector<unsigned char> combined(width * height, 255);   // Белый фон

    // Рисование на изображениях
    for (auto& elem : elements_layer4) {
        gdstk::Box bbox = elem->bounding_box();
        draw_rectangle(l4_output, width, bbox.min.x - min_x, bbox.min.y - min_y, bbox.max.x - min_x, bbox.max.y - min_y);
    }

    for (auto& elem : elements_layer2) {
        gdstk::Polygon* polygon = dynamic_cast<gdstk::Polygon*>(elem);
        if (polygon) {
            gdstk::Array<gdstk::Point> points;
            polygon->get_points(points);
            for (size_t i = 0; i < points.size; ++i) {
                int x = points[i].x - min_x;
                int y = points[i].y - min_y;
                set_pixel(l2_output, width, x, y);
            }
        }
    }

    // Комбинированное изображение
    for (auto& elem : elements_layer4) {
        gdstk::Box bbox = elem->bounding_box();
        draw_rectangle(combined, width, bbox.min.x - min_x, bbox.min.y - min_y, bbox.max.x - min_x, bbox.max.y - min_y);
    }

    for (auto& elem : elements_layer2) {
        gdstk::Polygon* polygon = dynamic_cast<gdstk::Polygon*>(elem);
        if (polygon) {
            gdstk::Array<gdstk::Point> points;
            polygon->get_points(points);
            for (size_t i = 0; i < points.size; ++i) {
                int x = points[i].x - min_x;
                int y = points[i].y - min_y;
                set_pixel(combined, width, x, y);
            }
        }
    }

    // Сохранение изображений
    stbi_write_png("l4_slits.png", width, height, 1, l4_output.data(), width);
    stbi_write_png("l2_pores.png", width, height, 1, l2_output.data(), width);
    stbi_write_png("l2_l4_combined.png", width, height, 1, combined.data(), width);

    std::cout << "done" << std::endl;
    return 0;
}
