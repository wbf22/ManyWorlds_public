#pragma once


using namespace std;


#include <cstdint>

struct Distance {

    /*
    Prime Length: the distance light travels in one Prime Resonance, scaled to ~2.11 cm
    
    Prime Resonance period = 1,000,000,000 / 1,420,405,751 Hz = 0.7040241841 s.
    Light travels c × T_prime = 211,061,141 m in one PR.
    Dividing by 10^10 gives ~2.11 cm — the fundamental distance unit.

    It actually pretty much the width of a finger interestingly.
    */
    uint64_t prime_length; // max 18446744073709551615 or ~41 light years

    /*
        Prem, or 10 prime lengths
        ~21.1 cm per unit
    */
    uint64_t get_prems() const {
        return this->prime_length / 20ULL;
    }


    /*
        Cubit, or 20 prime lengths
        ~42.2 cm per unit
        (basically our size 1 block)
    */
    uint64_t get_cubits() const {
        return this->prime_length / 20ULL;
    }


    /*
        Beam, or 100 prime lengths
        ~2.11 m per unit
    */
    uint64_t get_beams() const {
        return this->prime_length / 100ULL;
    }

    /*
        Jaunt, or 1000 prime lengths
        ~21.1 m per unit
    */
    uint64_t get_jaunt() const {
        return this->prime_length / 1000ULL;
    }

    /*
        League, or 100,000 prime lengths
        ~2.11 km per unit
    */
    uint64_t get_leagues() const {
        return this->prime_length / 100'000ULL;
    }

    /*
        Horizon, or 100 million prime lengths
        ~2,110 km per unit
    */
    uint64_t get_horizons() const {
        return this->prime_length / 100'000'000ULL;
    }

    /*
        Orbit, or 10^12 prime lengths
        ~21.1 million km per unit
    */
    uint64_t get_orbits() const {
        return this->prime_length / 1'000'000'000'000ULL;
    }

    /*
        Light Span, or 10^18 prime lengths
        ~2.23 light years per unit
    */
    uint64_t get_light_spans() const {
        return this->prime_length / 1'000'000'000'000'000'000ULL;
    }
};
