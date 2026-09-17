#pragma once


#include "RequestHandler.h"
#include "util/CryptoRandom.hpp"
#include "../security/Security.h"
#include "Login.hpp"
#include "../Settings.hpp"
#include <unordered_map>
#include <vector>
#include "server/Interface.hpp"

#include <string>
#include "util/Logger.hpp"


#define RED "\033[1;31m"
#define RESET "\033[0m"


using namespace std;



/**
 * Handles a register request. 
 * 
 * Syntax:
 * {password, n bytes}
 * 
 * or with Captcha:
 * {captcha, n bytes}{password, n bytes}
 */
struct Register : public RequestHandler<Interface::Register>
{   

    int MAX_LENGTH;
    

    Register(int num_capthcas, int captcha_answers) {

        // make a fake request with the max sizes
        Interface::Register max_size_request;
        max_size_request.type = RequestType::REGISTER;
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

        // compute max packed size
        uint8_t tmp[256];
        this->MAX_LENGTH = (max_size_request.pack(tmp) + 7) / 8;
    }

    Interface::Register parse_body(
        const int message_length,
        const char* message,
        const string& ip_address, 
        Data& data
    ) override {
        // XXX add option for whitelist only registration, so that this can be done if bots are attacking

        // if already logged in, return (prevents attacks: attacker registering multiple times on same ip)
        string ip_player_id = data.get_id_if_logged_in(ip_address);
        if (ip_player_id.length() == 36) throw runtime_error("Player already logged in");

        // filter by length (prevents attacks: big requests)
        if (message_length > this->MAX_LENGTH) throw runtime_error("Request too large");

        // do some maintenance if under attack (prevents attacks: cleans up fake attacker accounts)
        bool limit_reached = false;
        data.clear_attacker_accounts_if_needed(limit_reached);
        if (limit_reached) {
            Logger::error("ACCOUNT CREATION UNDER ATTACK: had to deny recent account creation request. Max accounts setting: " + to_string(Settings::MAX_ACCOUNTS));
            throw runtime_error("Reached max accounts, possible attack");
        }

        // parse request
        Interface::Register register_request = Interface::Register::unpack((uint8_t*)message);

        return register_request;
    }
    

    void fulfill_request(
        Interface::Register body,
        const string& ip_address, 
        const int port,
        Data& data,
        shared_ptr<Request> request,
        const string& player_id
    ) override 
    {
        /*
            Botnets: Up to millions of IP addresses.
            Botnets: switch instantly, or in parallel

            Proxy Networks: 10s of thousands of IP addresses
            Proxy Networks: within seconds
        */


        /*
        
        Attacks
        - spam puzzle request, locking server
        - get puzzle and solve, then spam register (millions of accounts)
        - spam submissions to puzzles to use up submissions (locks registration)
        
        Defenses
        - rate limit puzzle requests
        - rate limit account creation
        - delete puzzles once they've been solved 3 times?
        - puzzle ids are collision resistant

        */



        // check captcha (prevents attacks: making registering harder and slower for bots)
        if (Settings::REQUIRE_CAPTCHA_DURING_REGISTER) {
            if (!Security::is_captcha_valid(body.captcha_answers, data)) return;
        }
        
        // get credentials
        string new_player_id = CryptoRandom::uuid();

        // hash password
        string hash_salt = Security::hash_password(body.password);

        // save credentials
        data.add_player(new_player_id, hash_salt);


        // mark as logged in and send back auth response
        Login::mark_logged_in_and_send_back_auth_response(ip_address, new_player_id, data, request);
    }

    ~Register() override {}

};







