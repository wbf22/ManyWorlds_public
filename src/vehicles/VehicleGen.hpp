#pragma once


#include <memory>
#include <vector>
#include <cmath>
#include "util/Position.h"
#include "util/Grid.hpp"
#include "blocks/Block.h"
#include "space/Planet.h"
#include "util/PartContour.hpp"
#include "util/Random.h"
#include "util/BMaterial.hpp"
#include "util/CoordinateConversion.hpp"

using namespace std;

enum class VehiclePurpose {
    CARGO,
    MASS_TRANSIT,
    PERSONAL_TRANSIT
};

enum class VehicleTravelType {
    FLUID_FLOAT,
    BALLOON,
    LIFT,
    TERRAIN,
    SPACE
};

struct Lift {
    Vector3 x_moving_lift;
    Vector3 x_neg_moving_lift;
    Vector3 y_moving_lift;
    Vector3 y_neg_moving_lift;
    Vector3 z_moving_lift;
    Vector3 z_neg_moving_lift;
};

struct VehiclePart {
    shared_ptr<Position> pos_on_vehicle;
    shared_ptr<Lift> lift;

    Grid<shared_ptr<Block>> blocks;
    vector<shared_ptr<VehiclePart>> child_parts;

    vector<shared_ptr<Block>> get_all_blocks() {

        vector<shared_ptr<Block>> blocks = this->blocks.values();
        for (shared_ptr<VehiclePart> part : this->child_parts) {
            vector<shared_ptr<Block>> part_blocks = part->blocks.values();
            blocks.insert(blocks.end(), part_blocks.begin(), part_blocks.end());
        }

        return blocks;
    }
};

struct Motor : VehiclePart {

};

struct Wing : VehiclePart {

};

/**
 * A material used on wheels to make them glide over blocks instead of harshly jarring on then
 */
struct DamperMaterial : VehiclePart {

};

struct WheelAssembly : VehiclePart {

};

struct Thruster : VehiclePart {
    enum class ThrusterHeat {
        CHEMICAL, // burning stuff to make hot gas
        NUCLEAR, // heated hydrogen with nuclear
        ANTI_MATTER, // combine matter and antimatter for energy
        NONE
    };

    enum class ThrusterPropellant {
        PLASMA,
        HYDROGEN_GAS,
        ION,
        RADIATION_AND_PARTICALS
    };

    enum class ThrusterType {
        ELECTRIC, // use electricity to spit out ions
        HEAT, // super heat hydrogen or plasma to spit it out
        MAGNETIC, // magnets shoot out plasma

    };

    ThrusterType type;
    ThrusterPropellant propellant;
    ThrusterHeat heat_source; // can be none if thruster type is 'HEAT'

};


struct VehicleGen {


    shared_ptr<Lift> calculate_lift(shared_ptr<VehiclePart> part) {
            // For each principal axis:
            //
            // +X, -X
            // +Y, -Y
            // +Z, -Z
            //
            // Sample silhouette/profile.

            // Measure:
            // - frontal area
            // - camber (top vs bottom)
            // - thickness
            // - leading-edge radius
            // - trailing-edge taper
            // - average curvature
            // - roughness

            // Store coefficients for later runtime use.
    }


    void set_interior_void_blocks(shared_ptr<VehiclePart> part) {
        // find local minima or pockets in the vehicle that might contain voids and set void blocks in those areas

        // (later this can be used to determine bouancy comparing fluid or gas in voids to surrounding fluids or gas. So players can make boats or ballons)
    }


    /**
     * Makes the main shape of the vehicle
     * 
     * The purpose should be determined before this.
     * @param seed_spike seed for deterministic generation
     * @param purpose what the vehicle is for (cargo, transit, personal)
     * @param travel_types how the vehicle gets around
     * @param gravity the max gravity the vehicle needs to overcome if airborne or sea born
     * @param fluid_density the min density of fluids this vehicle is designed to travel through
     * @param gas_density the min desnity of gas this vehicle is designed to travel through
     * @param internal_medium_density the density of the gas or fluid the vehicle will be filled with
     * 
     */
    static shared_ptr<VehiclePart> gen_vehicle(
        int64_t seed_spike,
        VehiclePurpose purpose, 
        vector<VehicleTravelType> travel_types, 
        double occupant_size,
        double gravity, 
        double fluid_density, 
        double gas_density, 
        double internal_medium_density
    ) {
        double block_size = CoordinateConversion::BLOCK_SCALE;

        // ---- determine size ----
        int length, width, height;
        switch (purpose) {
            case VehiclePurpose::CARGO:
                length = Random::randInt(seed_spike+1, 3*occupant_size, 15*occupant_size);
                width = Random::randInt(seed_spike+2, 3*occupant_size, 15*occupant_size);
                height = Random::randInt(seed_spike+3, 3*occupant_size, 15*occupant_size);
                break;
            case VehiclePurpose::MASS_TRANSIT:
                length = Random::randInt(seed_spike+1, 3*occupant_size, 15*occupant_size);
                width = Random::randInt(seed_spike+2, 3*occupant_size, 15*occupant_size);
                height = Random::randInt(seed_spike+3, 3*occupant_size, 15*occupant_size);
                break;
            default:
                length = Random::randInt(seed_spike+1, 1*occupant_size, 4*occupant_size);
                width = Random::randInt(seed_spike+2, 1*occupant_size, 4*occupant_size);
                height = Random::randInt(seed_spike+3, 1*occupant_size, 4*occupant_size);
                break;
        }

        // ---- travel type adjustments ----
        bool covered = true;
        for (auto tt : travel_types) {
            if (tt == VehicleTravelType::FLUID_FLOAT) {
                width *= 1.3;
                height *= 0.85;
                if (Random::randBool(seed_spike+4, 35)) {
                    covered = false;
                }
            }
            if (tt == VehicleTravelType::BALLOON) {
                length *= Random::randDouble(seed_spike+5, 1.6, 3.0);
                width  *= Random::randDouble(seed_spike+6, 1.6, 2.5);
                height *= Random::randDouble(seed_spike+7, 1.6, 2.5);
            }
            if (tt == VehicleTravelType::SPACE) {
                covered = true;
            }
            if (tt == VehicleTravelType::TERRAIN) {
                height *= 0.75;
            }
        }

        int l_blocks = max(4, (int)round(length / block_size));
        int w_blocks = max(3, (int)round(width / block_size));
        int h_blocks = max(3, (int)round(height / block_size));
        if (l_blocks % 2 == 1) l_blocks += 1; // keep even for symmetry

        string hull_mat = (gravity < 5 && Random::randBool(seed_spike+10, 50))
            ? BMaterial::ALUMINUM : BMaterial::IRON;

        double l_double = l_blocks * block_size;
        double w_double = w_blocks * block_size;
        double h_double = h_blocks * block_size;

        // ---- build hull contour ----
        double max_dim = max(w_double, h_double);
        double base_radius = max_dim * 0.5;
        double flatten = 2.0 * h_double / (w_double + h_double);
        flatten = min(1.99, max(0.01, flatten));

        double nose_r = base_radius * 0.2;
        vector<PartDim> hull_dims = {
            {0,                 nose_r,     0, 0, flatten},
            {l_double * 0.12,   base_radius, 0, 0, flatten},
            {l_double * 0.55,   base_radius, 0, 0, flatten},
            {l_double * 0.88,   base_radius * 0.55, 0, 0, flatten},
            {l_double,          nose_r,     0, 0, flatten}
        };

        vector<shared_ptr<Block>> hull_blocks;
        PartContour::interpolate_part_contour(
            l_double, hull_blocks, move(hull_dims), block_size,
            0, "z", true, true
        );

        // ---- convert hull blocks to grid ----
        auto main_part = make_shared<VehiclePart>();
        main_part->pos_on_vehicle = make_shared<Position>(0, 0, 0);

        auto vec_to_grid = [&](const vector<shared_ptr<Block>>& src, Grid<shared_ptr<Block>>& grid, const string& mat) {
            for (auto& b : src) {
                if (!b || !b->position_double) continue;
                Index3 idx = {
                    (int64_t)round(b->position_double->x / block_size),
                    (int64_t)round(b->position_double->y / block_size),
                    (int64_t)round(b->position_double->z / block_size)
                };
                if (!grid.has(idx)) {
                    auto gb = make_shared<Block>();
                    gb->position = make_shared<Position>(idx.x, idx.y, idx.z);
                    gb->position_double = b->position_double;
                    gb->size = block_size;
                    gb->material = mat;
                    grid[idx] = gb;
                }
            }
        };
        vec_to_grid(hull_blocks, main_part->blocks, hull_mat);

        // ---- find the lowest hull block (body floor) ----
        int body_floor_y = 0;
        if (!main_part->blocks.data.empty()) {
            auto it = main_part->blocks.data.begin();
            int fx = it->first.x, fy = it->first.y, fz = it->first.z;
            while (main_part->blocks.has({fx, fy - 1, fz}))
                fy--;
            bool found = true;
            while (found) {
                found = false;
                for (auto& kv : main_part->blocks.data) {
                    if (kv.first.y == fy - 1) {
                        fx = kv.first.x; fy = kv.first.y; fz = kv.first.z;
                        while (main_part->blocks.has({fx, fy - 1, fz}))
                            fy--;
                        found = true;
                        break;
                    }
                }
            }
            body_floor_y = fy;
        }

        // ---- interior cabin (if covered) ----
        if (covered && l_blocks > 6 && w_blocks > 4 && h_blocks > 3) {
            int inset = 1;
            int floor_y = -h_blocks / 2 + inset;
            int ceil_y  =  h_blocks / 2 - inset;
            int x_lo = -w_blocks / 2 + inset;
            int x_hi =  w_blocks / 2 - inset;
            int z_lo = 2;
            int z_hi = l_blocks - 3;

            auto fill_rect = [&](int y, int zs, int ze, int xs, int xe, const string& mat) {
                for (int z = zs; z <= ze; ++z) {
                    for (int x = xs; x <= xe; ++x) {
                        Index3 idx = {x, y, z};
                        if (!main_part->blocks.has(idx)) {
                            auto b = make_shared<Block>();
                            b->position = make_shared<Position>(x, y, z);
                            b->size = block_size;
                            b->material = mat;
                            main_part->blocks[idx] = b;
                        }
                    }
                }
            };

            // ---- windows on left/right hull sides ----
            for (int z = z_lo + 2; z <= z_hi - 2; z += 3) {
                if (!Random::randBool(seed_spike + 100 + z, 55)) continue;
                int win_w = (int)Random::randDouble(seed_spike + 110 + z, 1, min(3, (x_hi - x_lo) / 2));
                int win_h = (int)Random::randDouble(seed_spike + 120 + z, 1, min(2, (ceil_y - floor_y) / 3));
                int wx = x_lo;
                int wz = z;
                int wy_lo = floor_y + 2;
                int wy_hi = wy_lo + win_h;

                // remove hull blocks for window opening on left side
                for (int wy = wy_lo; wy <= wy_hi && wy < ceil_y; ++wy) {
                    for (int woff = 0; woff <= win_w; ++woff) {
                        Index3 hole = {wx, wy, wz + woff};
                        main_part->blocks.data.erase(hole);
                    }
                }

                // mirror window on right side
                int rwx = x_hi;
                for (int wy = wy_lo; wy <= wy_hi && wy < ceil_y; ++wy) {
                    for (int woff = 0; woff <= win_w; ++woff) {
                        Index3 hole = {rwx, wy, wz + woff};
                        main_part->blocks.data.erase(hole);
                    }
                }
            }
        }


        // ---- wings (if LIFT based) ----
        bool lift_based = false;
        for (auto tt : travel_types) {
            if (tt == VehicleTravelType::LIFT) lift_based = true;
        }
        if (lift_based) {
            double span = w_double * Random::randDouble(seed_spike+200, 1.5, 4.0);
            double chord = Random::randDouble(seed_spike+201, 2.0, 5.0);
            int span_blocks = max(2, (int)round(span / block_size));
            int chord_blocks = max(1, (int)round(chord / block_size));
            int n_wings = 2;
            int wing_roll = (int)Random::randDouble(seed_spike+202, 0, 100);
            if (wing_roll < 30) n_wings = 4;
            else if (wing_roll < 50) n_wings = 6;

            int wing_z = l_blocks / 3;
            int wing_y = h_blocks / 4;

            auto build_one_wing = [&](int x_dir, int z_off, int y_off) -> shared_ptr<Wing> {
                auto wing_part = make_shared<Wing>();
                wing_part->pos_on_vehicle = make_shared<Position>(x_dir * (w_blocks / 2 + 1), y_off, z_off);
                string wing_mat = Random::randBool(seed_spike + 210 + x_dir * z_off, 50)
                    ? BMaterial::ALUMINUM : BMaterial::IRON;
                for (int s = 0; s < span_blocks; ++s) {
                    int cx = x_dir * (w_blocks / 2 + 1 + s);
                    int taper = max(1, chord_blocks - s * chord_blocks / span_blocks);
                    for (int c = 0; c < taper; ++c) {
                        Index3 wi = {cx, y_off, z_off + c};
                        auto b = make_shared<Block>();
                        b->position = make_shared<Position>(cx, y_off, z_off + c);
                        b->position_double = make_shared<PositionDouble>(
                            cx * block_size,
                            y_off * block_size,
                            (z_off + c) * block_size
                        );
                        b->size = block_size;
                        b->material = wing_mat;
                        wing_part->blocks[wi] = b;
                    }
                }
                return wing_part;
            };

            main_part->child_parts.push_back(build_one_wing(1, wing_z, wing_y));
            main_part->child_parts.push_back(build_one_wing(-1, wing_z, wing_y));

            if (n_wings >= 4) {
                int wing_z2 = 2 * l_blocks / 3;
                int wing_y2 = h_blocks / 4;
                main_part->child_parts.push_back(build_one_wing(1, wing_z2, wing_y2));
                main_part->child_parts.push_back(build_one_wing(-1, wing_z2, wing_y2));
            }
            if (n_wings >= 6) {
                int wing_z3 = l_blocks / 5;
                main_part->child_parts.push_back(build_one_wing(1, wing_z3, -h_blocks / 4));
                main_part->child_parts.push_back(build_one_wing(-1, wing_z3, -h_blocks / 4));
            }
        }

        // ---- propulsion ----
        int rear_z = l_blocks - 1;
        int thruster_count = 0;
        for (auto tt : travel_types) {
            if (tt == VehicleTravelType::SPACE || tt == VehicleTravelType::LIFT) {
                thruster_count = max(thruster_count, 2);
            }
            if (tt == VehicleTravelType::FLUID_FLOAT) {
                thruster_count = max(thruster_count, 1);
            }
        }

        // thrusters (space / lift)
        if (thruster_count > 0) {
            auto tr_type = Thruster::ThrusterType::HEAT;
            int type_roll = (int)Random::randDouble(seed_spike+300, 0, 100);
            if (type_roll < 30) tr_type = Thruster::ThrusterType::ELECTRIC;
            else if (type_roll < 60) tr_type = Thruster::ThrusterType::MAGNETIC;

            auto tr_prop = Thruster::ThrusterPropellant::HYDROGEN_GAS;
            int prop_roll = (int)Random::randDouble(seed_spike+301, 0, 100);
            if (prop_roll < 25) tr_prop = Thruster::ThrusterPropellant::PLASMA;
            else if (prop_roll < 50) tr_prop = Thruster::ThrusterPropellant::ION;
            else if (prop_roll < 75) tr_prop = Thruster::ThrusterPropellant::RADIATION_AND_PARTICALS;

            auto tr_heat = Thruster::ThrusterHeat::CHEMICAL;
            if (tr_type == Thruster::ThrusterType::ELECTRIC) {
                tr_heat = Thruster::ThrusterHeat::NONE;
            } else {
                int heat_roll = (int)Random::randDouble(seed_spike+302, 0, 100);
                if (heat_roll < 20) tr_heat = Thruster::ThrusterHeat::NUCLEAR;
                else if (heat_roll < 35) tr_heat = Thruster::ThrusterHeat::ANTI_MATTER;
            }

            auto make_thruster = [&](int x_pos) -> shared_ptr<Thruster> {
                auto thr = make_shared<Thruster>();
                thr->type = tr_type;
                thr->propellant = tr_prop;
                thr->heat_source = tr_heat;
                thr->pos_on_vehicle = make_shared<Position>(x_pos, 0, rear_z + 2);
                int nozzle_r = std::max(1, w_blocks / 6);
                for (int dz = 0; dz < 3; ++dz) {
                    for (int dx = -nozzle_r; dx <= nozzle_r; ++dx) {
                        for (int dy = -nozzle_r; dy <= nozzle_r; ++dy) {
                            if (dx*dx + dy*dy > nozzle_r*nozzle_r) continue;
                            Index3 ni = {x_pos + dx, dy, rear_z + 1 + dz};
                            auto b = make_shared<Block>();
                            b->position = make_shared<Position>(x_pos + dx, dy, rear_z + 1 + dz);
                            b->position_double = make_shared<PositionDouble>(
                                (x_pos + dx) * block_size,
                                dy * block_size,
                                (rear_z + 1 + dz) * block_size
                            );
                            b->size = block_size;
                            b->material = BMaterial::TITANIUM;
                            thr->blocks[ni] = b;
                        }
                    }
                }
                // inner glow
                Index3 glow = {x_pos, 0, rear_z + 3};
                auto gb = make_shared<Block>();
                gb->position = make_shared<Position>(x_pos, 0, rear_z + 3);
                gb->position_double = make_shared<PositionDouble>(
                    x_pos * block_size,
                    0.0,
                    (rear_z + 3) * block_size
                );
                gb->size = block_size;
                gb->material = BMaterial::BLUE_STAR;
                thr->blocks[glow] = gb;

                return thr;
            };

            if (thruster_count >= 2) {
                int thruster_spread = std::max(2, w_blocks / 4);
                main_part->child_parts.push_back(make_thruster(thruster_spread));
                main_part->child_parts.push_back(make_thruster(-thruster_spread));
            } else {
                main_part->child_parts.push_back(make_thruster(0));
            }
        }

        // motor + propeller for FLUID_FLOAT
        for (auto tt : travel_types) {
            if (tt == VehicleTravelType::FLUID_FLOAT) {
                auto motor_part = make_shared<Motor>();
                motor_part->pos_on_vehicle = make_shared<Position>(0, -h_blocks/4, rear_z + 1);
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dz = 0; dz < 2; ++dz) {
                        Index3 mi = {0, -h_blocks/4 + dy, rear_z + dz};
                        auto b = make_shared<Block>();
                        b->position = make_shared<Position>(0, -h_blocks/4 + dy, rear_z + dz);
                        b->position_double = make_shared<PositionDouble>(
                            0.0,
                            (-h_blocks/4 + dy) * block_size,
                            (rear_z + dz) * block_size
                        );
                        b->size = block_size;
                        b->material = BMaterial::COPPER;
                        motor_part->blocks[mi] = b;
                    }
                }
                main_part->child_parts.push_back(motor_part);
            }
        }

        // vector<shared_ptr<Block>> blocks = main_part->get_all_blocks();
        // IsometricRenderer::render_s(blocks, "src/TEST/output/RENDER.png", IsoAngle::NE, 32);

        // TERRAIN: wheel assemblies + motors
        for (auto tt : travel_types) {
            if (tt == VehicleTravelType::TERRAIN) {
                int wheel_r = max(1, w_blocks / 8);
                int thickness = max(1, w_blocks / 6);
                int axle_t = max(1, wheel_r / 2);
                int wheel_y = body_floor_y - max(1, wheel_r / 2);
                int front_z = max(2, l_blocks / 4);
                int rear_z = max(2, 3 * l_blocks / 4);
                int side_offset = w_blocks / 2;
                string axel_mat = BMaterial::IRON;
                string motor_mat = BMaterial::COPPER;

                auto wheel_blocks_at = [&](int cx, int cy, int cz, Grid<shared_ptr<Block>>& grid) {
                    for (int dx = -thickness/2; dx <= thickness/2; ++dx) {
                        for (int dy = -wheel_r; dy <= wheel_r; ++dy) {
                            int hz = (int)round(sqrt((double)(wheel_r*wheel_r - dy*dy)));
                            hz = max(hz, 1);
                            for (int dz = -hz; dz <= hz; ++dz) {
                                Index3 di = {cx + dx, cy + dy, cz + dz};
                                auto b = make_shared<Block>();
                                b->position = make_shared<Position>(di.x, di.y, di.z);
                                b->position_double = make_shared<PositionDouble>(
                                    di.x * block_size, di.y * block_size, di.z * block_size
                                );
                                b->size = block_size;
                                b->material = BMaterial::DEFAULT;
                                grid[di] = b;
                            }
                        }
                    }
                };

                auto make_assembly = [&](int wheel_z) -> shared_ptr<WheelAssembly> {
                    auto assy = make_shared<WheelAssembly>();
                    assy->pos_on_vehicle = make_shared<Position>(0, wheel_y, wheel_z);
                    wheel_blocks_at(-side_offset, wheel_y, wheel_z, assy->blocks);
                    wheel_blocks_at(side_offset, wheel_y, wheel_z, assy->blocks);
                    for (int x = -side_offset; x <= side_offset; ++x) {
                        for (int dy = -axle_t/2; dy <= axle_t/2; ++dy) {
                            Index3 ai = {x, wheel_y + dy, wheel_z};
                            auto b = make_shared<Block>();
                            b->position = make_shared<Position>(ai.x, ai.y, ai.z);
                            b->position_double = make_shared<PositionDouble>(
                                ai.x * block_size, ai.y * block_size, ai.z * block_size
                            );
                            b->size = block_size;
                            b->material = axel_mat;
                            assy->blocks[ai] = b;
                        }
                    }
                    return assy;
                };

                auto make_motor = [&](int wheel_z) -> shared_ptr<Motor> {
                    int motor_y_lo = wheel_y + axle_t / 2 + 1;
                    int motor_w = w_blocks / 4 - 1;
                    auto motor = make_shared<Motor>();
                    motor->pos_on_vehicle = make_shared<Position>(0, motor_y_lo, wheel_z);
                    for (int x = -motor_w; x <= motor_w; ++x) {
                        for (int z = wheel_z - 1; z <= wheel_z + 1; ++z) {
                            int hull_y = motor_y_lo;
                            for (int check = motor_y_lo; check <= h_blocks; ++check) {
                                if (main_part->blocks.has({x, check, z})) {
                                    hull_y = check;
                                    break;
                                }
                            }
                            for (int y = motor_y_lo; y < hull_y; ++y) {
                                Index3 mi = {x, y, z};
                                auto b = make_shared<Block>();
                                b->position = make_shared<Position>(mi.x, mi.y, mi.z);
                                b->position_double = make_shared<PositionDouble>(
                                    mi.x * block_size, mi.y * block_size, mi.z * block_size
                                );
                                b->size = block_size;
                                b->material = motor_mat;
                                motor->blocks[mi] = b;
                            }
                        }
                    }
                    return motor;
                };

                main_part->child_parts.push_back(make_assembly(front_z));
                main_part->child_parts.push_back(make_assembly(rear_z));
                main_part->child_parts.push_back(make_motor(front_z));
                main_part->child_parts.push_back(make_motor(rear_z));
            }
        }

        return main_part;
    }


};

