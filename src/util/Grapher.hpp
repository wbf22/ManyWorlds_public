#pragma once

#include "Bitmap.hpp"
#include <cstdint>
#include <iostream>
#include <cmath>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <iostream>
#include <random>
#include <chrono>


using namespace std;

struct Coor {
    double x, y;
};

namespace grapher {
    struct Color {
        uint8_t r, g, b;
    };

}

struct Grapher {


    /**
     * Graphs a tensor and saves as an image
     * 
     * The tensor x can be of shape [BATCH, x, y] or [BATCH, y] or [y]
     * 
     * For shapes [BATCH, x, y], the grapher will create a line for
     * each BATCH and that goes from point to point in the tensor.
     * 
     * For shapes [BATCH, y], the x coordinates for each point are assumed
     * to be the index of the point in the tensor. A line will be created for each batch.
     * 
     * For shapes [y], the x coordinates for each point are assumed to be the index of 
     * the point in the tensor.
     * 
     */
    template<typename T>
    static void graph_points(vector<pair<T, T>>& vec, const char* filename) {

        


        // create pixel data
        int width = 1920;
        int height = 1080;
        uint8_t* pixel_data = new uint8_t[height * width * 3]{25};

        // draw grid lines
        for (int y = 0; y < height; y+=40) {
            for (int x = 0; x < width; x++) {
                set_pixel(pixel_data, x, y, width, height, {64, 64, 64});
                set_pixel(pixel_data, x, y+1, width, height, {64, 64, 64});
                set_pixel(pixel_data, x, y-1, width, height, {64, 64, 64});
            }
        }
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x+=40) {
                set_pixel(pixel_data, x, y, width, height, {64, 64, 64});
                set_pixel(pixel_data, x+1, y, width, height, {64, 64, 64});
                set_pixel(pixel_data, x-1, y, width, height, {64, 64, 64});
            }
        }


        // find max and min and adjust them to be a little higher so the graph has a margin
        double max_x = numeric_limits<double>().min();
        double max_y = numeric_limits<double>().min();
        double min_x = numeric_limits<double>().max();
        double min_y = numeric_limits<double>().max();
        for (pair<T, T>& vec_pair : vec) {
            if (vec_pair.first > max_x) max_x = vec_pair.first;
            if (vec_pair.second > max_y) max_y = vec_pair.second;
            if (vec_pair.first < min_x) min_x = vec_pair.first;
            if (vec_pair.second < min_y) min_y = vec_pair.second;
        }
        double margin_x = (max_x - min_x) * 0.05;
        double margin_y = (max_y - min_y) * 0.05;
        max_x += margin_x;
        max_y += margin_y;
        min_x -= margin_x;
        min_y -= margin_y;



        // draw lines
        double x_range = max_x - min_x;
        double y_range = max_y - min_y;
        // double max_range = max(x_range, y_range);
        double x_scale = x_range == 0? 0.00001 : width / x_range;
        double y_scale = y_range == 0? 0.00001 : height / y_range;
        
        for (pair<T, T>& vec_pair : vec) {
            grapher::Color color = random_light_color();
            Coor point = {
                (vec_pair.first - min_x) * x_scale,
                (vec_pair.second - min_y) * y_scale
            };
            draw_circle(pixel_data, point, 5, width, height, color);
        }

        generateBitmapImage(pixel_data, height, width, filename);
    }

    
    static int64_t random(int64_t inclusive, int64_t exclusive)
    {
        random_device rd;  // Will be used to obtain a seed for the random number engine
        std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
        uniform_int_distribution<> distrib(inclusive, exclusive - 1);

        return distrib(gen);
    }

    static grapher::Color random_light_color() {
        return {
            (uint8_t) Grapher::random(50, 256),
            (uint8_t) Grapher::random(50, 256),
            (uint8_t) Grapher::random(50, 256)
        };
    }

    static void set_pixel(uint8_t* pixel_data, int x, int y, int width, int height, grapher::Color color) {
        if (x < 0 || x >= width || y < 0 || y >= height) return;
        
        pixel_data[y * width * 3 + x * 3 + 0] = color.r;
        pixel_data[y * width * 3 + x * 3 + 1] = color.g;
        pixel_data[y * width * 3 + x * 3 + 2] = color.b;
    }

    static void draw_circle(uint8_t* pixel_data, Coor center, double radius, int image_width, int image_height, grapher::Color color) {
        for (int x = center.x - radius; x <= center.x + radius; x++) {
            for (int y = center.y - radius; y <= center.y + radius; y++) {
                if (pow(x - center.x, 2) + pow(y - center.y, 2) <= pow(radius, 2)) {
                    set_pixel(pixel_data, x, y, image_width, image_height, color);
                }
            }
        }
    }

    static void draw_line(uint8_t* pixel_data, int width, int height, Coor start, Coor end, grapher::Color color) {
        double dx = end.x - start.x;
        double dy = end.y - start.y;
        
        if (abs(dx) > abs(dy)) {
            dy /= abs(dx);
            dx /= abs(dx);
        }
        else {
            dx /= abs(dy);
            dy /= abs(dy);
        }
        
        double x = start.x;
        double y = start.y;
        bool x_neg = dx < 0;
        bool y_neg = dy < 0;

        while((x_neg ? x >= end.x : x <= end.x) && (y_neg ? y >= end.y : y <= end.y)) {
            draw_circle(
                pixel_data, 
                { x, y },
                2,
                width, 
                height,
                color
            );
            
            x += dx;
            y += dy;
        }

    }




    // unfinished or unused

    static grapher::Color get_color(uint8_t* pixel_data, int x, int y, int width, int height) {
        grapher::Color color = {0, 0, 0};
        if (x < 0 || x >= width || y < 0 || y >= height) return color;

        color.r = pixel_data[y * width * 3 + x * 3 + 0];
        color.g = pixel_data[y * width * 3 + x * 3 + 1];
        color.b = pixel_data[y * width * 3 + x * 3 + 2];
        return color;
    }

    static void draw_char(uint8_t* pixel_data, int width, int x, int y, char c, grapher::Color color) {
        if (c == '1') {
            
        }
        else if (c == '2') {

        }
        else if (c == '3') {

        }
        else if (c == '4') {

        }
        else if (c == '5') {

        }
        else if (c == '6') {

        }
        else if (c == '7') {

        }
        else if (c == '8') {

        }
        else if (c == '9') {

        }
        else if (c == '0') {

        }
        else if (c == '.') {

        }
        else if (c == '-') {

        }
        else if (c == ',') {

        }
    }

};

