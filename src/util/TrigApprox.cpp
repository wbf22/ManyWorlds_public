

// Fill out your copyright notice in the Description page of Project Settings.


#include "TrigApprox.h"


TrigApprox::TrigApprox()
{
}

TrigApprox::~TrigApprox()
{
}





/*
	Uses a piece wise function to approximate sin() cheaply.

	1 fmod, 4 comparisons, 2 +/-, 1 *

	Saves ~0.0000018 miliseconds per call vs std::sin

	60fps is ~16 miliseconds per frame
	
	if you call sin 1000/frame, it'll save you 0.0018/frame

	not really necessary to use this probably

*/
double TrigApprox::sin(double x) {
	double mod = fmod(x, PI_2);

	if (mod < 0) mod += PI_2;
	

	if (mod < PIE) {
		if (mod < PI_HALVES) {
			if (mod < PI_FOURTHS) {
				return mod * SLOPE_LESS_FOURTHS;
			}
			else {
				return (mod - PI_FOURTHS) * SLOPE_LESS_PI_HALVES + SQRT_2_HALVES;
			}
		}
		else {
			if (mod < PI_3_FOURTHS) {
				return (mod - PI_HALVES) * SLOPE_LESS_PI_HALVES_NEG + 1;
			}
			else {
				return (mod - PI_3_FOURTHS) * SLOPE_LESS_FOURTHS_NEG + SQRT_2_HALVES;
			}
		}
	}
	else {
		if (mod < PI_3_HALVES) {
			if (mod < PI_5_FOURTHS) {
				return (mod - PIE) * SLOPE_LESS_FOURTHS_NEG;
			}
			else {
				return (mod - PI_5_FOURTHS) * SLOPE_LESS_PI_HALVES_NEG - SQRT_2_HALVES;
			}
		}
		else {
			if (mod < PI_7_FOURTHS) {
				return (mod - PI_3_HALVES) * SLOPE_LESS_PI_HALVES - 1;
			}
			else {
				return (mod - PI_7_FOURTHS) * SLOPE_LESS_FOURTHS - SQRT_2_HALVES;
			}
		}
	}


}


double TrigApprox::cos(double x) {
	return sin(x + PI_HALVES);
}


double TrigApprox::tan(double x) {
	return sin(x) / cos(x);
}



void TrigApprox::test() {

	double half = TrigApprox::sin(.5);
	double one = TrigApprox::sin(1);
	double threeHalf = TrigApprox::sin(1.5);
	double two = TrigApprox::sin(2);
	double fiveHalf = TrigApprox::sin(2.5);
	double three = TrigApprox::sin(3);
	double sevenHalf = TrigApprox::sin(3.5);
	double four = TrigApprox::sin(4);
	double nineHalf = TrigApprox::sin(4.5);
	double five = TrigApprox::sin(5);
	double elevenHalf = TrigApprox::sin(5.5);
	double six = TrigApprox::sin(6);


	double negOne = TrigApprox::sin(-1);
	double negThree = TrigApprox::sin(-3);
	double negRand = TrigApprox::sin(-6.2789);

	double big = TrigApprox::sin(-7.2789);




	auto start = std::chrono::high_resolution_clock::now();

	for (int i = 0; i < 10000000; i++) {
		double x = std::sin(2.23453535);
	}

	auto end = std::chrono::high_resolution_clock::now();

	auto stdSin = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);


	start = std::chrono::high_resolution_clock::now();

	for (int i = 0; i < 10000000; i++) {
		double x = TrigApprox::sin(2.23453535);
	}

	end = std::chrono::high_resolution_clock::now();

	auto TApprSin = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
}