// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <vector>
#include <memory>
#include <cmath>

using namespace std;


/**
 * 
 */
class Random
{
public:
	Random(int64_t seed);
	~Random();


	/*
	* Adjusts a seed to work better with our random function.
	* 
	* Seed values without many digits don't work super well so we
	* want to adjust them so that seeds aren't too small.
	*/
	static int64_t adjustedSeed(int64_t inputSeed);

	/*
	* Deterministic number that will follow a pattern but will be very hard to distinguish
	* from true random
	* 
	* More iterations takes more time but will appear less pattern like (though this has not
	* been tested extensively)
	*
	* Number between min and max (inclusive, exclusvie)
	* 
	*/
	static int hardRandInt(int64_t seed, int64_t min, int64_t max, int iterations);


	static int64_t rotl64(int64_t x, unsigned int n);

	static int64_t rotr64(int64_t x, unsigned int n);

	/*
	* Deterministic number that will follow a pattern but will be very hard to distinguish
	* from true random
	* 
	* Fairly Cheap:
	* (4 XOR, 3 shifts, 1 multiply, 1 divide, 1 modulus, 2 addition)
	*
	* Number between min and max (inclusive, exclusvie)
	* 
	*/
	static int64_t randInt(int64_t seed, int64_t min, int64_t max);

	/*
	* Deterministic number that will follow a pattern but will be very hard to distinguish
	* from true random
	*
	* On average will be closer to the provided average value
	*
	* Number between min and max (inclusive, exclusvie)
	* 
	*/
	static int64_t randInt(int64_t seed, int64_t min, int64_t average, int64_t max);

	/*
	* Deterministic number that will follow a pattern but will be very hard to distinguish
	* from true random
	*
	* On average will be closer to the provided average value
	*
	* 'averageStrenth' determines how likely the random value is close to average. (0-100.0)
	* Works by getting a random weight from 0-100 from averageWeight, and then doing a weighted average between the average and a 
	* random value.
	*
	* Number between min and max (inclusive, exclusvie)
	* 
	*/
	static int64_t randInt(int64_t seed, int64_t min, int64_t average, int64_t max, double averageStrength);


	/*
	* Deterministic number that will follow a pattern but will be very hard to distinguish
	* from true random
	*
	* Number between min and max (inclusive, ~inclusive)
	*
	*/
	static double randDouble(int64_t seed, double min, double max);

	/*
	* Deterministic number that will follow a pattern but will be very hard to distinguish
	* from true random
	*
	* On average will be closer to the provided average value
	*
	* Number between min and max (inclusive, ~inclusive)
	*
	*/
	static double randDouble(int64_t seed, double min, double average, double max);

	
	/*
	* Deterministic number that will follow a pattern but will be very hard to distinguish
	* from true random
	*
	* On average will be closer to the provided average value
	*
	* 'averageStrenth' determines how likely the random value is close to average. (typically values close to 1.0 are best)
	*
	* Number between min and max (inclusive, exclusvie)
	* 
	*/
	static double randDouble(int64_t seed, double min, double average, double max, double averageStrength);


	/*
	* Deterministic number that will follow a pattern but is cheap.
	* (2 adds, 2 subtraction, 1 modulus)
	*
	* Number between min and max (inclusive, exclusvie)
	* 
	*/
	static int cheapRandom(int seed, int min, int max);


	/*
	* Deterministic bool that will follow a pattern but will be very hard to distinguish
	* from true random
	* 
	*/
	static bool randBool(int64_t seed);


	static bool randBool(int64_t seed, int amount_true_in_100);


	struct Params {
		int64_t value;
		int64_t min;
		int64_t max;
		int64_t leeway;

		Params(int64_t value, int64_t min, int64_t max, int64_t leeway)
			: value(value), min(min), max(max), leeway(leeway) {}
	};

	static vector<int64_t> possible_seeds(shared_ptr<Params> params, int64_t& index);

	static int64_t reverse(vector<shared_ptr<Params>> param_sets);

	static int64_t iterations_to_reverse(int num_params, int64_t average_min, int64_t average_max, int64_t average_leeway);

    /**
     * Returns a randomized copy of the given vector. Doesn't deep copy objects.
     */
    template<typename T>
    static vector<T> get_random_copy_list(vector<T> &vec, int64_t seed) {
        vector<T> copy;
		for (T t : vec) {
			copy.push_back(t);
		}

		for (int i = 0; i < copy.size(); ++i) {
			int swap = Random::randInt(seed+i, 0, copy.size());
			T current = copy[i];
			copy[i] = copy[swap];
			copy[swap] = current;
		}

		return copy;
    }


    /**
     * Uses the weights and our random function to make a choice. 
	 * 
	 * Weights should sum to 100, and there should be the same number of weights as options
     */
    template <typename T>
	static T weighted_choice(vector<int> weight, vector<T> options, int64_t seed)
	{
		int rando = Random::randInt(seed, 0, 100);

		int sum = 0;
		for (int i = 0; i < weight.size(); i++) {
			sum += weight[i];

			if (sum >= rando) {
				return options[i];
			}
		}

		return options[options.size()-1];
	}

	template <typename T>
	static T random_choice(vector<T> options, int64_t seed) {
		return options[Random::randInt(seed, 0, options.size())];
	}

	/**
	 * Makes a set of weights that sum to 100. The weights start at the start value and progress to end value.
	 * They're then scaled to sum to 100
	 */
	static vector<int> make_weights(int start, int end, int num_weights);

	/**
	 * Takes a set of weights and scales them to add up to 100
	 */
	static vector<int> balance_weights(vector<int> weights);

	/**
	 * Makes a seed for arbitrary coordinates. 
	 */
	static int64_t seed_from_coordinates(int64_t x, int64_t y, int64_t z);


    /**
     * Uses the weights and our random function to make a choice. 
	 * 
	 * Weights don't have to sum up to 100 but should all be positive. 
	 * They are there to capture relative sizes.
     */
	static int weighted_choice_irregular(vector<float> weights, int64_t seed)
	{

		float sum = 0;
		for (int i = 0; i < weights.size(); i++) {
			sum += weights[i];
		}

		float rando = Random::randDouble(seed, 0, sum);
		float intro_sum = 0;
		for (int i = 0; i < weights.size(); i++) {
			intro_sum += weights[i];
			if (intro_sum > rando) {
				return i; 
			}
		}



		return weights.size()-1;
	}

	static int64_t randIntOld(int64_t seed, int64_t min, int64_t max);
	
};
