#pragma once

#include "util/Random.h"
#include "util/Util.hpp"
#include "space/Planet.h"
#include "../body/Body.hpp"
#include "util/Grid.hpp"
#include "util/PartContour.hpp"
#include "../world/maps/IsometricRenderer.hpp"


using namespace std;



enum class Diet {
    CARNIVORE,
    OMNIVORE,
    HERBIVORE
};

enum class MovementMethod {
    SWIM, // very common
    FLY, // common (but more with low gravity)
    FLOAT, // less common (situational based on gravity, more at risk to popping)
    WALK, // very common
    JUMP, // less common
    SLITHER_RIPPLE, // common as easy to develop (but better in niche)
    ROLL // very rare (better if planet is smooth or sandy, and no large plants)
};

struct Alien {
    Diet diet;   
    MovementMethod movement;
    shared_ptr<Body> body;
    vector<double> dimensions_length_width_height;
    double block_size;
};


struct AlienGen {

    inline static double MAX_ALIEN_WEIGHT_METRIC_TONS = 2700000;

    static shared_ptr<Alien> gen_alien(shared_ptr<Planet> home_world, int64_t spike) {
        
        shared_ptr<Alien> new_alien = std::make_shared<Alien>();


        // size (rough width/length in block 0.25 that is 0.125m in game or about 5in)
        double max_size_meters = 30;
        double size = Random::randDouble(home_world->seed + spike, 0.0625, max_size_meters / 0.125);
        size *= 9.8 / home_world->gravity;


        // diet
        new_alien->diet = Random::weighted_choice<Diet>({60, 30, 10}, {Diet::HERBIVORE, Diet::OMNIVORE, Diet::HERBIVORE}, home_world->seed + spike + 1);

        // movement method
        double percent_ocean = ((double)home_world->seaLevel - home_world->minHeight) / ((double)home_world->maxHeight - home_world->minHeight);

        bool ocean_creature = Random::randDouble(home_world->seed + spike + 2, 0, 1) < percent_ocean;
        if (home_world->evolutionStage < 3 && !ocean_creature) {
            if (percent_ocean > 0.1) {
                ocean_creature = true;
            }
        }
        if (ocean_creature) {
            vector<MovementMethod> movements = {MovementMethod::SWIM, MovementMethod::SLITHER_RIPPLE, MovementMethod::WALK};
            vector<int> weights = home_world->evolutionStage < 3? vector<int>{20, 40, 40} : vector<int>{80, 10, 10};
            new_alien->movement = Random::weighted_choice(weights, movements, home_world->seed + spike + 1);
        }
        else {

            // FLY, FLOAT, WALK, JUMP, SLITHER_RIPPLE, ROLL
            vector<MovementMethod> movements = {MovementMethod::FLY, MovementMethod::FLOAT, MovementMethod::WALK, MovementMethod::JUMP, MovementMethod::SLITHER_RIPPLE, MovementMethod::ROLL};
            int fly = 20;
            int float_c = 2;
            int walk = 60;
            int jump = 5;
            int slither_ripple = 10;
            int roll = 0;

            // roll has better chance with high gravity or desert planets
            roll += home_world->gravity;
            if (home_world->precipitation < 20) {
                roll += 10;
            }

            // float can happen if low gravity and dense atmosphere (mostly atmosphere)
            float_c = 100 / (1 + pow(1.1, -home_world->atmosphericPressure+60)); // {100}/{1+1.1^{-x+60}}
           

            // fly more common with low gravity and denser atmospheres (mostly atmosphere)
            fly *= 2.5 * log(home_world->atmosphericPressure+1); // 2.5log(x+1)
            fly += 10 / home_world->gravity;

            // scale weights to sum to 100
            vector<int> weights = {fly, float_c, walk, jump, slither_ripple, roll};
            weights = Random::balance_weights(weights);

            new_alien->movement = Random::weighted_choice(weights, movements, home_world->seed + spike + 1);
        }

        // body shape and limbs
        AlienGen::body_dimensions(new_alien, home_world, spike);
        AlienGen::make_body_and_limbs(new_alien, home_world, spike);

        // insulation if cold

        // colors
        
        // sound

        return new_alien;

    }

    static void body_dimensions(shared_ptr<Alien> new_alien, shared_ptr<Planet> home_world, int64_t spike) {

        // body can be larger if herbivore or omnivore
        // herbivores can have longer necks or bodies if low gravity
        // swim gives you more of a fish shape

        
        // WEIGHT

        // determine max weight
        int64_t max_weight_metric_tons = (10000 / pow(home_world->gravity, 2.1)) - 0.14; // (10000 / x^2.1) - 0.14
        if (max_weight_metric_tons > MAX_ALIEN_WEIGHT_METRIC_TONS) max_weight_metric_tons = MAX_ALIEN_WEIGHT_METRIC_TONS;

        // get average weight
        double max_weight_kg = max_weight_metric_tons * 1000.0;
        double average_weight_kg = 100.0/home_world->gravity;

        // increase if herbivore
        if (new_alien->diet == Diet::HERBIVORE) {
            average_weight_kg *= 1.5;
        }

        // get random target weight
        double weight_kg = Random::randDouble(home_world->seed + spike + 1, 0.01, average_weight_kg, max_weight_kg, 90);

        // increase if swimming or rolling animal
        if(new_alien->movement == MovementMethod::SWIM || new_alien->movement == MovementMethod::ROLL) {
            weight_kg *= 2;
            weight_kg = Util::clamp(max_weight_kg, weight_kg);
        }


        // DIMENSIONS
        double volume = weight_kg / 1000.0; // m3
        double cubic_root = std::cbrt(volume);
        double min_dim = cubic_root*0.1;
        double width = Random::randDouble(home_world->seed+spike+2, min_dim, cubic_root);
        double height, length;
        
        double remainder = volume / width;
        if (new_alien->movement == MovementMethod::ROLL) {
            // roll has the same height and length, while width can vary
            double x = std::sqrt(remainder);
            height = x;
            length = x;
        }
        else {
            // length is the same or more than height
            length = Random::randDouble(home_world->seed+spike+3, cubic_root, remainder/min_dim);
            height = remainder / length;
        }

        new_alien->dimensions_length_width_height = {length, width, height};

        new_alien->body = std::make_shared<Body>();
        new_alien->body->weight_kg = weight_kg;

        if (cubic_root > 20) {
            new_alien->block_size = 1.0;
        }
        else if (cubic_root > 5) {
            new_alien->block_size = 0.25;
        }
        else {
            new_alien->block_size = 0.125;
        }

    } 


    static void make_body_and_limbs(shared_ptr<Alien> new_alien, shared_ptr<Planet> home_world, int64_t spike) {


        // XXX: defualt to having a good chance for a humanoid alien on a planet, especially advanced ones
        double length = new_alien->dimensions_length_width_height[0];
        double width = new_alien->dimensions_length_width_height[1];

        double stockiness = width / length;
        double chubbiness = Random::randInt(home_world->seed + spike + 16, 0, 100);


        // LIMBS
        int min_limbs = 0;
        if (new_alien->movement == MovementMethod::WALK) 
            min_limbs = 2;
        else if (new_alien->movement == MovementMethod::SWIM)
            min_limbs = 1;
        int max_limbs = min(200.0, length);
        int num_limbs = Random::randInt(
            home_world->seed + spike + 4,
            min_limbs,
            4,
            max_limbs,
            90
        );
        
        // make even number of limbs (likely)
        if (num_limbs % 2 != 0 && Random::randBool(home_world->seed + spike + 5, 95)) {
            --num_limbs;
        }

        // fly means large wings, possibly multiple wings
        /*
        
        - determine if upright or horizontal body
        - pair limbs on each side of the body
        - extra limb goes on front, back, or top
        - determine limb functions
        - determine limb lengths
        - determine limb directions


        SWIM 
        FLY
        FLOAT 
        WALK 
        JUMP
        SLITHER_RIPPLE
        ROLL

        */
    


        // SHAPE

        if (new_alien->movement != MovementMethod::ROLL) {


            shared_ptr<BodyPart> last_part = nullptr;

            // body
            double body_angle = Random::randDouble(home_world->seed+spike+6, 0, 0, 120);
            vector<shared_ptr<BodyPart>> main_body;
            double max_body_width = width * 2.0;
            double min_body_width = width * 0.5;
            int body_pieces = length / max_body_width;
            body_pieces = max(body_pieces, 1);
            double body_piece_length = Util::ceil_to_precision(length/body_pieces, new_alien->block_size);
            double flatten_amount = 0.5 + abs(90 - body_angle) / 90; // 0 -> 1.5, 45 -> 1, 90 -> 0.5 
            double curr_width = Random::randDouble(home_world->seed+spike-1, min_body_width, max_body_width);
            string body_axis = PartContour::adapt_axis(body_angle);
            length = body_piece_length * body_pieces;
            int total_length_blocks = floor(length / new_alien->block_size);
            for (int b = 0; b < body_pieces; ++b) {
                shared_ptr<BodyPart> body_piece = std::make_shared<BodyPart>();
                body_piece->type = BodyPartType::BODY;
                body_piece->length = body_piece_length;
                double z_offset = body_piece_length * b;
                double y_offset = PartContour::angle_y_offset(body_angle, z_offset/new_alien->block_size, total_length_blocks, body_axis) * new_alien->block_size;
                body_piece->start_pos = std::make_shared<PositionDouble>(0, y_offset, z_offset);
                BodyPart::parent_to_other(last_part, body_piece);
                last_part = body_piece;

                vector<PartDim> dims;
                for (int i = 0; i < body_piece_length; ++i) {
                    PartDim dim = {(double)i, curr_width, 0, 0, flatten_amount};
                    dims.push_back(dim);
                    curr_width += Random::randDouble(home_world->seed+spike+i+b*2, min_body_width, max_body_width);
                    curr_width /= 2;
                }
                interpolate_part_contour(body_piece, dims, new_alien->block_size, body_angle);
                new_alien->body->parts.push_back(body_piece);
                main_body.push_back(body_piece);

                // IsometricRenderer::render_and_save_png(body_piece->blocks, {}, IsoAngle::NE, "src/TEST/output/NE.png", 128);
            }
            

            // LIMBS
             vector<vector<shared_ptr<BodyPart>>> limbs;
            double min_leg_length = Random::randDouble(
                home_world->seed+spike+7, 
                length*0.1,
                length*0.5,
                length
            );
            int step = length / (num_limbs/2);
            if (step < 1) {
                step = 1;
                num_limbs = length;
            }

            // determine relative limb section ratios
            int limb_sections = Random::randInt(home_world->seed + spike + 8, 2, 4);
            vector<double> section_lengths;
            double remaining_length = 1;
            for (int i = 0; i < limb_sections-1; i++) {
                double n_length = Random::randDouble(home_world->seed + spike + i, 0.2, 0.4);
                section_lengths.push_back(n_length);
                remaining_length -= n_length;
            }
            section_lengths.push_back(remaining_length);

            // add each one
            bool is_sprawled = num_limbs > 6;
            bool done_with_legs = false;
            int leg_count = 0;
            for (double i = length-new_alien->block_size; i >= new_alien->block_size; i-=step) {

                double z_pos = Util::round_to_precision(i, new_alien->block_size);
                last_part = main_body[z_pos / body_piece_length];

                // determine if arm or leg
                /*
                    - back limbs are legs
                    - if body_angle is more that 45 then last few limbs have a chance for being arms
                    - if 70 or more than only the bottom limbs are legs and the rest are arms or wings

                */
                BodyPartType type = BodyPartType::LEG;
                if (i == length) {
                    type = BodyPartType::LEG;
                    leg_count += 2;
                    if (body_angle >= 70) {
                        done_with_legs = true;
                    }
                }
                else {
                    // switch to arms more likely if body angle is bigger
                    if (!done_with_legs) {
                        double chance = Random::randDouble(home_world->seed + spike + 7, 0, 100 * body_angle / 120.0, 100);
                        done_with_legs = Random::randBool(home_world->seed + spike + 8, chance);
                    } 

                    if (done_with_legs) {
                        type = BodyPartType::ARM;
                    }
                    else {
                        type = BodyPartType::LEG;
                        leg_count += 2;
                    }
                }

                // make limb
                vector<shared_ptr<BodyPart>> limb;

                double ground_to_body = min_leg_length + last_part->start_pos->y;
                double max_length = is_sprawled? 2*ground_to_body : 1.2*ground_to_body;
                double limb_length = Random::randDouble(home_world->seed+spike+7, ground_to_body, max_length);

                for (int s = 0; s < section_lengths.size(); ++s) {
                    
                    shared_ptr<BodyPart> limb_part = std::make_shared<BodyPart>();
                    shared_ptr<PositionDouble> limb_section_pos = std::make_shared<PositionDouble>(
                        Util::round_to_precision(width, new_alien->block_size), 
                        last_part->start_pos->y, 
                        z_pos
                    );
                    limb_part->type = type;
                    limb_part->length = limb_length * section_lengths[s];
                    limb_part->start_pos = limb_section_pos;
                    limb_section_pos->add(std::make_shared<PositionDouble>(0,-limb_part->length,0));
                    limb.push_back(limb_part);
                }
                limbs.push_back(limb);

                shared_ptr<BodyPart> thigh = limb[0];
                shared_ptr<BodyPart> calf = limb[1];

                // determine last part aligned width for dims
                double body_width = get_cross_section_width(
                    last_part,
                    std::make_shared<PositionDouble>(0, thigh->start_pos->y, thigh->start_pos->z), 
                    std::make_shared<PositionDouble>(-1, 0, 0), 
                    new_alien->block_size
                );

                // determine widths
                double thigh_width = thigh->length * 0.5 * stockiness / 100.0;
                double min_thigh_width = thigh_width * 0.5;
                double flatten_amount = 1 + abs(90 - body_angle) / 180; // 0 -> 1.5, 45 -> 1.25, 90 -> 1 
                double calf_width = thigh->length * 0.3 * stockiness / 100.0;
                double min_calf_width = calf_width * 0.3;

                double thigh_angle = 0.0;
                double calf_angle = 0.0;
                vector<PartDim> thigh_dims;
                vector<PartDim> calf_dims;

                bool knee_forward = Random::randBool(home_world->seed+spike+9+i);
                double thigh_and_calf_length = thigh->length + calf->length;
                // double thigh_z, thigh_y, calf_z, calf_y;
                if (knee_forward) {
                    double max_thigh_y = ground_to_body * section_lengths[0];
                    double n_angle = AlienGen::get_angle_for_leg_piece(thigh->length, max_thigh_y, 90);
                    thigh_angle = Random::randDouble(home_world->seed+spike+248, 90, n_angle);
                    calf_angle = Random::randDouble(home_world->seed+spike+234, 90-(thigh_angle-90), 90);

                    // big balanced quad
                    thigh_dims = {
                        {0, min_thigh_width, -1, 0, flatten_amount},
                        {thigh->length * 0.5, thigh_width, -1, 0, flatten_amount},
                        {thigh->length, body_width, -1, 0, flatten_amount},
                    };                 
                    
                    // add calf muscle  (if not jumper)
                    calf_dims = {
                        {0, min_calf_width, -1, 0, flatten_amount},
                        {calf->length * 0.5, calf_width, -1, 0, flatten_amount},
                        {calf->length, calf_width*1.1, -1, 0, flatten_amount},
                    };
                    
                }
                else {
                    // big back quad
                    double max_thigh_y = ground_to_body * section_lengths[0];
                    double n_angle = AlienGen::get_angle_for_leg_piece(thigh->length, max_thigh_y, -90);
                    thigh_angle = Random::randDouble(home_world->seed+spike+248, n_angle, 90);
                    if (thigh_angle < 0) thigh_angle += 360;
                    thigh_dims = {
                        {0, min_thigh_width, -4, 0, flatten_amount},
                        {thigh->length * 0.5, thigh_width, -4, 0, flatten_amount},
                        {thigh->length, body_width, -4, 0, flatten_amount},
                    };

                    // small muscle  (if not jumper)
                    calf_angle = Random::randDouble(home_world->seed+spike+234, 90-(thigh_angle-90), 90);
                    calf_dims = {
                        {0, min_calf_width, -1, 0, flatten_amount},
                        {calf->length * 0.5, calf_width, -1, 0, flatten_amount},
                        {calf->length, calf_width*1.1, -1, 0, flatten_amount},
                    };
                }

                AlienGen::interpolate_part_contour(
                    thigh, 
                    thigh_dims,
                    new_alien->block_size,
                    thigh_angle,
                    "z",
                    true
                );   
                AlienGen::interpolate_part_contour(
                    calf, 
                    calf_dims,
                    new_alien->block_size,
                    calf_angle
                );  

                shared_ptr<BodyPart> foot = limb.size() > 2? limb[2] : nullptr;
                if (foot != nullptr) {
                    double foot_angle = Random::randDouble(home_world->seed+spike+248, 90, 180);
                    double foot_width = foot->length * 0.2 * stockiness / 100.0;
                    double flatten_amount = 1; 
                    vector<PartDim> dims = {
                        {0, foot_width, -1, 0, flatten_amount},
                        {foot->length, foot_width*1.1, -1, 0, flatten_amount},
                    };
                    AlienGen::interpolate_part_contour(
                        foot, 
                        dims,
                        new_alien->block_size,
                        foot_angle
                    );
                    // IsometricRenderer::render_and_save_png(foot->blocks, {}, IsoAngle::NE, "src/TEST/output/NE.png", 256);

                }
                
                // alter limb part positions from angles
                double thigh_y = thigh->length * sin(thigh_angle * M_PI / 180.0);
                double thigh_z = thigh->length * cos(thigh_angle * M_PI / 180.0);
                thigh_y = Util::floor_to_precision(thigh_y, new_alien->block_size);
                thigh_z = Util::floor_to_precision(thigh_z, new_alien->block_size);
                double calf_y = calf->length * sin(calf_angle * M_PI / 180.0);
                double calf_z = calf->length * cos(calf_angle * M_PI / 180.0);
                calf_y = Util::floor_to_precision(calf_y, new_alien->block_size);
                calf_z = Util::floor_to_precision(calf_z, new_alien->block_size);
                calf->start_pos->y += -thigh_y;
                calf->start_pos->z += thigh_z;
                foot->start_pos->y += -thigh_y - calf_y;
                foot->start_pos->z += thigh_z + calf_z;

                // parent to body sections and add to body
                BodyPart::parent_to_other(last_part, thigh);
                new_alien->body->parts.push_back(thigh);
                BodyPart::parent_to_other(thigh, calf);
                new_alien->body->parts.push_back(calf);
                if (foot != nullptr) {
                    BodyPart::parent_to_other(calf, foot);
                    new_alien->body->parts.push_back(foot);
                }
            
                
                // duplicate legs on the mirrored side of the body
                if (type == BodyPartType::LEG) {
                    vector<shared_ptr<BodyPart>> mirrored_limb = duplicate_legs(new_alien, limb);
                    limbs.push_back(mirrored_limb);
                }
            
            }


            // neck
            vector<shared_ptr<BodyPart>>  neck;
            double neck_length = Random::randDouble(home_world->seed+spike+14, 0.0, 2.0 * new_alien->block_size, length);
            double section_length = Util::ceil_to_precision(neck_length / width, new_alien->block_size);
            int neck_pieces = neck_length / section_length;
            neck_length = neck_pieces * section_length;
            int neck_total_length_blocks = floor(length / new_alien->block_size);
            double neck_angle = Random::randDouble(home_world->seed+spike+15, 0, 0, 120);
            string neck_axis = PartContour::adapt_axis(neck_angle);
            double min_neck_width = max(new_alien->block_size, 0.1 * min_body_width);
            double max_neck_width = max(new_alien->block_size, 0.5 * max_body_width);
            shared_ptr<BodyPart> body_start = main_body[0];
            last_part = main_body[0];
            if (neck_length > 1) {
                double curr_width = get_cross_section_width(body_start, body_start->start_pos, std::make_shared<PositionDouble>(-1,0,0), new_alien->block_size);
                for (int n = 0; n < neck_pieces; ++n) {

                    shared_ptr<BodyPart> neck_section = std::make_shared<BodyPart>();
                    neck_section->type = BodyPartType::NECK;
                    neck_section->length = section_length;
                    double n_offset = n * section_length;
                    double length_offset = PartContour::angle_y_offset(body_angle, n_offset/new_alien->block_size, neck_total_length_blocks, neck_axis) * new_alien->block_size;
                    neck_section->start_pos = std::make_shared<PositionDouble>(
                        0,
                        neck_axis == "z" ? length_offset : n_offset + body_start->start_pos->y,
                        neck_axis == "y" ? length_offset : n_offset
                    );
                    BodyPart::parent_to_other(last_part, neck_section);
                    last_part = neck_section;

                    double flatten_amount = 1.0 + abs(90 - neck_angle) / 180; // 0 -> 1.5, 45 -> 1.25, 90 -> 1.0 
                    vector<PartDim> dims;
                    for (int i = 0; i < neck_section->length; ++i) {
                        PartDim dim = {(double)i, curr_width, 0, 0, flatten_amount};
                        dims.push_back(dim);
                        curr_width += Random::randDouble(home_world->seed+spike+i, min_neck_width, max_neck_width);
                        curr_width /= 2;
                    }

                    interpolate_part_contour(neck_section, dims, new_alien->block_size, body_angle);
                    new_alien->body->parts.push_back(neck_section);
                    neck.push_back(neck_section);

                    IsometricRenderer::render_and_save_png(neck_section->blocks, {}, IsoAngle::NE, "src/TEST/output/NE.png", 256);
                }
            }

            // head
            shared_ptr<BodyPart> head = std::make_shared<BodyPart>();
            head->type = BodyPartType::HEAD;
            head->length = length * Random::randDouble(home_world->seed+spike+234, 0.02, 0.5);
            head->start_pos = std::make_shared<PositionDouble>(last_part->start_pos->x, last_part->start_pos->y, last_part->start_pos->z);
            BodyPart::parent_to_other(head, last_part);

            flatten_amount = Random::randDouble(home_world->seed+spike+2, 0.5, 1.5);
            vector<PartDim> dims;
            curr_width = max_neck_width * Random::randDouble(home_world->seed+spike+3, 0, 2);
            curr_width = max(new_alien->block_size, curr_width);
            for (int i = 0; i < head->length / new_alien->block_size; ++i) {
                PartDim dim = {(double)i * new_alien->block_size, curr_width, 0, 0, flatten_amount};
                dims.push_back(dim);
                curr_width += curr_width * Random::randDouble(home_world->seed+spike+i, 0.9, 1.1);
                curr_width /= 2;
            }
            double head_angle = Random::randDouble(home_world->seed+spike+33, 90, 180);
            interpolate_part_contour(head, dims, new_alien->block_size, head_angle);
            new_alien->body->parts.push_back(head);

            // IsometricRenderer::render_and_save_png(head->blocks, {}, IsoAngle::NE, "src/TEST/output/NE.png", 256);


            // tail
            vector<shared_ptr<BodyPart>> tail;
            double tail_length = Random::randDouble(home_world->seed+spike+14, 0.0, 1.0, length);
            double tail_section_length = tail_length / min_body_width;
            int tail_sections = tail_length / tail_section_length;
            double min_tail_width = max(new_alien->block_size, 0.1 * min_body_width);
            double max_tail_width = max(new_alien->block_size, 0.5 * max_body_width);
            double tail_angle = body_angle;
            shared_ptr<BodyPart> main_body_end = main_body[main_body.size()-1];;
            last_part = main_body_end;
            if (tail_length > 1) {
                for (int t = 0; t < tail_sections; ++t) {
                    shared_ptr<BodyPart> tail_section = std::make_shared<BodyPart>();
                    tail_section->type = BodyPartType::NECK;
                    tail_section->length = tail_section_length;
                    tail_section->start_pos = std::make_shared<PositionDouble>(0,0,body_pieces*body_piece_length);
                    BodyPart::parent_to_other(last_part, tail_section);

                    double flatten_amount = 1.0 + abs(90 - body_angle) / 180; // 0 -> 1.5, 45 -> 1.25, 90 -> 1.0
                    double curr_width = Random::randDouble(home_world->seed+spike+17, min_tail_width, max_tail_width);
                    vector<PartDim> dims;
                    for (int i = 0; i < tail_section->length; ++i) {
                        PartDim dim = {(double)i, curr_width, 0, 0, flatten_amount};
                        dims.push_back(dim);
                        curr_width += Random::randDouble(home_world->seed + spike + i, min_tail_width, max_tail_width);
                        curr_width /= 2;
                    }

                    if (Random::randBool(home_world->seed + spike + 241 + t, 5)) {
                        double new_angle = Random::randDouble(home_world->seed + spike + 890, 0, tail_angle, 360);
                        tail_angle = tail_angle * 0.9 + new_angle * 0.1;
                    }
                    double diff = main_body_end->start_pos->y - tail_section->start_pos->y;
                    double flatten_curve_amount = (main_body_end->start_pos->y - diff) / main_body_end->start_pos->y;
                    tail_angle = tail_angle * (1-flatten_curve_amount) + 180.0 * flatten_curve_amount;

                    interpolate_part_contour(tail_section, dims, new_alien->block_size, tail_angle);
                    new_alien->body->parts.push_back(tail_section);
                    tail.push_back(tail_section);

                    // IsometricRenderer::render_and_save_png(tail_section->blocks, {}, IsoAngle::NE, "src/TEST/output/NE.png", 256);

                }
            }



            vector<shared_ptr<Block>> test_all_blocks = new_alien->body->get_all_blocks();
            IsometricRenderer::render_and_save_png(test_all_blocks, {}, IsoAngle::NE, "src/TEST/output/NE.png", 256);


        }
        else {
            // roll?

        }
        




    } 

    static double get_angle_for_leg_piece(double leg_piece_length, double max_y_length, double min_angle) {
        
        // like knee forward thigh
        if (min_angle >= 90) {
            if (max_y_length > 0) {
                if (max_y_length > leg_piece_length) return 90;
                return 90 + asin(max_y_length/leg_piece_length) / M_PI * 180.0;
            }
            else {
                if (abs(max_y_length) > leg_piece_length) return 270;
                return 180 + asin(abs(max_y_length)/leg_piece_length) / M_PI * 180.0;
            }
        }
        // like knee backward thigh
        else {
            if (max_y_length > 0) {
                if (max_y_length > leg_piece_length) return 90;
                return asin(max_y_length/leg_piece_length) / M_PI * 180.0;
            }
            else {
                if (abs(max_y_length) > leg_piece_length) return -90; // +360 since we don't use negative angles, but useful for thinking of here
                return -90 + asin(abs(max_y_length)/leg_piece_length) / M_PI * 180.0;
            }
        }
    }

    static void animate(vector<shared_ptr<BodyPart>> limb, shared_ptr<Alien> alien, bool is_sprawled) {

        // run
        if (limb.size() == 3) {

        }
        else if (limb.size() == 2) {

        }


        // Idle

        // hit

        // fall

        // magic

        // swim

        // fly

        // hurt
    }


    static void interpolate_part_contour(shared_ptr<BodyPart> part, vector<PartDim> part_dims, double block_size, double angle, string axis="z", bool round_ends=false) {
        PartContour::interpolate_part_contour(part->length, part->blocks, move(part_dims), block_size, angle, axis, round_ends, false);
    }

    static void interpolate_part_contour_hollow(shared_ptr<BodyPart> part, vector<PartDim> part_dims, double block_size, double angle, string axis="z", bool round_ends=false) {
        PartContour::interpolate_part_contour(part->length, part->blocks, move(part_dims), block_size, angle, axis, round_ends, true);
    }

    static vector<shared_ptr<BodyPart>> duplicate_legs(shared_ptr<Alien> alien, vector<shared_ptr<BodyPart>>& limb) {

        vector<shared_ptr<BodyPart>> mirrored_limb;
        if (limb.empty()) {
            return mirrored_limb;
        }

        shared_ptr<BodyPart> last_part = limb[0]->parent_part;

        for (shared_ptr<BodyPart>& part : limb) {
            shared_ptr<BodyPart> new_part = std::make_shared<BodyPart>();
            new_part->type = part->type;
            new_part->length = part->length;
            new_part->mass = part->mass;
            new_part->currentRotation = part->currentRotation;
            new_part->start_pos = std::make_shared<PositionDouble>(
                -part->start_pos->x,
                part->start_pos->y,
                part->start_pos->z
            );

            for (shared_ptr<Block>& block : part->blocks) {
                new_part->blocks.push_back(block->duplicate());
            }

            BodyPart::parent_to_other(last_part, new_part);
            alien->body->parts.push_back(new_part);
            mirrored_limb.push_back(new_part);
            last_part = new_part;
        }

        return mirrored_limb;
    }

    static double get_cross_section_width(shared_ptr<BodyPart> part, shared_ptr<PositionDouble> pos, shared_ptr<PositionDouble> direction, double block_size) {

        // load part blocks in grid map
        shared_ptr<Block> start_block;
        unordered_map<string, shared_ptr<Block>> blocks_from_part;
        for (shared_ptr<Block> block : part->blocks) {
            string tag = grid_tag(block, part);
            blocks_from_part[tag] = block;
        }

        // convert position to block offset
        shared_ptr<Position> pos_r = std::make_shared<Position>(
            Util::round_to_precision(pos->x, block_size),
            Util::round_to_precision(pos->y, block_size),
            Util::round_to_precision(pos->z, block_size)
        );

        // travel along direction
        bool hit_empty = false;
        double i = 0;
        while(!hit_empty) {
            shared_ptr<Position> offset = pos_r->mult(i);
            string tag = grid_tag(pos_r, offset->x, offset->y, offset->z);   
            if (blocks_from_part.find(tag) == blocks_from_part.end()) {
                hit_empty = true;
            }
            ++i;
        }

        return i / 2;
    }


	static shared_ptr<Position> calc_relative_position_in_size_blocks(shared_ptr<Block> block, shared_ptr<BodyPart> parent_part) {
        double x = block->position_double->x;
        double y = block->position_double->y;
        double z = block->position_double->z;

        x += parent_part->start_pos->x;
        y += parent_part->start_pos->y;
        z += parent_part->start_pos->z;

		return std::make_shared<Position>(
			round(x / block->size), 
			round(y / block->size), 
			round(z / block->size)
		);
	}

	static string grid_tag(shared_ptr<Block> block, shared_ptr<BodyPart> parent_part, int64_t offset_x=0, int64_t offset_y=0, int64_t offset_z=0){
        
		shared_ptr<Position> pos = calc_relative_position_in_size_blocks(block, parent_part);
		return grid_tag(pos, offset_x, offset_y, offset_z);
	}

	static string grid_tag(shared_ptr<Position> pos, int64_t offset_x=0, int64_t offset_y=0, int64_t offset_z=0){
        
		char buf[96]; // enough for four 64-bit ints + spaces
		int len = snprintf(buf, sizeof(buf), "%" PRId64 " %" PRId64 " %" PRId64, pos->x+offset_x, pos->y+offset_y, pos->z+offset_z);
		return string(buf, len);
	}


};
