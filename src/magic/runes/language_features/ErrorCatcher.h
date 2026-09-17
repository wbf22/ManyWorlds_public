#pragma once


#include <string>


using namespace std;


struct ErrorCatcher {

    virtual void error(string msg, int line_number) = 0;
};



