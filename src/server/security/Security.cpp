
#include "Security.h"

Security::Security()
{
}

Security::~Security()
{
}

bool Security::check_client(string& ip_address, Data& data)
{
    // check if client is blacklisted
    if (data.blacklist.run_with_read_lock([&](auto& bl) { return bl.find(ip_address) != bl.end(); }))
        return true;
    
    return false;
}

bool Security::rate_limit(string& ip_address, RequestType type, Data& data)
{
    chrono::steady_clock::time_point now = chrono::steady_clock::now();
    chrono::steady_clock::time_point last_request_time = data.check_in(ip_address + to_string((int)type), now);
    int64_t elasped_ms = chrono::duration_cast<chrono::milliseconds>(now - last_request_time).count();

    if (this->TYPE_TO_RATE_LIMIT_MS.find(type) != this->TYPE_TO_RATE_LIMIT_MS.end())
        return this->TYPE_TO_RATE_LIMIT_MS[type] > elasped_ms;

    return Security::RATE_LIMIT_MS > elasped_ms;
}


bool Security::rate_limit_by_id(const string& player_id, Data& data) {
    chrono::steady_clock::time_point now = chrono::steady_clock::now();
    chrono::steady_clock::time_point last_login_time = data.get_last_login_attempt(player_id, now);

    int64_t elasped_ms = chrono::duration_cast<chrono::milliseconds>(now - last_login_time).count();

    return Security::RATE_LIMIT_MS > elasped_ms;
}

string Security::hash_password(const string& password) {
    string sha = SHA256::hashString(password);
    return BCrypt::generateHash(sha, 12);
}

bool Security::is_password_valid(const string& password, const string& hash) {
    string sha = SHA256::hashString(password);
    return BCrypt::validatePassword(sha, hash) == 1;
}

bool Security::is_captcha_valid(unordered_map<uint64_t, unordered_set<uint8_t>>& captcha_answers, Data& data) {

    // check captcha answers
    for(pair<uint64_t, unordered_set<uint8_t>> captcha : captcha_answers) {
        if (!data.puzzle_pool.run_with_read_lock([&](auto& pool) { return pool.check_puzzle(captcha.first, captcha.second); })) return false;
    }

    return true;
}


