#pragma once

#include <godot_cpp/variant/quaternion.hpp>




using namespace std;
using namespace godot;

struct KeyFrame
{
    Vector3 position;
    Quaternion rotation;
    double time;


    KeyFrame() {}

    KeyFrame(Vector3 position, Quaternion rotation) {
        this->position = position;
        this->rotation = rotation;
    }

    KeyFrame(Vector3 position, Quaternion rotation, double time) {
        this->position = position;
        this->rotation = rotation;
        this->time = time;
    }

};

