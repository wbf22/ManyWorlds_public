#pragma once


#include <string>
#include <unordered_map>
#include "server/Interface.hpp"
#include "util/StellarCoordinate.hpp"


using namespace std;


struct Player {
    // security
    string player_id;
    string salted_password;
    string op_level;

    // player info
    string username;
    double health = 100;
    int placed_blocks = 0;
    StellarCoordinate steller_coordinate;
    
    
    unordered_map<string, double> money;

    
    // XXX make function to serialize player data into a string to be stored




};


