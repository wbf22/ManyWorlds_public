#pragma once


#include "../Server.hpp"
#include <string>
#include <cstdint>
#include "RequestHandler.h"
#include "../Data.hpp"
#include "server/Interface.hpp"



using namespace std;


struct PlaceBlock : public RequestHandler<Interface::Placement> {

    ~PlaceBlock() override {}
    
    Interface::Placement parse_body(
        const int message_length,
        const char* message,
        const string& ip_address, 
        Data& data
    ) override {
        return Interface::Placement();
    }
    
    void fulfill_request(
        Interface::Placement body,
        const string& ip_address, 
        const int port,
        Data& data,
        shared_ptr<Request> request,
        const string& player_id
    ) override 
    {
        
        // get player location

        // determine where block is being placed

        // decide if that's ok

        // place block and notify nearby players
    }

};

