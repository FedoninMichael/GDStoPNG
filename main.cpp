#include <iostream>
#include <opencv2/opencv.hpp>
#include <libgdsii/GDSIIModel.h>
#include <filesystem>

using namespace cv;
using namespace std;
namespace fs = std::filesystem;

// Функции для рисования "разрезов" и "пор"
void add_slits(Mat &image, const vector<GDSIIElement> &elements, int min_x, int min_y) {
    for (const auto &box : elements) {
        auto bounds = box.getBoundingBox();  // Получаем bounding box элемента
        int x1 = bounds.minX - min_x;
        int y1 = bounds.minY - min_y;
        int x2 = bounds.maxX - min_x;
        int y2 = bounds.maxY - min_y;
        rectangle(image, Point(x1, y1), Point(x2, y2), Scalar(0), FILLED); // Рисуем прямоугольник
    }
}

void add_pores(Mat &image, const vector<GDSIIElement> &elements, int min_x, int min_y) {
    for (const auto &obj : elements) {
        for (const auto &coord : obj.getPoints()) {
            int x = coord.x - min_x;
            int y = coord.y - min_y;
            if (x >= 0 && y >= 0 && x < image.cols && y < image.rows) {
                image.at<uchar>(y, x) = 0; // Ставим точку (черный цвет)
            }
        }
    }
}

int main() {
    // Импорт GDSII файлов
    string layer4_path = fs::absolute("die5_from_topleft_layer4_slits_shown.GDS").string();
    string layer2_path = fs::absolute("die5_from_topleft_layer2_shown.GDS").string();

    GDSIIModel l4_gds(layer4_path);
    GDSIIModel l2_gds(layer2_path);

    // Получение bounding box для каждого слоя
    auto l4_bounds = l4_gds.getBoundingBox();
    auto l2_bounds = l2_gds.getBoundingBox();

    // Вычисление минимальных и максимальных значений координат
    int min_x = min(l4_bounds.minX, l2_bounds.minX);
    int min_y = min(l4_bounds.minY, l2_bounds.minY);
    int width = max(l4_bounds.maxX - min_x, l2_bounds.maxX - min_x);
    int height = max(l4_bounds.maxY - min_y, l2_bounds.maxY - min_y);

    // Создание изображений
    Mat l4_output(height + 1, width + 1, CV_8UC1, Scalar(255));  // Белый фон
    Mat l2_output(height + 1, width + 1, CV_8UC1, Scalar(255));  // Белый фон
    Mat combined(height + 1, width + 1, CV_8UC1, Scalar(255));   // Белый фон

    // Рисуем слои
    add_slits(l4_output, l4_gds.getElements(), min_x, min_y);
    add_pores(l2_output, l2_gds.getElements(), min_x, min_y);

    // Комбинированное изображение
    add_slits(combined, l4_gds.getElements(), min_x, min_y);
    add_pores(combined, l2_gds.getElements(), min_x, min_y);

    // Сохраняем изображения
    imwrite("l4_slits.png", l4_output);
    imwrite("l2_pores.png", l2_output);
    imwrite("l2_l4_combined.png", combined);

    cout << "done" << endl;
    return 0;
}
