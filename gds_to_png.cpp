#include <iostream>
#include <getopt.h>
#include <vector>
#include <string>
#include <sstream>
#include <regex>
#include "external/gdstk/include/gdstk/gdstk.hpp"
#include "external/gdstk/include/gdstk/clipper_tools.hpp"
#include "external/gdstk/external/clipper/clipper.hpp"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "external/stb/stb_image_write.h"

using namespace std;
bool layer_provided = false;


// Redefine the string into pair of coordinates
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


// Transformation gdstk::Polygon into Clipper::Path
ClipperLib::Path convert_to_clipper(const gdstk::Polygon* polygon) {
    ClipperLib::Path path;
    for (size_t i = 0; i < polygon->point_array.count; ++i) {
        path << ClipperLib::IntPoint(
                static_cast<int64_t>(polygon->point_array[i].x * 1e2),
                static_cast<int64_t>(polygon->point_array[i].y * 1e2)
        );
    }
    return path;
}


// Transformation Clippe::Path into gdstk::Polygon
gdstk::Polygon convert_from_clipper(const ClipperLib::Path& path) {
    gdstk::Polygon result = {0};
    for (const auto& point : path) {
        result.point_array.append({static_cast<double>(point.X) * 1e-2, static_cast<double>(point.Y) * 1e-2});
    }
    return result;
}


int main(int argc, char* argv[]) {

    string input_gds, output_gds;
    pair<double, double> lowleft_coord = {0, 0};
    pair<double, double> upright_coord = {0, 0};
    bool cut = false;

    string layer_datatype_str;
    gdstk::Tag layer_datatype = gdstk::make_tag(0, 0);

    // define options
    static struct option long_options[] = {
            {"input", required_argument, 0, 'i'},
            {"layer", required_argument, 0, 'L'},
            {"output", optional_argument, 0, 'o'},
            {"lowleft", optional_argument, 0, 'l'},
            {"upright", optional_argument, 0, 'u'},
            {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "i:L:o:l:u:", long_options, nullptr)) != -1) {
        switch (opt) {
            case 'i': input_gds = optarg; break;
//            case 'o': output_gds = optarg; break;
            case 'L': {
                string layer_arg(optarg);
                size_t comma_pos = layer_arg.find(',');
                if (comma_pos == string::npos) {
                    cerr << "error: --layer must contain a comma separating layer and datatype (e.g., 8,0)" << endl;
                    return 1;
                }
                try {
                    int layer = stoi(layer_arg.substr(0, comma_pos));
                    int datatype = stoi(layer_arg.substr(comma_pos + 1));
                    std::cout << "parsed layer: " << layer << ", datatype: " << datatype << std::endl;
                    layer_datatype = gdstk::make_tag(layer, datatype);
                    layer_provided = true;
                } catch (const invalid_argument& e) {
                    cerr << "error parsing --layer: " << e.what() << endl;
                    return 1;
                }
                break;
            }
            case 'o':
                if (optarg) {
                    output_gds = optarg;
                } else {
                    cerr << "error: -o option requires an argument" << endl;
                    return 1;
                }
                break;
            case 'l': lowleft_coord = parse_coordinates(optarg); cut = true; break;
            case 'u': upright_coord = parse_coordinates(optarg); cut = true; break;
            default: cerr << "usage: --input <input.gds> --layer (layer,datatype) [--output <output.gds>] [--lowleft x,y] [--upright x,y]\\n";
            return 1;
        }
    }

    if (input_gds.empty()) {
        cerr << "error: --input <input.gds> is mandatory\n";
        return 1;
    }

    if (!layer_provided) {
        cerr << "error: --layer is mandatory and must be in the format layer,datatype (e.g., 8,0)\n";
        return 1;
    }

    if (output_gds.empty()) output_gds = input_gds;

    gdstk::Set<gdstk::Tag> layer_datatypes = {};
    layer_datatypes.add(layer_datatype);
    gdstk::Library lib = gdstk::read_gds(input_gds.c_str(), 1e-6, 10e-8, &layer_datatypes, nullptr);
    gdstk::Cell* top_cell = lib.cell_array[0];
    gdstk::Array<gdstk::Reference*> removed_references = {0};
    top_cell->flatten(true, removed_references);

    gdstk::Array<gdstk::Polygon *> final_polygons = {0};

    if (cut) {
        // define a bounding rectangle for the clipper
        ClipperLib::Path rect = {
                ClipperLib::IntPoint(lowleft_coord.first * 1e2, lowleft_coord.second * 1e2),
                ClipperLib::IntPoint(upright_coord.first * 1e2, lowleft_coord.second * 1e2),
                ClipperLib::IntPoint(upright_coord.first * 1e2, upright_coord.second * 1e2),
                ClipperLib::IntPoint(lowleft_coord.first * 1e2, upright_coord.second * 1e2)
        };

        // trimming polygons
        ClipperLib::Clipper clipper;
        clipper.AddPath(rect, ClipperLib::ptClip, true);

        gdstk::Array<gdstk::Polygon *> polygons = {0};
        top_cell->get_polygons(true, true, 0, false, gdstk::Tag(), polygons);

        ClipperLib::Paths clipped_polygons;
        for (size_t i = 0; i < polygons.count; ++i) {
            ClipperLib::Path poly_path = convert_to_clipper(polygons[i]);
            clipper.AddPath(poly_path, ClipperLib::ptSubject, true);
        }

        clipper.Execute(ClipperLib::ctIntersection, clipped_polygons);

        // transformation clipper::paths into gdstk::polygon
        for (const auto &path: clipped_polygons) {
            final_polygons.append(new gdstk::Polygon(convert_from_clipper(path)));
        }

        // replace old polygons into trim polygons
        top_cell->clear();
        for (size_t i = 0; i < final_polygons.count; ++i) {
            top_cell->polygon_array.append(final_polygons[i]);
        }
    } else {
        // copying all polygons without cutting
        top_cell->get_polygons(true, true, 0, false, gdstk::Tag(), final_polygons);
    }


//    todo: realized black pixels sets in vertices of polygons. necessary define pixel size in key to program
//     and fill polygons with black pixels.
//    // determining the size of blank image
//    int width = upright_coord.first - lowleft_coord.first;
//    int height = upright_coord.second - lowleft_coord.second;
//    cout << "width: " << width << endl;
//    cout << "height: " << height << endl;
//
//    // blank image
//    std::vector<unsigned char> image(height * width, 255);
//
//    // sets black pixels at the vertices of polygons.
//    for (size_t i = 0; i < final_polygons.count; ++i) {
//        gdstk::polygon* polygon = final_polygons[i];
//        for (size_t j = 0; j < polygon->point_array.count; ++j) {
////            int x = static_cast<int>(polygon->point_array[j].x) - min_x;
////            int y = static_cast<int>(polygon->point_array[j].y) - min_y;
//            int x = static_cast<int>(polygon->point_array[j].x) - lowleft_coord.first;
//            int y = upright_coord.second - static_cast<int>(polygon->point_array[j].y);
//            if (x >= 0 && x < width && y >= 0 && y < height) {
//                image[y * width + x] = 0;  // black pixel at vertices of polygons
//            }
//        }
//    }
//
//    // save topology to png
//    stbi_write_png(output_png.c_str(), width, height, 1, image.data(), width);
//    cout << "изображение сохранено: " << output_png << endl;


    // create a gdsii library and cell for output
    gdstk::Library library = {};
    library.init("output_library", 1e-6, 1e-8);
    gdstk::Cell* output_cell = new gdstk::Cell();
    output_cell->name = gdstk::copy_string("output", NULL);
    library.cell_array.append(output_cell);

    // add trim polygons to output cell
    for (size_t i = 0; i < final_polygons.count; ++i) {
        output_cell->polygon_array.append(final_polygons[i]);
    }

    // write library to gds file
    library.write_gds(output_gds.c_str(), 0, NULL);
    cout << "gds file saved: " << output_gds << endl;

    for (size_t i = 0; i < final_polygons.count; ++i) {
        delete final_polygons[i];
    }


    layer_datatypes.clear();
    final_polygons.clear();
    output_cell->clear();
    delete output_cell;
    library.clear();
    lib.clear();
    top_cell->clear();

    return 0;
}

