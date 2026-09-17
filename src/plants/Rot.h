// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "util/PositionDouble.h"
#include "util/TrigApprox.h"
#include <cmath>





using namespace std;

/**
 * Class used for simple rotation. Only rotates in two ways, relative to the root of the object
 * 
 * 
 * 
 * (to visualize rotating yourself, between an axis above you, or one that goes from your right to left)
 * 
 * First you rotate around the vertical axis using the 'y' value. Then in the direction you are looking, you
 * rotate up and down around the 'x' axis.
 * 
 */
class Rot
{
public:
	Rot(double x, double y);
	~Rot();

	// around right-left axis 0-2pi
	double x;

	// around vertical axis 0-2pi
	double y;

	// precalculated values needed in rotations
	double xHorizontalRatio;
	double xVerticalRatio;
	double yHorizontalRatio;
	double yVerticalRatio;



	static shared_ptr<PositionDouble> rotate(shared_ptr<PositionDouble> initialOffsetFromRoot, shared_ptr<Rot> rotation);
 

	

private:
	static shared_ptr<PositionDouble> rotateX(shared_ptr<PositionDouble> initialOffsetFromRoot, double x);

	static shared_ptr<PositionDouble> rotateY(shared_ptr<PositionDouble> initialOffsetFromRoot, double y);
};
