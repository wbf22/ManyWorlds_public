#pragma once


#include "server.h"
#include <string>
#include <cstdint>
#include "util/StellarCoordinate.hpp"




using namespace std;

enum Unit {
    QUADRANT,
    LIGHT_YEAR,
    KILOMETER
};

constexpr int64_t MAX_QUADRANT = 3490729320012;
constexpr int64_t MIN_QUADRANT = -3490729320012;
constexpr uint16_t LIGHT_YEARS_PER_QUADRANT = 10000;
constexpr uint64_t KM_PER_LIGHT_YEAR = 9460730472581;


template<typename T>
struct Kilometer {
    uint64_t x;
    uint64_t y;
    uint64_t z;
    vector<shared_ptr<T>> objects;


    void add(shared_ptr<T> object, StellarCoordinate location) {
        this->objects.push_back(object);
    }

    vector<shared_ptr<T>> getObjects(StellarCoordinate location) {
        return this->objects;
    }

    static string key(uint64_t x, uint64_t y, uint64_t z) {
        return to_string(x) + ":" + to_string(y) + ":" + to_string(z);
    }
};

template<typename T>
struct LightYear {
    uint16_t x;
    uint16_t y;
    uint16_t z;
    unordered_map<string, Kilometer<T>> kilometers;


    void add(shared_ptr<T> object, StellarCoordinate location) {
        // validate bounds
        if (location.km_x > KM_PER_LIGHT_YEAR || location.km_x < 0) {
            throw new runtime_error("Kilometer x out of bounds");
        }
        if (location.km_y > KM_PER_LIGHT_YEAR || location.km_y < 0) {
            throw new runtime_error("Kilometer y out of bounds");
        }
        if (location.km_z > KM_PER_LIGHT_YEAR || location.km_z < 0) {
            throw new runtime_error("Kilometer z out of bounds");
        }
        
        string key = Kilometer<T>::key(location.km_x, location.km_y, location.km_z);
        if (this->kilometers.find(key) == this->kilometers.end()) {
            this->kilometers[key] = {location.km_x, location.km_y, location.km_z, {}};
        }

        this->kilometers[key].add(object, location);

    }

    vector<shared_ptr<T>> getObjects(StellarCoordinate location){
        vector<shared_ptr<T>> objects;
        if (location.km_x > KM_PER_LIGHT_YEAR) {
            for (pair<string, Kilometer<T>> pair_sector : this->kilometers) {
                vector<shared_ptr<T>> kilometer_objects = pair_sector.second.getObjects(location);
                objects.insert(objects.end(), kilometer_objects.begin(), kilometer_objects.end());
            }
        }
        else {
            string key = Kilometer<T>::key(location.km_x, location.km_y, location.km_z);
            if (this->kilometers.find(key) != this->kilometers.end()) {
                vector<shared_ptr<T>> kilometer_objects = this->kilometers[key].getObjects(location);
                objects.insert(objects.end(), kilometer_objects.begin(), kilometer_objects.end());
            }
        }

        return objects;
    }

    static string key(uint16_t x, uint16_t y, uint16_t z) {
        return to_string(x) + ":" + to_string(y) + ":" + to_string(z);
    }
};

template<typename T>
struct Quadrant {

    int64_t x;
    int64_t y;
    int64_t z;
    unordered_map<string, LightYear<T>> light_years;

    void add(shared_ptr<T> object, StellarCoordinate location) {
        // validate bounds
        if (location.light_year_x > LIGHT_YEARS_PER_QUADRANT || location.light_year_x < 0) {
            throw new runtime_error("Light Year x out of bounds");
        }
        if (location.light_year_y > LIGHT_YEARS_PER_QUADRANT || location.light_year_y < 0) {
            throw new runtime_error("Light Year y out of bounds");
        }
        if (location.light_year_z > LIGHT_YEARS_PER_QUADRANT || location.light_year_z < 0) {
            throw new runtime_error("Light Year z out of bounds");
        }
        
        // add to light year
        string key = LightYear<T>::key(location.light_year_x, location.light_year_y, location.light_year_z);
        if (this->light_years.find(key) == this->light_years.end()) {
            this->light_years[key] = {location.light_year_x, location.light_year_y, location.light_year_z, {}};
        }

        this->light_years[key].add(object, location);

    }

    vector<shared_ptr<T>> getObjects(StellarCoordinate location)
    {
        vector<shared_ptr<T>> objects;
        if (location.light_year_x > LIGHT_YEARS_PER_QUADRANT) {
            for (pair<string, LightYear<T>> pair_sector : this->light_years) {
                vector<shared_ptr<T>> light_year_objects = pair_sector.second.getObjects(location);
                objects.insert(objects.end(), light_year_objects.begin(), light_year_objects.end());
            }
        }
        else {
            string key = LightYear<T>::key(location.light_year_x, location.light_year_y, location.light_year_z);
            if (this->light_years.find(key) != this->light_years.end()) {
                vector<shared_ptr<T>> light_year_objects = this->light_years[key].getObjects(location);
                objects.insert(objects.end(), light_year_objects.begin(), light_year_objects.end());
            }
        }

        return objects;
    }


    static string key(int64_t x, int64_t y, int64_t z) {
        return to_string(x) + ":" + to_string(y) + ":" + to_string(z);
    }
    
};


template<typename T>
struct LocationBank {

    unordered_map<string, Quadrant<T>> quadrants;

    void add(shared_ptr<T> object, StellarCoordinate location) {

        // validate bounds
        if (location.quadrant_x > MAX_QUADRANT || location.quadrant_x < MIN_QUADRANT) {
            throw new runtime_error("Quadrant x out of bounds");
        }
        if (location.quadrant_y > MAX_QUADRANT || location.quadrant_y < MIN_QUADRANT) {
            throw new runtime_error("Quadrant y out of bounds");
        }
        if (location.quadrant_z > MAX_QUADRANT || location.quadrant_z < MIN_QUADRANT) {
            throw new runtime_error("Quadrant z out of bounds");
        }
        
        // add to quadrant
        string key = Quadrant<T>::key(location.quadrant_x, location.quadrant_y, location.quadrant_z);
        if (this->quadrants.find(key) == this->quadrants.end()) {
            this->quadrants[key] = {location.quadrant_x, location.quadrant_y, location.quadrant_z, {}};
        }

        this->quadrants[key].add(object, location);

    }

    vector<shared_ptr<T>> get(
        StellarCoordinate coordinate,
        Unit unit,
        int radius
    )
    {
        // make sure radius isn't too large
        if (radius > 6)
            throw new runtime_error("Radius too large, must be 6 or less. Big O (2n-1)^3 iterations for radius size. Use a larger unit if you need a larger radius");

        // walk radius in all directions by unit
        vector<StellarCoordinate> coordinates = walkRadius(
            coordinate, 
            radius, 
            unit
        );

        // get each from the bank
        vector<shared_ptr<T>> objects;
        for (StellarCoordinate& location : coordinates) {

            string key = Quadrant<T>::key(location.quadrant_x, location.quadrant_y, location.quadrant_z);
            if (this->quadrants.find(key) != this->quadrants.end()) {
                vector<shared_ptr<T>> quadrant_objects = this->quadrants[key].getObjects(location);
                objects.insert(objects.end(), quadrant_objects.begin(), quadrant_objects.end());
            }

        }

        return objects;
    }

    vector<StellarCoordinate> walkRadius(
        StellarCoordinate location,
        int radius,
        Unit unit
    ) 
    {
        vector<StellarCoordinate> coordinates;
        if (unit == QUADRANT) {

            location.light_year_x = LIGHT_YEARS_PER_QUADRANT + 1;
            location.light_year_y = LIGHT_YEARS_PER_QUADRANT + 1;
            location.light_year_z = LIGHT_YEARS_PER_QUADRANT + 1;
            location.km_x = KM_PER_LIGHT_YEAR + 1;
            location.km_y = KM_PER_LIGHT_YEAR + 1;
            location.km_z = KM_PER_LIGHT_YEAR + 1;

            for (int x_i = -radius; x_i < radius; ++x_i) {
                for (int y_i = -radius; y_i < radius; ++y_i) {
                    for (int z_i = -radius; z_i < radius; ++z_i) {

                        bool out_of_bound = location.quadrant_x + x_i > MAX_QUADRANT || location.quadrant_x + x_i < MIN_QUADRANT;
                        out_of_bound = out_of_bound || location.quadrant_y + y_i > MAX_QUADRANT || location.quadrant_y + y_i < MIN_QUADRANT;
                        out_of_bound = out_of_bound || location.quadrant_z + z_i > MAX_QUADRANT || location.quadrant_z + z_i < MIN_QUADRANT;
                        if (out_of_bound) continue;

                        StellarCoordinate new_location = location;
                        new_location.quadrant_x += x_i;
                        new_location.quadrant_y += y_i;
                        new_location.quadrant_z += z_i;

                        coordinates.push_back(new_location);
                    }
                }
            }
        }
        else if (unit == LIGHT_YEAR) {

            location.km_x = KM_PER_LIGHT_YEAR + 1;
            location.km_y = KM_PER_LIGHT_YEAR + 1;
            location.km_z = KM_PER_LIGHT_YEAR + 1;

            for (int x_i = -radius; x_i < radius; ++x_i) {
                for (int y_i = -radius; y_i < radius; ++y_i) {
                    for (int z_i = -radius; z_i < radius; ++z_i) {

                        StellarCoordinate new_location = location;
                        bool in_bounds_x = true;
                        stepLightYearCoordinate(
                            new_location.quadrant_x,
                            new_location.light_year_x,
                            x_i,
                            in_bounds_x
                        );
                        bool in_bounds_y = true;
                        stepLightYearCoordinate(
                            new_location.quadrant_y,
                            new_location.light_year_y,
                            y_i,
                            in_bounds_y
                        );
                        bool in_bounds_z = true;
                        stepLightYearCoordinate(
                            new_location.quadrant_z,
                            new_location.light_year_z,
                            z_i,
                            in_bounds_z
                        );

                        if (in_bounds_x && in_bounds_y && in_bounds_z) coordinates.push_back(new_location);

                    }
                }
            }
        }
        else if (unit == KILOMETER) {

            for (int x_i = -radius; x_i < radius; ++x_i) {
                for (int y_i = -radius; y_i < radius; ++y_i) {
                    for (int z_i = -radius; z_i < radius; ++z_i) {

                        StellarCoordinate new_location = location;
                        bool in_bounds_x = true;
                        stepKilometerCoordinate(
                            new_location.quadrant_x,
                            new_location.light_year_x,
                            new_location.km_x,
                            x_i,
                            in_bounds_x
                        );
                        bool in_bounds_y = true;
                        stepKilometerCoordinate(
                            new_location.quadrant_y,
                            new_location.light_year_y,
                            new_location.km_y,
                            y_i,
                            in_bounds_y
                        );
                        bool in_bounds_z = true;
                        stepKilometerCoordinate(
                            new_location.quadrant_z,
                            new_location.light_year_z,
                            new_location.km_z,
                            z_i,
                            in_bounds_z
                        );

                        if (in_bounds_x && in_bounds_y && in_bounds_z) coordinates.push_back(new_location);

                    }
                }
            }
        }

        return coordinates;
    }

    void stepLightYearCoordinate(
        int64_t& quadrent_coordinate,
        uint16_t& coordinate, 
        int offset,
        bool& in_bounds
    ) {

        // step coordinate (handling overflow)
        if (coordinate < abs(offset) && offset < 0) {
            --quadrent_coordinate;
            coordinate = LIGHT_YEARS_PER_QUADRANT + offset + coordinate;
        }
        else if (offset > 0 && coordinate + offset > LIGHT_YEARS_PER_QUADRANT) {
            ++quadrent_coordinate;
            coordinate = offset - (LIGHT_YEARS_PER_QUADRANT - coordinate);
        }
        else {
            coordinate += offset;
        }

        // check if we've stepped out of max or min quadrant
        if (quadrent_coordinate > 3490729320012 || quadrent_coordinate < -3490729320012) {
            in_bounds = false;
        }
        else {
            in_bounds = true;
        }

    }


    void stepKilometerCoordinate(
        int64_t& quadrent_coordinate,
        uint16_t& light_year_coordinate,
        uint64_t& coordinate, 
        int offset,
        bool& in_bounds
    ) {

        // step coordinate (handling overflow)
        if (coordinate < abs(offset) && offset < 0) {
            stepLightYearCoordinate(quadrent_coordinate, light_year_coordinate, -1, in_bounds);
            coordinate = KM_PER_LIGHT_YEAR + offset + coordinate;
        }
        else if (offset > 0 && coordinate + offset > KM_PER_LIGHT_YEAR) {
            stepLightYearCoordinate(quadrent_coordinate, light_year_coordinate, 1, in_bounds);
            coordinate = offset - (KM_PER_LIGHT_YEAR - coordinate);
        }
        else {
            coordinate += offset;
        }

    }


};

