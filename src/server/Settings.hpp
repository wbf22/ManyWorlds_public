#pragma once

#include <string>

using namespace std;

struct Settings {

    // basic settings
    inline static string WORLD_FOLDER = "world";
    inline static int PORT = 20202;
    inline static int TIMEOUT_MS = 1000;
    inline static int WORKER_THREADS = 16;

    // security settings
    inline static int MAX_ACCOUNTS = 1000000;
    inline static int MIN_PASSWORD_LENGTH = 8;
    inline static int MAX_PASSWORD_LENGTH = 64;
    inline static bool REQUIRE_CAPTCHA_DURING_REGISTER = true;
    inline static bool REQUIRE_CAPTCHA_DURING_LOGIN = true;
    inline static int NUM_CAPTCHA_PUZZLES = 1;
    inline static int CAPTCHA_DIFFICULTY = 5;

    // data
    inline static bool STORE_READABLE_DATA = true;

    // debug
    inline static bool DEBUG_MODE = true;

};
