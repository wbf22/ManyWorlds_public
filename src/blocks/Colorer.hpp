#pragma once

using namespace std;


struct Colorer {

    struct Range {
        int64_t start;
        int64_t stop;
        string material;
    };

    void y_level_color(Grid<shared_ptr<Block>>& blocks, vector<Range> colors) {

        // apply the materials based on their ranges to each block in the grid based on y level
    }


    void z_level_color(Grid<shared_ptr<Block>>& blocks, vector<Range> colors) {

        // apply the materials based on their ranges to each block in the grid based on z level
    }


    void angle_level_color(Grid<shared_ptr<Block>>& blocks, vector<Range> colors) {

        // find the avg point of the grid

        // apply the materials based on their ranges to each block in the grid based on 0-360 angle around the avg point
    }

    void solid_color(Grid<shared_ptr<Block>>& blocks, string material) {
        // set the material on each block
    }

    vector<Range> make_every_other_range(string material_1, string material_2, int step, int length) {

        // make a vector of ranges alternating ever 'step' blocks between the colors

        return {};
    }
};