#pragma once


using namespace std;


#include <cstdint>

struct Time {

    /*
    Prime Resonances since the start of the universe.
    
    Based on the spin flip of the electron and proton in a hydrogen atom which is 1,420,405,751 Hz or 
    0.0000000007040241841 seconds. A billion oscilations is a 'Prime Resonance' which is 0.7040241841 real seconds.
    */
    uint64_t prime_resonance_since_genesis; // max 18446744073709551615 or 411 billion years

    /*
        Cycle, or Kilo Prime Resonance since last vector day
        ~11.73 minutes per unit
    */
    uint64_t get_cycles() const {
        uint64_t day = this->get_vector_days();
        return (this->prime_resonance_since_genesis - day * 100000ULL) / 1000ULL;
    }

    /*
        Vector day, or 100 cycles
        ~19.56 hours per unit
    */
    uint64_t get_vector_days() const {
        uint64_t nova = this->get_nova();
        return (this->prime_resonance_since_genesis - nova * 100000000ULL) / 100000ULL;
    }


    /*
        Nova, a 100 million primes, or 1000 vector days  
        ~2.23 years per unit
    */
    uint64_t get_nova() const {
        return this->prime_resonance_since_genesis / 100000000ULL;
    }

    /*
        Era, 10,000 novas
        ~22,300 years per unit
    */
    uint64_t get_era() const {
        return this->prime_resonance_since_genesis / 1000000000000ULL;
    }

    /*
        Aeon, 
        ~2.23 billion years per unit
    */
    uint64_t get_aeon() const {
        return this->prime_resonance_since_genesis / 100000000000000000ULL;
    }
};