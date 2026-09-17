#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
// #include "Global.hpp"
// #include "Expression.hpp"
#include "Variable.hpp"
#include "ExecutableUnit.h"
#include "SystemFunctions.hpp"




using namespace std;

class Global;


enum LineType {
    EMPTY_LINE,
    VARIABLE_SET,
    FUNCTION_CALL,
    FOR_LOOP,
    FOR_EACH,
    WHILE,
    FUNCTION_DEFINITION,
    RETURN_STATEMENT
};

struct Line : public ExecutableUnit {
    LineType type;
    int number = -1;

    // variable set
    string variable_target;
    shared_ptr<ExecutableUnit> expression;

    // function call
    string function_name;
    vector<shared_ptr<ExecutableUnit>> input_expressions;


    // for loop
    shared_ptr<Variable> step;
    shared_ptr<Variable> end;
    shared_ptr<Variable> i;
    string i_name;

    // for each
    string for_each_target;

    // while
    shared_ptr<ExecutableUnit> condition;
    
    // all loops
    vector<shared_ptr<Line>> loop_block;
    

    shared_ptr<ExecutableUnit> resolve(
        unordered_map<string, shared_ptr<ExecutableUnit>>& global_functions, 
        unordered_map<string, shared_ptr<ExecutableUnit>>& global_variables,
        stringstream& output,
        unordered_map<string, shared_ptr<ExecutableUnit>>& local_variables,
        ErrorCatcher* error_catcher
    ) override;

    void add_varaible_to_global_or_local_context(
        shared_ptr<Variable>& var,
        unordered_map<string, shared_ptr<ExecutableUnit>>& global_variables, 
        unordered_map<string, shared_ptr<ExecutableUnit>>& local_variables
    );


    shared_ptr<Variable> execute_system_function(
        string function_name, 
        stringstream& output, 
        unordered_map<string, shared_ptr<ExecutableUnit>>& input_params, 
        int line_number
    );
};