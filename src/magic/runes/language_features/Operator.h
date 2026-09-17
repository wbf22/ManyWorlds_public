#pragma once


#include <string>
#include "Variable.hpp"


using namespace std;



enum class Operator {
    ADD,
    SUBTRACT,
    MULTIPLY,
    DIVIDE,
    POWER,

    AND,
    OR,
    EQUALS,
    DOES_NOT_EQUAL,
    GREATER_THAN,
    LESS_THAN,
    GREATER_THAN_OR_EQUAL,
    LESS_THAN_OR_EQUAL,


    NONE
};


Operator parse_operator(const string& str, int& i);

bool is_higher_order(Operator operato, Operator other_operator);


bool is_boolean_operator(Operator operato);

bool is_operator(char c);


// for Variable Type
string variable_type_name(VariableType type);
