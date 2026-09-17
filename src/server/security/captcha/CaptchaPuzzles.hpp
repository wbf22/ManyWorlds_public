#pragma once


#include <vector>
#include "../../Settings.hpp"
#include "util/CryptoRandom.hpp"
#include "server/ServerUtil.h"
#include <unordered_map>
#include <unordered_set>
#include <deque>
#include "PuzzleData.hpp"
#include "util/data_structures/IndexPool.hpp"




using namespace std;


inline uint8_t GREY[3] = {156, 156, 156};
inline uint8_t DARK_GREY[3] = {77, 77, 77};
inline uint8_t BLACK[3] = {20, 20, 20};
inline uint8_t WHITE[3] = {250, 250, 100};
inline uint8_t RED_[3] = {220, 40, 40};
inline uint8_t GREEN[3] = {0, 150, 20};
inline uint8_t LIGHT_BLUE[3] = {10, 140, 170};
inline uint8_t BLUE[3] = {10, 65, 170};
inline uint8_t ORANGE[3] = {180, 110, 4};
inline uint8_t PURPLE[3] = {150, 6, 150};
inline uint8_t BROWN[3] = {92, 60, 5};
inline uint8_t YELLOW[3] = {215, 190, 35};
inline uint8_t PINK[3] = {255, 105, 240};

inline uint8_t* COLORS[13] = {GREY, DARK_GREY, BLACK, WHITE, RED_, GREEN, LIGHT_BLUE, BLUE, ORANGE, PURPLE, BROWN, YELLOW, PINK};

const int DRAWING_SIZE = 20;
const int MAX_DRAWING_SIZE = 30;


enum DRAWING {
    UFO,
    ALIEN,
    WIZARD,
    SKELETON,
    PLANET,
    MOON,
    STAR,
    ASTERIOD,
    NEBULA
};

const int REFRESH_RATE_SEC = 10; 
const int POOL_SIZE = 1234;
const int REFRESH_SIZE = 40; // this means a puzzles lifespan is ~5 minutes. 'life_span = (REFRESH_RATE_SEC / REFRESH_SIZE) * POOL_SIZE'
const uint16_t puzzle_square = Settings::CAPTCHA_DIFFICULTY * MAX_DRAWING_SIZE / 2;

struct CaptchaPuzzles {



    // main puzzle pool
    IndexPool<PuzzleData> puzzles;




    CaptchaPuzzles() {
        for (int i = 0; i < POOL_SIZE; i++) {
            make_new();
        }
    }

    ~CaptchaPuzzles() {}

    // PUZZLE POOL MAINTENANCE AND ANSWER CHECKING
    PuzzleData get_puzzle(uint64_t& puzzle_id, uint16_t& square) {
        int64_t rand = CryptoRandom::rand(0, POOL_SIZE);
        square = puzzle_square;
        return this->puzzles.random(rand, puzzle_id);
    }

    bool check_puzzle(uint64_t puzzle_id, unordered_set<uint8_t>& solution) {
        // check if the solution is correct
        unordered_set<uint8_t> correct_solution = this->puzzles[puzzle_id].solution;
        bool is_correct = solution == correct_solution;

        if (is_correct) {
            // increment successful_submissions (prevents attacks)
            this->puzzles[puzzle_id].successful_submissions++;

            // delete if 2 submissions are used (prevents attacks)
            if (this->puzzles[puzzle_id].successful_submissions > 1) {
                delete_puzzle(puzzle_id);
                make_new();
            }
        }


        return is_correct;
    }

    void make_new() {

        // make new puzzle
        uint32_t image_size = puzzle_square * puzzle_square * 3;
        uint8_t* puzzle = new uint8_t[image_size];
        unordered_set<uint8_t> solution;
        make_puzzle(puzzle, puzzle_square, solution);
        this->puzzles.insert(PuzzleData(puzzle, image_size, solution));

    }

    void delete_puzzle(uint64_t puzzle_id) {
        this->puzzles.erase(puzzle_id);
    }



    // PUZZLE GENERATION
    static void make_puzzle(uint8_t* puzzle, int square, unordered_set<uint8_t>& solution) {

        CaptchaPuzzles::gen_background(puzzle, square, square);

        // add drawing to image
        unordered_set<int32_t> image_locations;
        for (int i = 0; i < Settings::CAPTCHA_DIFFICULTY; ++i) {

            // choose random drawing
            DRAWING type = (DRAWING) CryptoRandom::rand_byte(0, 3);
            uint8_t* drawing = new uint8_t[DRAWING_SIZE * DRAWING_SIZE * 3]();
            switch(type) {
                case UFO:
                    ufo(drawing);
                    break;
                case ALIEN:
                    alien(drawing);
                    break;
                case WIZARD:
                    wizard(drawing);
                    break;
                case SKELETON:
                    skeleton(drawing);
                    break;
                default:
                    break;
            }

            // add to solution set
            solution.insert(type);

            // change colors slightly
            corrupt_image(drawing, DRAWING_SIZE * DRAWING_SIZE * 3);

            // rotate
            uint8_t* new_drawing = new uint8_t[DRAWING_SIZE * DRAWING_SIZE * 3]();
            rotate_image(drawing, new_drawing, CryptoRandom::rand_double(-0.2, 0.2), DRAWING_SIZE);
            delete[] drawing;
            drawing = new_drawing;

            // place
            place_randomly(image_locations, puzzle, square, drawing, DRAWING_SIZE, DRAWING_SIZE);
            delete[] drawing;

            
        }
    }

    static void gen_background(uint8_t* pixel_data, int width, int height) {
        
        uint8_t star_white[15] = {
            200, 200, 250, // white
            227, 232, 250, // blue
            250, 230, 250, // purple
            250, 160, 160, // red
            250, 250, 230 // yellow
        };

        for (int x = 0; x < width * height * 3; x+=3) {
            
            if (CryptoRandom::rand_bool(1)) {

                int i = CryptoRandom::rand_byte(0, 5) * 3;

                uint8_t r = star_white[i];
                uint8_t g = star_white[i+1];
                uint8_t b = star_white[i+2];

                pixel_data[x] = b;
                pixel_data[x+1] = g;
                pixel_data[x+2] = r;
            }
            else {
                uint8_t r = CryptoRandom::rand_byte(0, 20);
                uint8_t g = CryptoRandom::rand_byte(0, 20);
                uint8_t b = CryptoRandom::rand_byte(0, 50);
                
                pixel_data[x] = b;
                pixel_data[x+1] = g;
                pixel_data[x+2] = r;
            }
        }
    }

    static void place_randomly(unordered_set<int32_t>& image_locations, uint8_t* image, int square, uint8_t* drawing, int dr_width, int dr_height) {
            
        int16_t x = CryptoRandom::rand_byte(0, square - MAX_DRAWING_SIZE);
        int16_t y = CryptoRandom::rand_byte(0, square - MAX_DRAWING_SIZE);

        int32_t coor = (x << 16) | y;

        while (image_locations.find(coor) != image_locations.end()) {
            x = CryptoRandom::rand_byte(0, square - MAX_DRAWING_SIZE);
            y = CryptoRandom::rand_byte(0, square - MAX_DRAWING_SIZE);
            coor = (x << 16) | y;
        }

        image_locations.insert(coor);

        for (int x_i = 0; x_i < dr_width; ++x_i) {
            for (int y_i = 0; y_i < dr_height; ++y_i) {
            
                int dr_index = (y_i * dr_height + x_i) * 3;

                // skip if pixel is black
                if (drawing[dr_index] == 0 && drawing[dr_index+1] == 0 && drawing[dr_index+2] == 0) continue;
                
                // copy to final image
                int image_index = ((y + y_i) * square + (x + x_i)) * 3;
                image[image_index] = drawing[dr_index];
                image[image_index+1] = drawing[dr_index+1];
                image[image_index+2] = drawing[dr_index+2];
            }
        }

        // mark locations as taken
        int radius = MAX_DRAWING_SIZE / 3;
        for (int x_i = x - radius; x_i < x + radius; ++x_i) {
            for (int y_i = y - radius; y_i < y + radius; ++y_i) {
                int32_t coor_i = (x_i << 16) | y_i;
                image_locations.insert(coor_i);
            }
        }
    }

    static void scale_image(uint8_t* drawing, uint8_t* new_drawing, int width, int height, int new_width, int new_height) {

        // DOESN'T WORK YET

        double scale_x = (double) width / new_width;
        double scale_y = (double) height / new_height;

        for (int x = 0; x < new_width; ++x) {
            for (int y = 0; y < new_height; ++y) {
                
                int index = (y * new_width + x) * 3;

                // get the pixel from the original image
                double original_x = x * scale_x;
                double original_y = y * scale_y;
                int original_index = ((int) original_y * width + (int) original_x) * 3;

                new_drawing[index] = drawing[original_index];
                new_drawing[index+1] = drawing[original_index+1];
                new_drawing[index+2] = drawing[original_index+2];
            }
        }
    }

    static void corrupt_image(uint8_t* drawing, int size) {
        for (int i = 0; i < size; i+=3) {
            
            // skip if pixel is black
            if (drawing[i] == 0 && drawing[i+1] == 0 && drawing[i+2] == 0) continue;


            // modify color slightly
            drawing[i] += CryptoRandom::rand_byte(-2, 2);
            if (drawing[i] > 255) drawing[i] = 255;
            if (drawing[i] < 0) drawing[i] = 0;

            drawing[i+1] += CryptoRandom::rand_byte(-2, 2);
            if (drawing[i+1] > 255) drawing[i+1] = 255;
            if (drawing[i+1] < 0) drawing[i+1] = 0;

            drawing[i+2] += CryptoRandom::rand_byte(-2, 2);
            if (drawing[i+2] > 255) drawing[i+2] = 255;
            if (drawing[i+2] < 0) drawing[i+2] = 0;
        }
    }

    static void rotate_image(uint8_t* drawing, uint8_t* new_drawing, double radians, int square) {
        
        int center_x = square / 2;
        int center_y = square / 2;

        for (int x = 0; x < square; ++x) {
            for (int y = 0; y < square; ++y) {
            
                int index = (y * square + x) * 3;

                // skip if pixel is black
                if (drawing[index] == 0 && drawing[index+1] == 0 && drawing[index+2] == 0) continue;
                
                // rotate pixel
                double new_x_d = cos(radians) * (x - center_x) - sin(radians) * (y - center_y) + center_x;
                double new_y_d = sin(radians) * (x - center_x) + cos(radians) * (y - center_y) + center_y;
                
                // rounding
                double new_x;
                double new_y;
                double dec_x = modf(new_x_d, &new_x);
                double dec_y = modf(new_y_d, &new_y);

                // set
                int new_index = ((int) new_y * square + (int) new_x) * 3;
                new_drawing[new_index] = drawing[index];
                new_drawing[new_index+1] = drawing[index+1];
                new_drawing[new_index+2] = drawing[index+2];
            }
        }
    }


    // DRAWING FUNCTIONS
    static void ufo(uint8_t* image_20_by_20) {
        
        //   0        1        2      3     4       5        6        7      8      9       10     11     12
        // {GREY, DARK_GREY, BLACK, WHITE, RED_, GREEN, LIGHT_BLUE, BLUE, ORANGE, PURPLE, BROWN, YELLOW, PINK}

        // main color
        uint8_t options_main[8] = {0, 3, 4, 5, 8, 9, 10, 11};
        int main_index = options_main[CryptoRandom::rand_byte(0, 8)];
        uint8_t* main_color = COLORS[main_index];

        // secondary color
        uint8_t options_secondary[3] = {1, 2, 7};
        int secondary_index = options_secondary[CryptoRandom::rand_byte(0, 3)];
        uint8_t* secondary_color = COLORS[secondary_index];

        // windshield
        uint8_t options_windsheild[3] = {6, 12};
        int windshield_index = options_windsheild[CryptoRandom::rand_byte(0, 2)];
        uint8_t* windshield_color = COLORS[windshield_index];

        // lights
        int light_index_1 = CryptoRandom::rand_byte(0, 11);
        while(light_index_1 == main_index || light_index_1 == secondary_index) light_index_1 = CryptoRandom::rand_byte(0, 11);
        uint8_t* light_1 = COLORS[light_index_1];

        int light_index_2 = CryptoRandom::rand_byte(0, 11);
        while(light_index_2 == main_index || light_index_2 == secondary_index) light_index_2 = CryptoRandom::rand_byte(0, 11);
        uint8_t* light_2 = COLORS[light_index_2];

        
        // define pixels
        vector< pair<uint8_t*, pair<int, int>> > pixels = {
            // light
            {light_1, {9, 8}},
            
            // windsheild
            {windshield_color, {7, 9}},
            {windshield_color, {8, 9}},
            {windshield_color, {9, 9}},
            {windshield_color, {10, 9}},
            {windshield_color, {11, 9}},
            {windshield_color, {12, 9}},

            {windshield_color, {6, 10}},
            {windshield_color, {7, 10}},
            {windshield_color, {8, 10}},
            {windshield_color, {9, 10}},
            {windshield_color, {10, 10}},
            {windshield_color, {11, 10}},
            {windshield_color, {12, 10}},
            {windshield_color, {13, 10}},

            {windshield_color, {5, 11}},
            {windshield_color, {6, 11}},
            {windshield_color, {7, 11}},
            {windshield_color, {8, 11}},
            {windshield_color, {9, 11}},
            {windshield_color, {10, 11}},
            {windshield_color, {11, 11}},
            {windshield_color, {12, 11}},
            {windshield_color, {13, 11}},
            {windshield_color, {14, 11}},

            // main body
            {main_color, {3, 12}},
            {main_color, {4, 12}},
            {main_color, {5, 12}},
            {secondary_color, {6, 12}},
            {main_color, {7, 12}},
            {main_color, {8, 12}},
            {secondary_color, {9, 12}},
            {main_color, {10, 12}},
            {main_color, {11, 12}},
            {secondary_color, {12, 12}},
            {main_color, {13, 12}},
            {main_color, {14, 12}},
            {secondary_color, {15, 12}},

            {main_color, {2, 13}},
            {secondary_color, {3, 13}},
            {main_color, {4, 13}},
            {secondary_color, {5, 13}},
            {main_color, {6, 13}},
            {main_color, {7, 13}},
            {main_color, {8, 13}},
            {secondary_color, {9, 13}},
            {main_color, {10, 13}},
            {main_color, {11, 13}},
            {main_color, {12, 13}},
            {secondary_color, {13, 13}},
            {main_color, {14, 13}},
            {main_color, {15, 13}},
            {secondary_color, {16, 13}},
            {main_color, {17, 13}},

            {secondary_color, {2, 14}},
            {main_color, {3, 14}},
            {main_color, {4, 14}},
            {secondary_color, {5, 14}},
            {main_color, {6, 14}},
            {main_color, {7, 14}},
            {main_color, {8, 14}},
            {secondary_color, {9, 14}},
            {main_color, {10, 14}},
            {main_color, {11, 14}},
            {main_color, {12, 14}},
            {secondary_color, {13, 14}},
            {main_color, {14, 14}},
            {main_color, {15, 14}},
            {main_color, {16, 14}},
            {main_color, {17, 14}},

            {secondary_color, {3, 15}},
            {main_color, {4, 15}},
            {secondary_color, {5, 15}},
            {main_color, {6, 15}},
            {main_color, {7, 15}},
            {main_color, {8, 15}},
            {secondary_color, {9, 15}},
            {main_color, {10, 15}},
            {main_color, {11, 15}},
            {main_color, {12, 15}},
            {secondary_color, {13, 15}},
            {main_color, {14, 15}},
            {secondary_color, {15, 15}},
            {main_color, {16, 15}},

            {main_color, {5, 16}},
            {secondary_color, {6, 16}},
            {main_color, {7, 16}},
            {main_color, {8, 16}},
            {secondary_color, {9, 16}},
            {main_color, {10, 16}},
            {main_color, {11, 16}},
            {secondary_color, {12, 16}},
            {main_color, {13, 16}},
            {main_color, {14, 16}},

            // bottom lights
            {light_2, {6, 17}},
            {light_2, {9, 17}},
            {light_2, {12, 17}},


        };

        
        // set pixels
        for (auto& pixel : pixels) {
            int index = (pixel.second.second * 20 + pixel.second.first) * 3;
            image_20_by_20[index] = pixel.first[0];
            image_20_by_20[index+1] = pixel.first[1];
            image_20_by_20[index+2] = pixel.first[2];
        }

        /*
            (2 + 13 * 20) * 3        
        */

    }

    static void alien(uint8_t* image_20_by_20) {

        //   0        1        2      3     4       5        6        7      8      9       10     11     12
        // {GREY, DARK_GREY, BLACK, WHITE, RED_, GREEN, LIGHT_BLUE, BLUE, ORANGE, PURPLE, BROWN, YELLOW, PINK}

        // main color
        uint8_t options_main[10] = {0, 3, 4, 5, 6, 8, 9, 10, 11, 12};
        int main_index = options_main[CryptoRandom::rand_byte(0, 10)];
        uint8_t* main_color = COLORS[main_index];

        // secondary color
        uint8_t options_secondary[3] = {1, 2, 7};
        int secondary_index = options_secondary[CryptoRandom::rand_byte(0, 3)];
        uint8_t* secondary_color = COLORS[secondary_index];

        // define pixels
        vector< pair<uint8_t*, pair<int, int>> > pixels = {
            // legs
            {main_color, {5, 1}},
            {main_color, {7, 1}},
            {main_color, {5, 2}},
            {main_color, {7, 2}},
            {main_color, {5, 3}},
            {main_color, {7, 3}},
            {main_color, {5, 4}},
            {main_color, {7, 4}},
            {main_color, {5, 5}},
            {main_color, {7, 5}},

            // body
            {main_color, {5, 6}},
            {main_color, {6, 6}},
            {main_color, {7, 6}},
            {main_color, {5, 7}},
            {main_color, {6, 7}},
            {main_color, {7, 7}},
            {main_color, {5, 8}},
            {main_color, {6, 8}},
            {main_color, {7, 8}},
            {main_color, {5, 9}},
            {main_color, {6, 9}},
            {main_color, {7, 9}},

            // arms
            {main_color, {2, 4}},
            {main_color, {2, 5}},
            {main_color, {2, 6}},
            {main_color, {2, 7}},
            {main_color, {3, 7}},
            {main_color, {3, 8}},
            {main_color, {4, 8}},
            {main_color, {10, 4}},
            {main_color, {10, 5}},
            {main_color, {10, 6}},
            {main_color, {10, 7}},
            {main_color, {9, 7}},
            {main_color, {9, 8}},
            {main_color, {8, 8}},

            // head
            {main_color, {6, 10}},
            {main_color, {5, 11}},
            {main_color, {6, 11}},
            {main_color, {7, 11}},
            {main_color, {4, 12}},
            {main_color, {5, 12}},
            {main_color, {6, 12}},
            {main_color, {7, 12}},
            {main_color, {8, 12}},
            {main_color, {4, 13}},
            {main_color, {5, 13}},
            {main_color, {6, 13}},
            {main_color, {7, 13}},
            {main_color, {8, 13}},
            {main_color, {4, 14}},
            {main_color, {5, 14}},
            {main_color, {6, 14}},
            {main_color, {7, 14}},
            {main_color, {8, 14}},
            {main_color, {4, 15}},
            {main_color, {5, 15}},
            {main_color, {6, 15}},
            {main_color, {7, 15}},
            {main_color, {8, 15}},
            {main_color, {5, 16}},
            {main_color, {6, 16}},
            {main_color, {7, 16}},

            // eyes
            {secondary_color, {5, 13}},
            {secondary_color, {5, 14}},
            {secondary_color, {7, 13}},
            {secondary_color, {7, 14}}
        };


        // set pixels
        for (auto& pixel : pixels) {
            int index = (pixel.second.second * 20 + pixel.second.first) * 3;
            image_20_by_20[index] = pixel.first[0];
            image_20_by_20[index+1] = pixel.first[1];
            image_20_by_20[index+2] = pixel.first[2];
        }

    }

    static void wizard(uint8_t* image_20_by_20) {

        //   0        1        2      3     4       5        6        7      8      9       10     11     12
        // {GREY, DARK_GREY, BLACK, WHITE, RED_, GREEN, LIGHT_BLUE, BLUE, ORANGE, PURPLE, BROWN, YELLOW, PINK}

        // main color
        uint8_t options_main[9] = {1, 4, 5, 6, 7, 8, 9, 11, 12};
        int main_index = options_main[CryptoRandom::rand_byte(0, 9)];
        uint8_t* main_color = COLORS[main_index];

        // main color accent
        uint8_t main_accent[3];
        memcpy(main_accent, main_color, 3);
        main_accent[2] -= 30;

        // face color
        uint8_t options_secondary[3] = {3, 10};
        int secondary_index = options_secondary[CryptoRandom::rand_byte(0, 2)];
        uint8_t face_color[3];
        memcpy(face_color, COLORS[secondary_index], 3);
        face_color[1] -= 10;

        // nose color
        uint8_t nose_color[3];
        memcpy(nose_color, face_color, 3);
        nose_color[1] -= 20;

        // beard color
        uint8_t options_beard[3] = {0, 3};
        int beard_index = options_beard[CryptoRandom::rand_byte(0, 2)];
        uint8_t* beard_color = COLORS[beard_index];



        // define pixels
        vector< pair<uint8_t*, pair<int, int>> > pixels = {
            // main body
            {main_color, {3, 1}},
            {main_color, {4, 1}},
            {main_color, {5, 1}},
            {main_color, {6, 1}},
            {main_color, {7, 1}},
            {main_color, {8, 1}},
            {main_accent, {9, 1}},
            {main_color, {10, 1}},
            {main_color, {11, 1}},
            {main_color, {4, 2}},
            {main_color, {5, 2}},
            {main_color, {6, 2}},
            {main_color, {7, 2}},
            {main_color, {8, 2}},
            {main_color, {9, 2}},
            {main_color, {10, 2}},
            {main_color, {11, 2}},
            {main_color, {5, 3}},
            {main_color, {6, 3}},
            {main_color, {7, 3}},
            {main_accent, {8, 3}},
            {main_color, {9, 3}},
            {main_color, {10, 3}},
            {main_color, {5, 4}},
            {main_color, {6, 4}},
            {main_accent, {7, 4}},
            {main_color, {8, 4}},
            {main_color, {9, 4}},
            {main_color, {10, 4}},
            {main_color, {3, 4}},
            {main_color, {2, 5}},
            {main_color, {3, 5}},
            {main_color, {4, 5}},
            {main_color, {5, 5}},
            {main_accent, {6, 5}},
            {main_color, {2, 6}},
            {main_color, {3, 6}},
            {main_color, {4, 6}},
            {main_accent, {5, 6}},
            {main_color, {6, 6}},
            {main_color, {4, 7}},
            {main_color, {5, 7}},
            {main_color, {12, 4}},
            {main_color, {9, 5}},
            {main_accent, {10, 5}},
            {main_color, {11, 5}},
            {main_color, {12, 5}},
            {main_color, {13, 5}},
            {main_color, {9, 6}},
            {main_color, {10, 6}},
            {main_accent, {11, 6}},
            {main_color, {12, 6}},
            {main_color, {13, 6}},
            {main_color, {10, 7}},
            {main_color, {11, 7}},

            // beard
            {beard_color, {7, 5}},
            {beard_color, {8, 5}},
            {beard_color, {7, 6}},
            {beard_color, {8, 6}},
            {beard_color, {6, 7}},
            {beard_color, {7, 7}},
            {beard_color, {8, 7}},
            {beard_color, {9, 7}},
            {beard_color, {4, 8}},
            {beard_color, {5, 8}},
            {beard_color, {6, 8}},
            {beard_color, {9, 8}},
            {beard_color, {10, 8}},
            {beard_color, {11, 8}},
            {beard_color, {4, 9}},
            {beard_color, {11, 9}},

            // face
            {face_color, {5, 9}},
            {face_color, {6, 9}},
            {face_color, {9, 9}},
            {face_color, {10, 9}},

            // nose
            {nose_color, {7, 8}},
            {nose_color, {8, 8}},
            {nose_color, {7, 9}},
            {nose_color, {8, 9}},

            // hat
            {main_color, {4, 10}},
            {main_color, {5, 10}},
            {main_color, {6, 10}},
            {main_color, {7, 10}},
            {main_color, {8, 10}},
            {main_color, {9, 10}},
            {main_color, {10, 10}},
            {main_color, {11, 10}},
            {main_color, {5, 11}},
            {main_accent, {6, 11}},
            {main_color, {7, 11}},
            {main_accent, {8, 11}},
            {main_color, {9, 11}},
            {main_accent, {10, 11}},
            {main_color, {5, 12}},
            {main_color, {6, 12}},
            {main_color, {7, 12}},
            {main_color, {8, 12}},
            {main_color, {9, 12}},
            {main_color, {10, 12}},
            {main_color, {6, 13}},
            {main_color, {7, 13}},
            {main_color, {8, 13}},
            {main_color, {9, 13}},
            {main_color, {10, 13}},
            {main_color, {6, 14}},
            {main_color, {7, 14}},
            {main_color, {8, 14}},
            {main_color, {9, 14}},
            {main_color, {7, 15}},
            {main_color, {8, 15}},
            {main_color, {9, 15}},
            {main_color, {7, 16}},
            {main_color, {8, 16}},
            {main_color, {9, 16}},
            {main_color, {8, 17}},
            {main_color, {8, 18}},




        };


        // set pixels
        for (auto& pixel : pixels) {
            int index = (pixel.second.second * 20 + pixel.second.first) * 3;
            image_20_by_20[index] = pixel.first[0];
            image_20_by_20[index+1] = pixel.first[1];
            image_20_by_20[index+2] = pixel.first[2];
        }

    }

    static void skeleton(uint8_t* image_20_by_20) {

        //   0        1        2      3     4       5        6        7      8      9       10     11     12
        // {GREY, DARK_GREY, BLACK, WHITE, RED_, GREEN, LIGHT_BLUE, BLUE, ORANGE, PURPLE, BROWN, YELLOW, PINK}

        // main color
        uint8_t options_main[10] = {0, 3, 4, 5, 6, 8, 9, 10, 11, 12};
        int main_index = options_main[CryptoRandom::rand_byte(0, 10)];
        uint8_t* main_color = COLORS[main_index];

        // define pixels
        vector< pair<uint8_t*, pair<int, int>> > pixels = {
            // legs
            {main_color, {5, 1}},
            {main_color, {7, 1}},
            {main_color, {5, 2}},
            {main_color, {7, 2}},
            {main_color, {5, 3}},
            {main_color, {7, 3}},
            {main_color, {5, 4}},
            {main_color, {7, 4}},
            {main_color, {5, 5}},
            {main_color, {7, 5}},

            // body
            {main_color, {5, 6}},
            {main_color, {6, 6}},
            {main_color, {7, 6}},
            {main_color, {5, 7}},
            {main_color, {6, 7}},
            {main_color, {7, 7}},
            {main_color, {5, 8}},
            {main_color, {6, 8}},
            {main_color, {7, 8}},
            {main_color, {5, 9}},
            {main_color, {6, 9}},
            {main_color, {7, 9}},

            // arms
            {main_color, {2, 4}},
            {main_color, {2, 5}},
            {main_color, {2, 6}},
            {main_color, {2, 7}},
            {main_color, {3, 7}},
            {main_color, {3, 8}},
            {main_color, {4, 8}},
            {main_color, {10, 4}},
            {main_color, {10, 5}},
            {main_color, {10, 6}},
            {main_color, {10, 7}},
            {main_color, {9, 7}},
            {main_color, {9, 8}},
            {main_color, {8, 8}},

            // head
            {main_color, {6, 10}},
            {main_color, {5, 11}},
            {main_color, {6, 11}},
            {main_color, {7, 11}},
            {main_color, {4, 12}},
            {main_color, {5, 12}},
            {main_color, {6, 12}},
            {main_color, {7, 12}},
            {main_color, {8, 12}},
            {main_color, {4, 13}},
            {main_color, {5, 13}},
            {main_color, {6, 13}},
            {main_color, {7, 13}},
            {main_color, {8, 13}},
            {main_color, {4, 14}},
            {main_color, {5, 14}},
            {main_color, {6, 14}},
            {main_color, {7, 14}},
            {main_color, {8, 14}},
            {main_color, {4, 15}},
            {main_color, {5, 15}},
            {main_color, {6, 15}},
            {main_color, {7, 15}},
            {main_color, {8, 15}},
            {main_color, {5, 16}},
            {main_color, {6, 16}},
            {main_color, {7, 16}}
        };


        // set pixels
        for (auto& pixel : pixels) {
            int index = (pixel.second.second * 20 + pixel.second.first) * 3;
            image_20_by_20[index] = pixel.first[0];
            image_20_by_20[index+1] = pixel.first[1];
            image_20_by_20[index+2] = pixel.first[2];
        }

    }



};