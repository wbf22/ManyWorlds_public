#pragma once



#include <iostream>


#define L_RED "\033[1;31m"
#define L_ORANGE "\033[1;33m"
#define L_GREEN "\033[1;32m"
#define L_BLUE "\033[1;34m"
#define L_YELLOW "\033[1;33m"
#define L_PURPLE "\033[1;35m"
#define L_RESET "\033[0m"

using namespace std;

enum LogLevel {
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR
};


struct Logger {
    static inline LogLevel LEVEL = INFO;

    static void trace(const string message) {
        if (LEVEL <= TRACE) {
            cerr << L_PURPLE << "TRACE: " << message << L_RESET << endl;
        }
    }

    static void debug(const string message) {
        if (LEVEL <= DEBUG) {
            cout << L_GREEN << "DEBUG: " << message << L_RESET << endl;
        }
    }

    static void info(const string message) {
        if (LEVEL <= INFO) {
            cout << L_BLUE << "INFO: " << message << L_RESET << endl;
        }
    }

    static void warn(const string message) {
        if (LEVEL <= WARN) {
            cout << L_ORANGE << "WARN: " << message << L_RESET << endl;
        }
    }

    static void error(const string message) {
        if (LEVEL <= ERROR) {
            cout << L_RED << "ERROR: " << message << L_RESET << endl;
        }
    }
};

