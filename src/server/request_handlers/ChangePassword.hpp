#pragma once


#include "RequestHandler.h"
#include "util/CryptoRandom.hpp"
#include "../security/Security.h"
#include "Interface.hpp"
#include "../Data.hpp"

#include <string>




using namespace std;



/**
 * Handles a password change request
 */
struct ChangePassword : public RequestHandler<Interface::ChangePassword>
{   

    int MAX_LENGTH;

    ChangePassword() {
        Interface::ChangePassword max_request;
        max_request.type = RequestType::PASSWORD_CHANGE;
        max_request.old_password = string(Settings::MAX_PASSWORD_LENGTH, 'a');
        max_request.new_password = string(Settings::MAX_PASSWORD_LENGTH, 'a');
        uint8_t tmp[256];
        this->MAX_LENGTH = (max_request.pack(tmp) + 7) / 8;
    }
    ~ChangePassword() override {}


    Interface::ChangePassword parse_body(
        const int message_length,
        const char* message,
        const string& ip_address, 
        Data& data
    ) override {

        if (message_length > this->MAX_LENGTH) throw runtime_error("Request too large");

        // parse request
        Interface::ChangePassword request = Interface::ChangePassword::unpack((uint8_t*)message);

        return request;
    }
    
    void fulfill_request(
        Interface::ChangePassword body,
        const string& ip_address, 
        const int port,
        Data& data,
        shared_ptr<Request> request,
        const string& player_id
    ) override 
    {
        // before this in RequestHandler.h the token should have already been verified matching to the ip_address
        // player_id should also be set, looking up the person by ip_address

        // get player id in request
        string request_player_id = CryptoRandom::convert_bytes_to_uuid(reinterpret_cast<char*>(body.player_id.data()));

        // check if player is changing their own password
        if (player_id != request_player_id) throw runtime_error("Player is trying to change another player's password");

        // verify old password
        string salted_password_hash = data.get_salted_password(request_player_id);
        if (!Security::is_password_valid(body.old_password, salted_password_hash)) return;

        // change password
        string hash_salt = Security::hash_password(body.new_password);
        data.change_password(request_player_id, hash_salt);


        // send a ping back to confirm success
        Interface::Ping response;
        response.status = 3;
        data.send_tcp(response, request->tcp_socket);
        
    }




};







