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
struct Login : public RequestHandler<Interface::Login>
{   

    int MAX_LENGTH;

    Login(int num_capthcas, int captcha_answers) {

        // make a fake request with the max sizes
        Interface::Login max_size_request;
        max_size_request.type = RequestType::LOGIN;
        max_size_request.num_captchas = num_capthcas;
        unordered_map<uint64_t, unordered_set<uint8_t>> answers;
        for (int i = 0; i < num_capthcas; i++) {
            unordered_set<uint8_t> answer;
            for (int j = 0; j < captcha_answers; j++) {
                answer.insert(j);
            }
            answers[i] = answer;
        }
        max_size_request.captcha_answers = answers;
        max_size_request.password = string(Settings::MAX_PASSWORD_LENGTH, 'a');
        char player_id[16];
        max_size_request.player_id = string(player_id, 16);

        // compute max packed size
        uint8_t tmp[256];
        this->MAX_LENGTH = (max_size_request.pack(tmp) + 7) / 8;

    }


    Interface::Login parse_body(
        const int message_length,
        const char* message,
        const string& ip_address, 
        Data& data
    ) override {
        
        // filter by length (prevents attacks: big requests)
        if (message_length > this->MAX_LENGTH) throw runtime_error("Request too large");

        // parse request
        Interface::Login login_request = Interface::Login::unpack((uint8_t*)message);

        return login_request;
    }

    void fulfill_request(
        Interface::Login body,
        const string& ip_address, 
        const int port,
        Data& data,
        shared_ptr<Request> request,
        const string& player_id
    ) override 
    {


        // check captcha (prevents attacks: slowing down login requests)
        int captcha_end = 0;
        if (Settings::REQUIRE_CAPTCHA_DURING_LOGIN) {
            if (!Security::is_captcha_valid(body.captcha_answers, data)) return;
        }

        // bound password length to prevent CPU exhaustion via bcrypt
        if (body.password.size() > Settings::MAX_PASSWORD_LENGTH) return;

        // rate limit by request_player_id
        if(Security::rate_limit_by_id(body.player_id, data)) return;

        // check credentials
        string salted_password_hash = data.get_salted_password(body.player_id);

        if (!Security::is_password_valid(body.password, salted_password_hash)) return;

        // mark logged in and send back auth response
        mark_logged_in_and_send_back_auth_response(ip_address, body.player_id, data, request);
    }

    ~Login() override {}



    static void mark_logged_in_and_send_back_auth_response(
        const string& ip_address, 
        const string& player_id, 
        Data& data,
        shared_ptr<Request> request
    ) {

        // mark ip as logged in (prevents attacks: attacker registering multiple times on same ip)
        data.logged_in_ip_address_to_player_id.run_with_lock([&](auto& logged_in) {
            logged_in[ip_address] = player_id;
        });


        // return player id
        Interface::Auth response;
        CryptoRandom::uuid_to_bytes(response.uuid.data(), player_id);
        data.send_tcp(response, request->tcp_socket);

    }


};







