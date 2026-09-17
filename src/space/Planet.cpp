// Fill out your copyright notice in the Description page of Project Settings.


#include "Planet.h"
#include "../util/BMaterial.hpp"
#include "../buildings_and_cities/City.hpp"

Planet::Planet(int64_t seed, shared_ptr<StellarCoordinate> location)
{

    // TODO probably move this to another class that handles the game start up 
    // TODO if this seed doesn't have a world present, then look for one nearby
    this->seed = Random::adjustedSeed(seed);

    this->location = location;

    this->precipitation = Random::randInt(this->seed, 0, 100);
    
    // XXX make more of these planet actually used and also randomly generated here
    this->gravity = 10;

    // zero the atmosphere/water fields so unset branches don't read garbage
    this->h2o = this->n2 = this->o2 = this->methane = this->h2 = this->he = 0;
    this->ammonia = this->co2 = this->sulfericAcid = this->sulfer = 0;
    this->phosphorus = this->chlorine = this->bromine = this->idoine = 0;
    this->sodium_potassium = this->iron = this->titanium = this->silicate = 0;
}

Planet::~Planet()
{
}

vector<shared_ptr<Economy>> Planet::get_child_economies() {
    return vector<shared_ptr<Economy>>(cities.begin(), cities.end());
}

