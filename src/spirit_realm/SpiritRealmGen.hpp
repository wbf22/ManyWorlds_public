#pragma once


#include "../server/Interface.hpp"
#include "space/SpaceGen.hpp"
#include "util/StellarCoordinate.hpp"


using namespace std;



struct SpiritRealmGen {

    void gen(shared_ptr<Universe> universe, StellarCoordinate location) {



        /*


        space is stretched by gravity, but otherwise almost non existant if little gravity is present
        dark matter is visible in the spirit realm, with celestial bodies having a spirit representation. There can also be other spiritual entities and objects

        good spiritual beings glow while bad ones are often distorted
        */


        /*

        generate space with space gen as needed.

        generate nearby space based on distance from nearby gravitational bodies
        - continuums
        - chambers
        - galaxies
        - clusters
        - solar systems
        - moon and planets or asteriods


        intially generate a low res version of the object and the increase the complexity as you get nearer. 
        Should have a minimum resolution as these will all be drawn onto the skybox
        Once you get close to a celestial body such as a planet or star, then we'll transition to world gen in space mode
        But we'll still show blocks in a sort of dark matter spirit like way

        */
    }



    double calc_stellar_coordinate_change(
        shared_ptr<Universe> universe, 
        StellarCoordinate last_location, 
        double x, // delta movements
        double y, 
        double z
    ) {
        // determine nearby galaxies, solar systems, and planets/moons

        // calculate the gravitational influence at that point

        // multiply that by some constant or use an equation to get a speed

        // in the void of space where gravitational influence is near 0, a 1 milllion light years is only 1 meters in game
        // with a gravitational influence of 1 m/s^2 (near the surface of an object) a 1000km is 1 meter in game
        // with a graviation influence of 10 m/s^2 1km is 1 meter


    }


};

