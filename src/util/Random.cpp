// Fill out your copyright notice in the Description page of Project Settings.


#include "Random.h"

Random::Random(int64_t seed)
{

}

Random::~Random()
{
}

int64_t Random::adjustedSeed(int64_t inputSeed)
{
    inputSeed = (abs(inputSeed) < 20000) ? inputSeed * 123456789 : inputSeed;
    return (inputSeed == 0) ? 123456789 : inputSeed;
}


int Random::hardRandInt(int64_t seed, int64_t min, int64_t max, int iterations)
{
    int value = 0;
    for (int i = 0; i < iterations; i++) {
        value = Random::randInt(seed + value, min, max);
    }
    return value;
}


int64_t Random::rotl64(int64_t x, unsigned int n) {
    const unsigned int bits = 64;
    n %= bits;  // Ensure rotation count is within 0-63
    return (x << n) | ((uint64_t)x >> (bits - n));
}

int64_t Random::rotr64(int64_t x, unsigned int n) {
    const unsigned int bits = 64;
    n %= bits;
    return ((uint64_t)x >> n) | (x << (bits - n));
}


int64_t Random::randInt(int64_t seed, int64_t min, int64_t max) {


    // old function
    /*
+    int64_t spike = seed * 123;
+
     if (min >= max) return min;

+    int64_t diff = abs(max - min);
+    spike = abs(spike) < 20000 ? spike * 123456789 : spike;
+    spike = spike == 0 ? 987654321 : spike;
+    spike = spike >> 1;
+
+    // Randomize
+    int64_t rots = spike ^ rotl64(spike, 10) ^ rotl64(spike, 26) ^ rotl64(spike, 31);
+    int64_t o_e = seed ^ rots ^ 2654435761 ^  2147483647;
+    
+    // Clamp between min and max
+    int64_t value = (int64_t) (abs(o_e) % diff);
 
     return value + min;

    
    */

    if (min >= max) return min;

    uint64_t diff = (uint64_t)max - (uint64_t)min;

    // 2-round Feistel — bijective, small seeds distributed across all bits
    uint64_t s = (uint64_t)seed ^ 0x9e3779b97f4a7c15ULL;
    uint32_t lo = (uint32_t)s;
    uint32_t hi = (uint32_t)(s >> 32);
    hi ^= (uint32_t)((uint64_t)lo * 0x9e3779b9);
    lo ^= (uint32_t)((uint64_t)hi * 0x9e3779b9);
    uint64_t z = ((uint64_t)hi << 32) | lo;

    z ^= z >> 13 ^ z >> 27 ^ z >> 31;

    int64_t value = (int64_t)(z % diff);
    return value + min;
}


int64_t Random::randInt(int64_t seed, int64_t min, int64_t average, int64_t max)
{

    int FULL_PERCENT = 10'000'000;
    int HALF_PERCENT = 5'000'000;

    double percent = Random::randInt(seed, 0, FULL_PERCENT);
    
    int64_t value;
    if (percent < HALF_PERCENT) {
        int64_t offset = (average - min) * (percent / HALF_PERCENT);
        value = offset + min;
    }
    else {
        int64_t offset = (max - average) * ((percent - HALF_PERCENT) / HALF_PERCENT);
        value = average + offset;
    }
    

    double weight_avg = Random::randInt(seed, 0, 100) / 100.0;
    return value * (1-weight_avg) + average * weight_avg; 


    // int64_t first = Random::randInt(seed, min, max);
    // int64_t second = Random::randInt(seed + 23, min, max);

    // return (std::abs(first - average) < std::abs(second - average))? first : second;
}


int64_t Random::randInt(int64_t seed, int64_t min, int64_t average, int64_t max, double averageStrength) {
    int64_t value = Random::randInt(seed, min, max);
    int64_t averageweight = Random::randInt(seed + 23, 0, 100);
    if (averageweight != 0) {
        double diff = averageStrength / averageweight;
        int64_t mult = Random::randInt(seed + 39, 0, 100);
        if (diff < 1) {
            /*
            if the diff is less than one

            if mult is 100 we want to leave diff as is
            if mult is 0 we want diff to be 1
            */
           diff += (1-diff) * (100-mult)/100.0;
        }
        else {
            diff *= mult/100.0;
        }
        averageweight *= diff;
    }

    int64_t valueWeight = 100 - averageweight;

    return (value * valueWeight + average * averageweight) / 100;
}

int RAND_DOUBLE_PRECISION = 100000;

double Random::randDouble(int64_t seed, double min, double max) {
    return Random::randInt(seed, min * RAND_DOUBLE_PRECISION, max * RAND_DOUBLE_PRECISION) / (double) RAND_DOUBLE_PRECISION;
}


double Random::randDouble(int64_t seed, double min, double average, double max) {
    return Random::randInt(seed, min * RAND_DOUBLE_PRECISION, average * RAND_DOUBLE_PRECISION, max * RAND_DOUBLE_PRECISION) / (double) RAND_DOUBLE_PRECISION;
}


double Random::randDouble(int64_t seed, double min, double average, double max, double averageStrength) {

    if (averageStrength > 100 || averageStrength < 0) {
        cout << "averageStrength must be between 0 and 100" << endl;
        // checkNoEntry();
    }

    double value = Random::randInt(seed, min * RAND_DOUBLE_PRECISION, average * RAND_DOUBLE_PRECISION, max * RAND_DOUBLE_PRECISION) / (double) RAND_DOUBLE_PRECISION;

    // get random weights for the average and the value
    double averageweight = Random::randInt(seed + 23, 0, 100) * averageStrength;
    if (averageweight > 100) {
        float fudgeFactor = averageweight / 10000.0;
        fudgeFactor = 1 - fudgeFactor;
        averageweight = 100 - fudgeFactor;
    }
    double valueWeight = 100 - averageweight;

    return (value * valueWeight + average * averageweight) / 100.0;
}


int Random::cheapRandom(int seed, int min, int max) {
    seed += max;
    seed -= min;
    int diff = max - min;
    seed = seed % diff;
    return seed + min;
}


bool Random::randBool(int64_t seed)
{
    return Random::randInt(seed, 0, 100) < 50;
}

bool Random::randBool(int64_t seed, int amount_true_in_100)
{
    return Random::randInt(seed, 0, 100) < amount_true_in_100;
}

vector<int64_t> Random::possible_seeds(shared_ptr<Params> params, int64_t& index) {

    vector<int64_t> possible_seeds;
    int64_t min = params->min;
    int64_t max = params->max;
    int64_t value = params->value - min;
    uint64_t diff = (uint64_t)max - (uint64_t)min;

    uint64_t z = (uint64_t)value + (uint64_t)index * diff;

    // Inverse Feistel
    uint32_t hi = (uint32_t)(z >> 32);
    uint32_t lo = (uint32_t)z;
    lo ^= (uint32_t)((uint64_t)hi * 0x9e3779b9);
    hi ^= (uint32_t)((uint64_t)lo * 0x9e3779b9);
    uint64_t s = ((uint64_t)hi << 32) | lo;
    int64_t seed = (int64_t)(s ^ 0x9e3779b97f4a7c15ULL);
    possible_seeds.push_back(seed);

    int64_t test = randInt(seed, min, max);
    if (test != params->value) {
        cout << "miss" << endl;
    }

    return possible_seeds;
}


int64_t Random::reverse(vector<shared_ptr<Params>> param_sets)
{

    int64_t seed;
    bool valid = false;
    int64_t index = 0;

    while(!valid) {

        vector<int64_t> pos_seeds = {};
        while(pos_seeds.empty()) {
            pos_seeds = Random::possible_seeds(param_sets[0], index);
            ++index;
            // if (index % 1000 == 0) cout << index << endl;
        }

        for (int64_t p_seed : pos_seeds) {

            bool match = true;
            for (int i = 1; i < param_sets.size(); ++i) {

                shared_ptr<Params> params = param_sets[i];
                int64_t value = params->value;
                int64_t min = params->min;
                int64_t max = params->max;

                int64_t out = Random::randInt(p_seed, min, max);
                match = out == value;
                match = match || abs(out - value) < params->leeway;
                if (!match) {
                    break;
                }
                seed = p_seed;

            }
            if (match) {
                valid = true;
                break;
            }

        }
        
    }

    // cout << "iterations " << index << endl;
    cout << index << endl;
    
    return seed;
}


int64_t Random::iterations_to_reverse(int num_params, int64_t average_range, int64_t max_range, int64_t average_leeway) {
    double safe_average_range = max_range * 0.9 + average_range * 0.1;
    double chance_for_hit_one_param = (1.0 + average_leeway) / safe_average_range;
    if (chance_for_hit_one_param > 1) chance_for_hit_one_param = 1;

    double chance_per_iteration = pow(1.7 * chance_for_hit_one_param, num_params-1);

    // when measured on my pc it was about 1413992.286 i/sec
    return (int64_t) 1.0 / chance_per_iteration;
}

vector<int> Random::make_weights(int start, int end, int num_weights)
{

    // collect weights
    double step = (end - start) / num_weights;
    vector<int> weights;
    double current = start;
    double sum = 0;
    for (int i = 0; i < num_weights; i++) {
        int new_weight = round(current);
        weights.push_back(new_weight);
        current += step;
        sum += new_weight;
    }

    // scale to sum to 100
    for (int i = 0; i < num_weights; i++) {
        weights[i] = round(
            100 * weights[i] / sum
        );
    }


    return weights;
}

vector<int> Random::balance_weights(vector<int> weights)
{
    double sum = 0;
    for (int i = 0; i < weights.size(); i++) {
        sum += weights[i];
    }

    // scale to sum to 100
    for (int i = 0; i < weights.size(); i++) {
        weights[i] = round(
            100 * weights[i] / sum
        );
    }
    return weights;
}

int64_t Random::seed_from_coordinates(int64_t x, int64_t y, int64_t z)
{
    int64_t seed = Random::rotl64(x, 32) ^ Random::rotl64(y, 16) ^ z;

    return Random::adjustedSeed(seed);
}


int64_t Random::randIntOld(int64_t seed, int64_t min, int64_t max) {

    int64_t spike = seed * 123;

    if (min >= max) return min;

    int64_t diff = abs(max - min);
    spike = abs(spike) < 20000 ? spike * 123456789 : spike;
    spike = spike == 0 ? 987654321 : spike;
    spike = spike >> 1;

    // Randomize
    int64_t rots = spike ^ rotl64(spike, 10) ^ rotl64(spike, 26) ^ rotl64(spike, 31);
    int64_t o_e = seed ^ rots ^ 2654435761 ^  2147483647;
    
    // Clamp between min and max
    int64_t value = (int64_t) (abs(o_e) % diff);
 
    return value + min;
}
