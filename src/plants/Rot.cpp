// Fill out your copyright notice in the Description page of Project Settings.


#include "Rot.h"


Rot::Rot(double x, double y)
{
    this->x = x;
    this->y = y;

    this->xHorizontalRatio = std::sin(x);
    this->xVerticalRatio = std::cos(x);

    this->yHorizontalRatio = std::sin(y);
    this->yVerticalRatio = std::cos(y);
}


Rot::~Rot()
{
}




/*
    Rotates a point around the root position. 
    - The Y rotation rotates around the y or upwards axis
    - The X rotation rotates around the left-right axis of the point (left right is influenced by the y rotation)


    Hopefully these links work you can test it out
    https://www.desmos.com/3d/4e0c52b71c
*/
shared_ptr<PositionDouble> Rot::rotate(shared_ptr<PositionDouble> initialOffsetFromRoot, shared_ptr<Rot> rotation)
{

    // determined how to do this is desmos. There could be a more efficient way. (this method could also reuse some trig function operations)
    // not quite sure how I figured this out. Got the original rotation by thinking, then the offset rotation 
    // by trying random stuff haha. (too complicated for my brain)

    shared_ptr<PositionDouble> newPos = Rot::rotateX(initialOffsetFromRoot, rotation->x);

    return Rot::rotateY(newPos, rotation->y);
}


shared_ptr<PositionDouble> Rot::rotateX(shared_ptr<PositionDouble> initialOffsetFromRoot, double x)
{

    double circleOffsetX = initialOffsetFromRoot->x;
    double circleOffsetZ = 0;


    double deltaX = (initialOffsetFromRoot->x - circleOffsetX) * (initialOffsetFromRoot->x - circleOffsetX);
    double deltaZ = (initialOffsetFromRoot->z - circleOffsetZ) * (initialOffsetFromRoot->z - circleOffsetZ);

    double xRot;
    double div = initialOffsetFromRoot->y == 0 ? 0.0000001 : initialOffsetFromRoot->y;
    if (initialOffsetFromRoot->z > 0) 
        xRot = std::atan(
            std::sqrt(deltaX + deltaZ) / div
        );
    else 
        xRot = std::atan(
            - std::sqrt(deltaX + deltaZ) / div
        );
   
    if (initialOffsetFromRoot->y < 0) xRot += TrigApprox::PIE;

    double circleRadius = std::sqrt(deltaX + deltaZ + initialOffsetFromRoot->y * initialOffsetFromRoot->y);

    double newX = circleOffsetX;
    double newY = circleRadius * std::cos(xRot + x);
    double newZ = circleOffsetZ + circleRadius * std::sin(xRot + x);

    return std::make_shared<PositionDouble>(newX, newY, newZ);
}

shared_ptr<PositionDouble> Rot::rotateY(shared_ptr<PositionDouble> initialOffsetFromRoot, double y)
{

    double circleRadius = std::sqrt(initialOffsetFromRoot->x * initialOffsetFromRoot->x + initialOffsetFromRoot->z * initialOffsetFromRoot->z);

    double div = initialOffsetFromRoot->z == 0 ? 0.0000001 : initialOffsetFromRoot->z;
    double yRot = std::atan(initialOffsetFromRoot->x / div);

    if (initialOffsetFromRoot->z < 0) yRot += TrigApprox::PIE;

    double newX = circleRadius * std::sin(yRot + y);
    double newY = initialOffsetFromRoot->y;
    double newZ = circleRadius * std::cos(yRot + y);
    
    return std::make_shared<PositionDouble>(newX, newY, newZ);
}

