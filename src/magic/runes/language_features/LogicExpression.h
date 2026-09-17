#pragma once


#include <memory>
#include <string>
#include <unordered_map>
#include "ExecutableUnit.h"
// #include "Function.hpp"
#include "Variable.hpp"
#include "Operator.h"
#include <cmath>


using namespace std;

// class Variable;
class Global;


struct LogicExpression : public ExecutableUnit {

    shared_ptr<ExecutableUnit> left_hand;
    Operator operato;
    shared_ptr<ExecutableUnit> right_hand;

    // LogicExpression();

    // virtual ~LogicExpression();

    shared_ptr<ExecutableUnit> resolve(
        unordered_map<string, shared_ptr<ExecutableUnit>>& global_functions, 
        unordered_map<string, shared_ptr<ExecutableUnit>>& global_variables,
        stringstream& output,
        unordered_map<string, shared_ptr<ExecutableUnit>>& local_variables,
        ErrorCatcher* error_catcher
    ) override;


    void get_variable_value(
        shared_ptr<Variable>& var, 
        unordered_map<string, shared_ptr<ExecutableUnit>>& global_variables, 
        unordered_map<string, shared_ptr<ExecutableUnit>>& parameter_variables
    );
};