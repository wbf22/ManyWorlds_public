#pragma once


#include "../Server.hpp"
#include <string>
#include <cstdint>
#include "RequestHandler.h"
#include "../Data.hpp"
#include "server/Interface.hpp"
#include "server/Interface.hpp"


using namespace std;

struct Stop : public RequestHandler<Interface::Stop> {

    int MAX_LENGTH;

    Stop() {
        Interface::Stop max_request;
        uint8_t tmp[64];
        this->MAX_LENGTH = (max_request.pack(tmp) + 7) / 8;
    }

    ~Stop() override {}

    Interface::Stop parse_body(
        const int message_length,
        const char* message,
        const string& ip_address, 
        Data& data
    ) override {
        if (message_length > this->MAX_LENGTH) throw runtime_error("Request too large");

        // parse request
        Interface::Stop stop_request = Interface::Stop::unpack((uint8_t*)message);

        return stop_request;
    }
    
    void fulfill_request(
        Interface::Stop body,
        const string& ip_address, 
        const int port,
        Data& data,
        shared_ptr<Request> request,
        const string& player_id
    ) override 
    {

        // check if player is admin with permissions
        if (!data.has_permission(player_id, Commands::STOP)) return;

        // stop the server
        data.running = false;

        // send a ping back to confirm success
        Interface::Ping response;
        response.status = 3;
        data.send(response, ip_address, port);


        // XXX: broadcast a message telling everyone the server is stopping
    }

};

