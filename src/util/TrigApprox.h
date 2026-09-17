// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <cmath>
#include <chrono>


using namespace std;

/**
 * 
 */
class TrigApprox
{
public:
	TrigApprox();
	~TrigApprox();

	inline static const double PIE = 3.14159265359;
	inline static const double PI_2 = 6.28318530718;
	inline static const double PI_HALVES = 1.57079632679;
	inline static const double PI_3_HALVES = 4.71238898037;
	inline static const double PI_FOURTHS = 0.785398163397;
	inline static const double PI_3_FOURTHS = 2.356194490191;
	inline static const double PI_5_FOURTHS = 3.926990816985;
	inline static const double PI_7_FOURTHS = 5.497787143779;

	inline static const double SQRT_2_HALVES = 0.707106781187;
	inline static const double SQRT_3_HALVES = 0.866025403784;
	inline static const double SQRT_3_HALVES_NEG = -0.866025403784;
	inline static const double HAVLE = 0.5;
	inline static const double HAVLE_NEG = -0.5;


	inline static const double SLOPE_LESS_FOURTHS = 0.900316316158;
	inline static const double SLOPE_LESS_FOURTHS_NEG = -0.900316316158;
	inline static const double SLOPE_LESS_PI_HALVES = 0.372923228578;
	inline static const double SLOPE_LESS_PI_HALVES_NEG = -0.372923228578;

	static double sin(double x);
	static double cos(double x);
	static double tan(double x);



private:
	void test();
};
