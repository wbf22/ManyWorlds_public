
#include "LogicExpression.h"


shared_ptr<ExecutableUnit> LogicExpression::resolve(
    unordered_map<string, shared_ptr<ExecutableUnit>>& global_functions, 
    unordered_map<string, shared_ptr<ExecutableUnit>>& global_variables,
    stringstream& output,
    unordered_map<string, shared_ptr<ExecutableUnit>>& local_variables,
    ErrorCatcher* error_catcher
) {
        
    shared_ptr<Variable> left_result = static_pointer_cast<Variable>(
        this->left_hand->resolve(global_functions, global_variables, output, local_variables, error_catcher)
    );
    shared_ptr<Variable> right_result = static_pointer_cast<Variable>(
        this->right_hand->resolve(global_functions, global_variables, output, local_variables, error_catcher)
    );

    // set variable values
    if (left_result->type == VariableType::UNKNOWN_TYPE) {
        Variable::get_variable_value(left_result, global_variables, local_variables, this->line, error_catcher);
    }
    if (right_result->type == VariableType::UNKNOWN_TYPE) {
        Variable::get_variable_value(right_result, global_variables, local_variables, this->line, error_catcher);
    }


    if (left_result->type == VariableType::STRING) {
        if (!is_boolean_operator(this->operato)) {
            if (this->operato != Operator::ADD ) {
                error_catcher->error("Strings can only be added (concatonated)", this->line);
            }
            shared_ptr<Variable> resolved = std::make_shared<Variable>();
            resolved->type = VariableType::STRING;
            resolved->value_string = left_result->value_string + right_result->get_value_as_string();
            return resolved;
        } 
        else if (right_result->is_data_structure()) {
            error_catcher->error("Strings can't be combined with " + variable_type_name(right_result->type), this->line);
        }
        else {
            string right_string = right_result->get_value_as_string();
            
            shared_ptr<Variable> resolved = std::make_shared<Variable>();
            resolved->type = VariableType::BOOL;
            switch (this->operato) {
                case Operator::EQUALS:                    resolved->value_bool = left_result->value_string == right_string;
                case Operator::DOES_NOT_EQUAL:            resolved->value_bool = left_result->value_string != right_string;
                case Operator::GREATER_THAN:              resolved->value_bool = left_result->value_string > right_string;
                case Operator::LESS_THAN:                 resolved->value_bool = left_result->value_string < right_string;
                case Operator::GREATER_THAN_OR_EQUAL:     resolved->value_bool = left_result->value_string >= right_string;
                case Operator::LESS_THAN_OR_EQUAL:        resolved->value_bool = left_result->value_string <= right_string;
                default:                                  error_catcher->error("Strings are only compared by these operators '==,!=,>,<,>=,<=", this->line);
            }
            return resolved;
        }
    }
    else if (left_result->is_numeric()) {
        
        if (is_boolean_operator(this->operato)) {
            if (right_result->is_numeric()) {
                double left_double = (left_result->type == VariableType::INT) ? left_result->value_int : left_result->value_double;
                double right_double = (right_result->type == VariableType::INT) ? right_result->value_int : right_result->value_double;

                shared_ptr<Variable> resolved = std::make_shared<Variable>();
                resolved->type = VariableType::BOOL;
                switch (this->operato) {
                    case Operator::EQUALS:                    resolved->value_bool = left_double == right_double;
                    case Operator::DOES_NOT_EQUAL:            resolved->value_bool = left_double != right_double;
                    case Operator::GREATER_THAN:              resolved->value_bool = left_double > right_double;
                    case Operator::LESS_THAN:                 resolved->value_bool = left_double < right_double;
                    case Operator::GREATER_THAN_OR_EQUAL:     resolved->value_bool = left_double >= right_double;
                    case Operator::LESS_THAN_OR_EQUAL:        resolved->value_bool = left_double <= right_double;
                    default:                                  error_catcher->error("Numbers are only compared by these operators '==,!=,>,<,>=,<=", this->line);
                }
                return resolved;
            }
            else {
                shared_ptr<Variable> resolved = std::make_shared<Variable>();
                resolved->type = VariableType::BOOL;
                resolved->value_bool = false;
                return resolved;
            }

        }


        if (!right_result->is_numeric()) {
            error_catcher->error("A " + variable_type_name(right_result->type) + " value can't be added to, subtracted from, multiply, or divide a number", this->line);
        }

        double left_double = (left_result->type == VariableType::INT) ? left_result->value_int : left_result->value_double;
        double right_double = (right_result->type == VariableType::INT) ? right_result->value_int : right_result->value_double;
        double resolved_number = left_double;
        if (this->operato == Operator::ADD) {
            resolved_number += right_double;
        }
        else if (this->operato == Operator::SUBTRACT) {
            resolved_number -= right_double;
        }
        else if (this->operato == Operator::MULTIPLY) {
            resolved_number *= right_double;
        }
        else if (this->operato == Operator::DIVIDE) {
            resolved_number /= right_double;
        }
        

        if (left_result->type == VariableType::DOUBLE || right_result->type == VariableType::DOUBLE || resolved_number != floor(resolved_number)) {
            shared_ptr<Variable> resolved = std::make_shared<Variable>();
            resolved->type = VariableType::DOUBLE;
            resolved->value_double = resolved_number;
            return resolved;
        }
        else {
            shared_ptr<Variable> resolved = std::make_shared<Variable>();
            resolved->type = VariableType::INT;
            resolved->value_int = (int) resolved_number;
            return resolved;
        }
    }
    else if (left_result->type == VariableType::NULL_VOID) {

        if (this->operato == Operator::EQUALS) {
            shared_ptr<Variable> resolved = std::make_shared<Variable>();
            resolved->type = VariableType::BOOL;
            resolved->value_bool = right_result->type == VariableType::NULL_VOID;
            return resolved;
        }
        else {
            error_catcher->error("Got a null value here", this->line);
        }
        
    }
    else if (left_result->is_data_structure()) {

        if (this->operato != Operator::ADD) {
            error_catcher->error(variable_type_name(left_result->type) + "s can only be combined with '+'. Logical operators aren't supported yet either. Did you mean to do that?", line);
        }

        if (left_result->type == VariableType::DICTIONARY) {
            if (right_result->type != VariableType::DICTIONARY) {
                error_catcher->error("Only dictionaries can be combined with dictionaries", line);
            }
            else {
                for(pair<string, shared_ptr<Variable>> pair : right_result->value_dictionary) {
                    left_result->value_dictionary[pair.first] = pair.second;
                }
                return left_result;
            }
        }
        else {
            vector<shared_ptr<Variable>> left_vector = left_result->get_as_vector();
            vector<shared_ptr<Variable>> right_vector = right_result->get_as_vector();

            for (shared_ptr<Variable> var : right_vector) {
                left_vector.push_back(var);
            }

            if (left_result->type == VariableType::LIST) {
                shared_ptr<Variable> resolved = std::make_shared<Variable>();
                resolved->type = VariableType::LIST;
                resolved->value_list = left_vector;
                return resolved;
            }
            else if (left_result->type == VariableType::SET) {
                shared_ptr<Variable> resolved = std::make_shared<Variable>();
                resolved->type = VariableType::SET;
                for (shared_ptr<Variable> var : left_vector) {
                    resolved->value_set.insert(var);
                }
                return resolved;
            }
            else if (left_result->type == VariableType::TUPLE) {
                shared_ptr<Variable> resolved = std::make_shared<Variable>();
                resolved->type = VariableType::TUPLE;
                resolved->value_tuple = left_vector;
                return resolved;
            }
            else if (left_result->type == VariableType::DEQUE) {
                shared_ptr<Variable> resolved = std::make_shared<Variable>();
                resolved->type = VariableType::DEQUE;
                for (shared_ptr<Variable> var : left_vector) {
                    resolved->value_deque.push_back(var);
                }
                return resolved;
            }

        }
    }
    else if (left_result->type == VariableType::BOOL) {

        shared_ptr<Variable> resolved = std::make_shared<Variable>();
        resolved->type = VariableType::BOOL;
        switch (this->operato) {
            case Operator::AND:                       resolved->value_bool = left_result->value_bool && right_result->value_bool;
            case Operator::OR:                        resolved->value_bool = left_result->value_bool || right_result->value_bool;
            case Operator::EQUALS:                    resolved->value_bool = left_result->value_bool == right_result->value_bool;
            case Operator::DOES_NOT_EQUAL:            resolved->value_bool = left_result->value_bool != right_result->value_bool;
            case Operator::GREATER_THAN:              resolved->value_bool = left_result->value_bool > right_result->value_bool;
            case Operator::LESS_THAN:                 resolved->value_bool = left_result->value_bool < right_result->value_bool;
            case Operator::GREATER_THAN_OR_EQUAL:     resolved->value_bool = left_result->value_bool >= right_result->value_bool;
            case Operator::LESS_THAN_OR_EQUAL:        resolved->value_bool = left_result->value_bool <= right_result->value_bool;
            default:                                  resolved->value_bool = left_result->value_bool == right_result->value_bool;
        }
        return resolved;
    }
    

    return nullptr;

}
