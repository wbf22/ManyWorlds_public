
#include "Operator.h"


Operator parse_operator(const string& str, int& i) {
    if (str[i] == '+') {
        ++i;
        return Operator::ADD;
    }
    else if (str[i] == '-') {
        ++i;
        return Operator::SUBTRACT;
    }
    else if (str[i] == '*') {
        ++i;
        return Operator::MULTIPLY;
    }
    else if (str[i] == '/') {
        ++i;
        return Operator::DIVIDE;
    }
    else if (str[i] == '&') {
        ++i;
        return Operator::AND;
    }
    else if (str[i] == '|') {
        ++i;
        return Operator::OR;
    }
    else if (str[i] == '=') {
        ++i;
        return Operator::EQUALS;
    }
    else if (str[i] == '!' && i+1 < str.length() && str[i+1] == '=') {
        i += 2;
        return Operator::DOES_NOT_EQUAL;
    }
    else if (str[i] == '<') {
        if (i+1 < str.length() && str[i+1] == '=') {
            i += 2;
            return Operator::LESS_THAN_OR_EQUAL;
        }
        else {
            ++i;
            return Operator::LESS_THAN;
        }
    }
    else if (str[i] == '>') {
        if (i+1 < str.length() && str[i+1] == '=') {
            i += 2;
            return Operator::GREATER_THAN_OR_EQUAL;
        }
        else {
            ++i;
            return Operator::GREATER_THAN;
        }
    }
    else {
        return Operator::NONE;
    }
}

bool is_higher_order(Operator operato, Operator other_operator) {

    return operato > other_operator;
}

bool is_boolean_operator(Operator operato) {
    return operato == Operator::AND ||
        operato == Operator::OR ||
        operato == Operator::EQUALS ||
        operato == Operator::DOES_NOT_EQUAL ||
        operato == Operator::GREATER_THAN ||
        operato == Operator::LESS_THAN ||
        operato == Operator::GREATER_THAN_OR_EQUAL ||
        operato == Operator::LESS_THAN_OR_EQUAL;
}

bool is_operator(char c) {
    return c == '+' || 
        c == '-' || 
        c == '*' || 
        c == '/' || 
        c == '&' || 
        c == '|' || 
        c == '=' || 
        c == '!' ||
        c == '<' ||
        c == '>';
}

string variable_type_name(VariableType type) {
    switch (type) {
        case NULL_VOID:  return "NULL_VOID";
        case INT:        return "INT";
        case DOUBLE:     return "DOUBLE";
        case BOOL:       return "BOOL";
        case STRING:     return "STRING";
        case LIST:       return "LIST";
        case DICTIONARY: return "DICTIONARY";
        case SET:        return "SET";
        case TUPLE:      return "TUPLE";
        case DEQUE:      return "DEQUE";
        default:         return "UNKNOWN_TYPE";
    }
}