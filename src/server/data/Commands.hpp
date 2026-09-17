#pragma once


#include <string>
#include <unordered_set>

using namespace std;

struct Commands {
    static inline const string STOP = "stop";

    static inline unordered_set<string> all_commands = {
        STOP
    };
};

