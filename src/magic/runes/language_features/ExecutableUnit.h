#pragma once


#include "ErrorCatcher.h"


using namespace std;

struct ExecutableUnit {

    
    int line = -1;
    string name;
    string parameter_binding; // for binding this varaible to a function parameter if is an input to a function


    virtual shared_ptr<ExecutableUnit> resolve(
        unordered_map<string, shared_ptr<ExecutableUnit>>& global_functions, 
        unordered_map<string, shared_ptr<ExecutableUnit>>& global_variables,
        stringstream& output,
        unordered_map<string, shared_ptr<ExecutableUnit>>& local_variables,
        ErrorCatcher* error_catcher
    ) = 0;

    virtual ~ExecutableUnit() {};

    
};



