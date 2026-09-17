#pragma once


#include "RequestHandler.h"
#include "util/CryptoRandom.hpp"
#include "../security/Security.h"
#include "server/Interface.hpp"
#include "../Data.hpp"

#include <string>




using namespace std;



/**
 * Handles a login request. 
 * 
 * Syntax:
 * {player_id, 16 bytes}{password, n bytes}
 *
 * or with Captcha:
 * {player_id, 16 bytes}{captcha, n bytes}{password, n bytes}
 */
struct Logout : public RequestHandler<Interface::Logout>
{   

    int MAX_LENGTH;

    Logout() {
        Interface::Logout max_request;
        uint8_t tmp[64];
        this->MAX_LENGTH = (max_request.pack(tmp) + 7) / 8;
    }


    Interface::Logout parse_body(
        const int message_length,
        const char* message,
        const string& ip_address, 
        Data& data
    ) override {

        if (message_length > this->MAX_LENGTH) throw runtime_error("Request too large");

        // parse request
        Interface::Logout logout_request = Interface::Logout::unpack((uint8_t*)message);

        return logout_request;
    }
    
    void fulfill_request(
        Interface::Logout body,
        const string& ip_address, 
        const int port,
        Data& data,
        shared_ptr<Request> request,
        const string& player_id
    ) override 
    {
        // get player id in request
        string request_player_id = CryptoRandom::convert_bytes_to_uuid(reinterpret_cast<char*>(body.uuid.data()));

        // check if player is logging themselves out
        if (player_id != request_player_id) throw runtime_error("Player is not logging themselves out");

        // log them out
        data.logged_in_ip_address_to_player_id.run_with_lock([&](auto& logged_in) {
            logged_in.erase(ip_address);
        });

        // send a ping back to confirm success
        Interface::Ping response;
        response.status = 3;
        data.send_tcp(response, request->tcp_socket);
        
    }

    ~Logout() override {}



};







