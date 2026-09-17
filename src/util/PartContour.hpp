#pragma once

#include "../body/BodyPart.hpp"
#include "Util.hpp"
#include <memory>
#include <vector>
#include <string>
#include <cmath>
#include <functional>
#include <unordered_map>

using namespace std;

struct PartDim{
    double i; // position along the length of the limb
    double width; // width at this position
    double center_offset_y; // offset from center + or -
    double center_offset_x; // offset from center + or -
    double flatten_amount; // flatten amount with 0 being perfectly flat horizontally, 2 being perfectly flat vertically
};

struct PartContour {

    using BlockFilter = function<bool(double x, double y, double z)>;

    static void fill_line_of_blocks(double z, double x_s, double x_e, double y_s, double y_e, double blocks_size, vector<shared_ptr<Block>>& blocks, string axis, const BlockFilter& filter = {}) {
        for (double y = y_s; y <= y_e; ++y) {
            for (double x = x_s; x <= x_e; ++x) {
                if (filter) {
                    if (axis == "z" && filter(x, y, z)) continue; 
                    if (axis == "y" && filter(x, z, y)) continue; 
                }
                shared_ptr<Block> block = std::make_shared<Block>();
                block->size = blocks_size;
                if (axis == "z") {
                    block->position_double = std::make_shared<PositionDouble>(x*blocks_size, y*blocks_size, z*blocks_size);
                }
                else if (axis == "y") {
                    block->position_double = std::make_shared<PositionDouble>(x*blocks_size, z*blocks_size, y*blocks_size);
                }
                blocks.push_back(block);
            }
        }
    }

    static void fill_annulus_line_of_blocks(double z, int outer_x, int inner_x, int y, double center_offset_x, double center_offset_y, double blocks_size, vector<shared_ptr<Block>>& blocks, string axis, const BlockFilter& filter = {}) {
        double cy = y + center_offset_y;
        double cx = center_offset_x;
        double x_left_outer = -outer_x + cx;
        double x_right_outer = outer_x + cx;
        double x_left_inner = -inner_x + cx;
        double x_right_inner = inner_x + cx;

        for (double x = x_left_outer; x <= x_right_outer; ++x) {
            if (x > x_left_inner && x < x_right_inner) continue;
            if (filter) {
                if (axis == "z" && filter(x, cy, z)) continue; 
                if (axis == "y" && filter(x, z, cy)) continue; 
            }
            shared_ptr<Block> block = std::make_shared<Block>();
            block->size = blocks_size;
            if (axis == "z") {
                block->position_double = std::make_shared<PositionDouble>(x*blocks_size, cy*blocks_size, z*blocks_size);
            }
            else if (axis == "y") {
                block->position_double = std::make_shared<PositionDouble>(x*blocks_size, z*blocks_size, cy*blocks_size);
            }
            blocks.push_back(block);
        }
    }

    static string adapt_axis(double angle, string axis="z") {
        if ((angle > 45 && angle < 135) || (angle > 225 && angle < 315)) {
            if (axis == "z") {
                axis = "y";
            }
            else {
                axis = "z";
            }
        }
        return axis;
    }

    static double angle_y_offset(double angle, int blocks_from_start, int total_length, string axis) {
        double rot_offset_y = 0;
        if (axis == "z" && angle) {
            int mes = total_length - blocks_from_start;
            rot_offset_y = sin(angle * M_PI / 180.0) * mes;
        }
        else if (axis == "y") {
            rot_offset_y = cos(angle * M_PI / 180.0) * blocks_from_start;
        }
        return rot_offset_y;
    }

    static void add_blocks_with_midpoint_circle_algorithm(double i, int rx, int ry, double block_size, double center_offset_x, double center_offset_y, vector<shared_ptr<Block>>& blocks, string axis, int wall_thickness=0, const BlockFilter& filter = {}) {
        double x = 0;
        double y = ry;
        double rx2 = rx*rx;
        double ry2 = ry*ry;

        double d = ry2 - rx2*ry + 0.25*rx2;
        double dx = 2*ry2*x;
        double dy = 2*rx2*y;

        bool thin = wall_thickness <= 0 || rx <= wall_thickness || ry <= wall_thickness;
        int inner_rx = max(1, rx - wall_thickness);
        int inner_ry = max(1, ry - wall_thickness);
        double inner_rx2 = inner_rx * inner_rx;
        double inner_ry2 = inner_ry * inner_ry;

        auto fill = [&](int xv, int yv) {
            if (thin) {
                PartContour::fill_line_of_blocks(i, -xv+center_offset_x, xv+center_offset_x, yv+center_offset_y, yv+center_offset_y, block_size, blocks, axis, filter);
            } else {
                int inner_x = 0;
                if (abs(yv) <= inner_ry) {
                    double inner_x_val = inner_rx2 * (1.0 - (double)(yv*yv) / inner_ry2);
                    if (inner_x_val >= 0) inner_x = round(sqrt(inner_x_val));
                }
                PartContour::fill_annulus_line_of_blocks(i, xv, inner_x, yv, center_offset_x, center_offset_y, block_size, blocks, axis, filter);
            }
        };

        while(dx < dy) {
            dx += 2*ry2;
            if (d < 0) {
                d += ry2 + dx;
            }
            else {
                fill(x, y);
                fill(x, -y);
                y -= 1;
                dy -= 2*rx2;
                d += ry2 + dx - dy;
            }
            x += 1;
        }

        d = ry2*(x+0.5)*(x+0.5) + rx2*(y-1)*(y-1) - rx2*ry2;
        while (y >= 0) {
            fill(x, y);
            if (y != -y) {
                fill(x, -y);
            }
            y -= 1;
            dy -= 2*rx2;
            if (d > 0) {
                d += rx2 - dy;
            }
            else {
                x += 1;
                dx += 2*ry2;
                d += rx2 - dy + dx;
            }
        }
    }

    static void interpolate_part_contour(double length, vector<shared_ptr<Block>>& blocks, vector<PartDim> part_dims, double block_size, double angle, string axis="z", bool round_ends=false, bool hollow=false) {

        angle = angle - floor(angle / 360) * 360;
        axis = PartContour::adapt_axis(angle, axis);

        double start_offset_x = 0.0;
        double start_offset_y = 0.0;
        double start_offset_i = 0.0;
        int length_in_blocks = length / block_size;
        int last_rx = 0, last_ry = 0;
        for (double i = 0; i < length_in_blocks; ++i) {

            vector<double> weights;
            vector<double> widths;
            vector<double> center_offset_ys;
            vector<double> center_offset_xs;
            vector<double> flatten_amounts;
            double total_weight = 0;
            for (PartDim& dim : part_dims) {
                double weight = 1.0 / Util::no_zero(abs(dim.i - i));
                weights.push_back(weight);
                total_weight += weight;

                double rot_offset_x = 0;
                double rot_offset_y = PartContour::angle_y_offset(angle, i, length_in_blocks, axis);
                
                widths.push_back(dim.width);
                center_offset_ys.push_back(dim.center_offset_y + rot_offset_y);
                center_offset_xs.push_back(dim.center_offset_x + rot_offset_x);
                flatten_amounts.push_back(dim.flatten_amount);
            }

            double center_offset_y = round(Util::interpolate(center_offset_ys, weights, total_weight));
            double center_offset_x = round(Util::interpolate(center_offset_xs, weights, total_weight));
            double flatten_amount = round(Util::interpolate(flatten_amounts, weights, total_weight));

            if (i == 0) {
                start_offset_x = -center_offset_x;
                start_offset_y = -center_offset_y;
                if (angle > 45 && angle <= 225) {
                    start_offset_i = -length_in_blocks;
                }
            }

            if (round_ends) {
                widths.push_back(0);
                double weight = 1.0 / pow(Util::no_zero(abs(0 - i)), 4);
                weights.push_back(weight);
                total_weight += weight;
            }
            double width = Util::interpolate(widths, weights, total_weight);

            double width_in_blocks = width / block_size;
            int rx = ceil(width_in_blocks * (2-flatten_amount));
            int ry = ceil(width_in_blocks * flatten_amount);

            int wall_thickness = 0;
            if (hollow) {
                wall_thickness = max(1, max(abs(rx - last_rx), abs(ry - last_ry)));
            }
            last_rx = rx;
            last_ry = ry;

            int i_mod;
            if (axis == "z"){
                if (angle <= 45 || angle >= 315 ) {
                    i_mod = i;
                }
                else {
                    i_mod = length_in_blocks - i;
                }
            }
            else {
                if (angle < 225) {
                    i_mod = length_in_blocks - i;
                }
                else {
                    i_mod = i;
                }
            }
            PartContour::add_blocks_with_midpoint_circle_algorithm(
                i_mod + start_offset_i, 
                rx, 
                ry, 
                block_size, 
                center_offset_x + start_offset_x, 
                center_offset_y + start_offset_y, 
                blocks, 
                axis,
                wall_thickness
            );
        }
    }

    template<typename F>
    static void midpoint_circle_perimeter(int r, F callback, double arc_start = -M_PI, double arc_end = M_PI) {
        if (r <= 0) return;
        bool full_circle = (arc_end - arc_start) >= 2.0 * M_PI - 0.001;
        int x = 0, z = r;
        int d = 1 - r;
        while (x <= z) {
            int pts[8][2] = {
                {x,z},{z,x},{-x,z},{-z,x},
                {x,-z},{z,-x},{-x,-z},{-z,-x}
            };
            for (int i = 0; i < 8; i++) {
                int px = pts[i][0], pz = pts[i][1];
                if (!full_circle) {
                    double a = atan2((double)pz, (double)px);
                    double sa = arc_start;
                    double ea = arc_end;
                    if (a < sa) a += 2.0 * M_PI;
                    if (ea < sa) ea += 2.0 * M_PI;
                    if (a < sa || a > ea) continue;
                }
                callback(px, pz);
            }
            if (d < 0) {
                d += 2*x + 3;
            } else {
                d += 2*(x - z) + 5;
                z--;
            }
            x++;
        }
    }

};
