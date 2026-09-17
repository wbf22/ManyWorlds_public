#pragma once


#include "server/Server.hpp"
#include <string>
#include <cstdint>
#include "RequestHandler.h"
#include "../Data.hpp"
#include "server/Interface.hpp"
#include "server/Interface.hpp"



using namespace std;

struct Ping : public RequestHandler<Interface::Ping> {

    ~Ping() override {}
    
    Interface::Ping parse_body(
        const int message_length,
        const char* message,
        const string& ip_address, 
        Data& data
    ) override {
        return Interface::Ping();
    }
    
    void fulfill_request(
        Interface::Ping body,
        const string& ip_address, 
        const int port,
        Data& data,
        shared_ptr<Request> request,
        const string& player_id
    ) override 
    {
        // return one byte to indicate the server is alive
        Interface::Ping response;
        response.status = 7;
        data.send_tcp(response, request->tcp_socket);
    }

};

