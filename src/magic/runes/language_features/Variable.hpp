#pragma once


#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <deque>
#include <memory>
#include <sstream>
#include "ExecutableUnit.h"
#include <sstream>



using namespace std;

enum VariableType {
    NULL_VOID,
    INT,
    DOUBLE,
    BOOL,
    STRING,
    LIST,
    DICTIONARY,
    SET,
    TUPLE,
    DEQUE,
    UNKNOWN_TYPE
};

string variable_type_name(VariableType type);


struct Variable : public ExecutableUnit {

    // Hash function for Variables. Needed for sets
    struct VariableHash {
        std::size_t operator()(const std::shared_ptr<Variable>& var) const {
            
            if (var->type == VariableType::NULL_VOID) {
                return std::hash<int>()(var->type);
            }
            else if (var->type == VariableType::INT) {
                return std::hash<int>()(var->type) ^ std::hash<int>()(var->value_int);
            }
            else if (var->type == VariableType::DOUBLE) {
                return std::hash<int>()(var->type) ^ std::hash<double>()(var->value_double);
            }
            else if (var->type == VariableType::BOOL) {
                return std::hash<bool>()(var->value_bool);
            }
            else if (var->type == VariableType::STRING) {
                return std::hash<int>()(var->type) ^ std::hash<string>()(var->value_string);
            }
            else {
                return std::hash<std::string>()(var->name);
            }
        }
    };
    // Equality function for Variables. Needed for sets
    struct VariableEqual {
        bool operator()(const std::shared_ptr<Variable>& lhs, const std::shared_ptr<Variable>& rhs) const {
            bool same_type = lhs->type == rhs->type;
            if (same_type) {
                if (lhs->type == VariableType::NULL_VOID) {
                    return true;
                }
                else if (lhs->type == VariableType::INT) {
                    return lhs->value_int == rhs->value_int;
                }
                else if (lhs->type == VariableType::DOUBLE) {
                    return lhs->value_double == rhs->value_double;
                }
                else if (lhs->type == VariableType::BOOL) {
                    return lhs->value_bool == rhs->value_bool;
                }
                else if (lhs->type == VariableType::STRING) {
                    return lhs->value_string == rhs->value_string;
                }
                else {
                    return lhs->name == rhs->name;
                }
            }
            else {
                return false;
            }
        }
    };
    
    
    VariableType type;
    bool immutable = false;

    int value_int;
    double value_double;
    bool value_bool;
    string value_string;
    vector<shared_ptr<Variable>> value_list;
    unordered_map<string, shared_ptr<Variable>> value_dictionary;
    unordered_set<shared_ptr<Variable>, VariableHash, VariableEqual> value_set;
    vector<shared_ptr<Variable>> value_tuple;
    deque<shared_ptr<Variable>> value_deque;

    
    shared_ptr<ExecutableUnit> resolve(
        unordered_map<string, shared_ptr<ExecutableUnit>>& global_functions, 
        unordered_map<string, shared_ptr<ExecutableUnit>>& global_variables,
        stringstream& output,
        unordered_map<string, shared_ptr<ExecutableUnit>>& local_variables,
        ErrorCatcher* error_catcher
    ) override {
        shared_ptr<Variable> resolved = std::make_shared<Variable>();
        resolved->name = this->name;


        resolved->type = this->type;
        
        resolved->value_int = this->value_int;
        resolved->value_double = this->value_double;
        resolved->value_bool = this->value_bool;
        resolved->value_string = this->value_string;
        resolved->value_list = this->value_list;
        resolved->value_dictionary = this->value_dictionary;
        resolved->value_set = this->value_set;
        resolved->value_tuple = this->value_tuple;
        resolved->value_deque = this->value_deque;
        
        return resolved;
    }
        


    static void add_varaible_to_global_or_local_context(
        shared_ptr<Variable>& var,
        unordered_map<string, shared_ptr<ExecutableUnit>>& global_variables, 
        unordered_map<string, shared_ptr<ExecutableUnit>>& local_variables
    ) {


        bool defined_local_variable = local_variables.find(var->name) != local_variables.end();
        bool global_variable = global_variables.find(var->name) != global_variables.end();
        if (global_variable && !defined_local_variable) {
            global_variables[var->name] = var;
        }
        else {
            local_variables[var->name] = var;
        }
    }

    static void get_variable_value(
        shared_ptr<Variable>& var, 
        unordered_map<string, shared_ptr<ExecutableUnit>>& global_variables, 
        unordered_map<string, shared_ptr<ExecutableUnit>>& local_variables,
        int line,
        ErrorCatcher* error_catcher
    ) {

        if (local_variables.find(var->name) != local_variables.end()) {
            var = static_pointer_cast<Variable>(local_variables[var->name]);
        }
        else {
            
            if (global_variables.find(var->name) != global_variables.end()) {
                var = static_pointer_cast<Variable>(global_variables[var->name]);
            }
            else {
                error_catcher->error("Can't find variable '" + var->name + "' here", line);
            }
        }
    }


    vector<shared_ptr<Variable>> get_as_vector() {
        
        vector<shared_ptr<Variable>> as_vector;
        if (this->type == VariableType::LIST) {
            as_vector = this->value_list;
        }
        else if (this->type == VariableType::SET) {
            for (const shared_ptr<Variable>& variable : this->value_set) {
                as_vector.push_back(variable);
            }
        }
        else if (this->type == VariableType::TUPLE) {
            as_vector = this->value_tuple;
        }
        else if (this->type == VariableType::DEQUE) {
            for (const shared_ptr<Variable>& variable : this->value_set) {
                as_vector.push_back(variable);
            }
        }
        
        return as_vector;
    }

    bool is_data_structure() {
        return this->type == VariableType::LIST || this->type == VariableType::DICTIONARY || this->type == VariableType::SET || this->type == VariableType::TUPLE || this->type == VariableType::DEQUE;
    }

    bool is_numeric() {
        return this->type == VariableType::INT || this->type == VariableType::DOUBLE;
    }

    string get_value_as_string() {
        if (this->type == VariableType::NULL_VOID) {
            return "null"; 
        }
        else if (this->type == VariableType::INT) {
            return to_string(value_int);
        }
        else if (this->type == VariableType::DOUBLE) {
            string str = to_string(this->value_double);
            
            // remove extra zeros from output like this: '4.5000'
            if (str.find('.') != std::string::npos) {
                int last = str.length() - 1;
                for (int i = str.length() - 1; i >= 0; --i) {
                    if (str[i] != '0' && str[i] != '.') {
                        last = i;
                        break;
                    }
                }

                str = str.substr(0, last + 1);
            }

            return str;
        }
        else if (this->type == VariableType::BOOL) {
            return to_string(value_bool);
        }
        else if (this->type == VariableType::STRING) {
            return value_string;
        }
        else if (this->type == VariableType::LIST) {
            stringstream ss;
            ss << "[" << endl;
            for (const shared_ptr<Variable>& varaible : this->value_list) {
                ss << varaible->get_value_as_string() << "," << endl;
            }
            ss << "]" << endl;

            return ss.str();
        }
        else if (this->type == VariableType::DICTIONARY) {
            stringstream ss;
            ss << "{" << endl;
            for (const auto& pair : this->value_dictionary) {
                ss << pair.first << "=" << pair.second->get_value_as_string() << "," << endl;
            }
            ss << "}" << endl;

            return ss.str();
        }
        else if (this->type == VariableType::SET) {
            stringstream ss;
            ss << "{" << endl;
            for (const shared_ptr<Variable>& varaible : this->value_set) {
                ss << varaible->get_value_as_string() << "," << endl;
            }
            ss << "}" << endl;

            return ss.str();
        }
        else if (this->type == VariableType::TUPLE) {
            stringstream ss;
            ss << "(" << endl;
            for (const shared_ptr<Variable>& varaible : this->value_tuple) {
                ss << varaible->get_value_as_string() << "," << endl;
            }
            ss << ")" << endl;

            return ss.str();
        }
        else if (this->type == VariableType::DEQUE) {
            stringstream ss;
            ss << "[" << endl;
            for (const shared_ptr<Variable>& varaible : this->value_deque) {
                ss << varaible->get_value_as_string() << "," << endl;
            }
            ss << "]" << endl;

            return ss.str();
        }

        return "";
    }

    bool is_immutable_as_function_param() {
        return 
            this->type == VariableType::NULL_VOID || 
            this->type == VariableType::INT || 
            this->type == VariableType::DOUBLE || 
            this->type == VariableType::BOOL || 
            this->type == VariableType::STRING || 
            this->type == VariableType::TUPLE;
    }

};

