#pragma once


#include <string>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include "../Data.hpp"
#include "server/Interface.hpp"
#include "../Settings.hpp"
#include "sha256.h"
#include "bcrypt/BCrypt.hpp"


using namespace std;

class Security {
public:
    Security();
    ~Security();
    
    // default rate limit for requests is once every 5 seconds 
    // WARNING: rate_limit_by_id depends on this
    static constexpr int RATE_LIMIT_MS = 5000; 

    unordered_map<RequestType, int> TYPE_TO_RATE_LIMIT_MS = {
        {RequestType::ATTACK, 100}, // every 100 ms, ~10 requests per second
        {RequestType::LOGIN, 5000}, // every 5000 ms, or once every 5 seconds
        {RequestType::REGISTER, 60000 * 2}, // every 2 minutes
        {RequestType::STOP, 60000 * 2}, // every 2 minutes
        {RequestType::PING, 500}, // every 500 ms
    };

    
    // returns true if client should be rejected
    bool check_client(string& ip_address, Data& data);

    // returns true if request should be rejected
    bool rate_limit(string& ip_address, RequestType type, Data& data);

    // returns true if request should be rejected
    static bool rate_limit_by_id(const string& player_id, Data& data);
     
    static string hash_password(const string& password);

    static bool is_password_valid(const string& password, const string& hash);

    /**
     * parses the captcha data from the request and checks if the captcha is valid
     * 
     * {num_captchas, 1 byte}{captcha_id, 8 byte}{len_captcha_answers, 2 bytes}{captcha_answers, len_captcha_answers bytes}
     */
    static bool is_captcha_valid(unordered_map<uint64_t, unordered_set<uint8_t>>& captcha_answers, Data& data);
};