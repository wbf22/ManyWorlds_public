#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "Variable.hpp"
#include "Line.h"
#include <memory>
#include "ExecutableUnit.h"
#include "SystemFunctions.hpp"

using namespace std;


struct Function : public ExecutableUnit {

    string name;
    vector<string> parameters;
    vector<shared_ptr<Line>> lines_or_blocks;
    shared_ptr<ExecutableUnit> return_value = nullptr;
    bool is_system_function = false;


    shared_ptr<ExecutableUnit> resolve(
        unordered_map<string, shared_ptr<ExecutableUnit>>& global_functions, 
        unordered_map<string, shared_ptr<ExecutableUnit>>& global_variables,
        stringstream& output,
        unordered_map<string, shared_ptr<ExecutableUnit>>& parameter_variables,
        ErrorCatcher* error_catcher
    ) override {
        if (this->is_system_function) {
            return this->call_system_function(
                global_variables,
                output,
                parameter_variables
            );
        }
        
        unordered_map<string, shared_ptr<ExecutableUnit>> local_variables;
    
        // make copies of immutable variables or transfer references
        for (const auto& pair : parameter_variables) {
            shared_ptr<Variable> var = static_pointer_cast<Variable>(  
                parameter_variables[pair.first]
            );
            if (var->is_immutable_as_function_param()) {
                // the resolve function for variables just makes a copy
                local_variables[var->name] = var->resolve(
                    global_functions,
                    global_variables,
                    output,
                    parameter_variables, 
                    error_catcher
                );
            }
            else {
                local_variables[var->name] = var;
            }
    
        }
        
        // execute function lines
        for (shared_ptr<Line>& line : this->lines_or_blocks) {
            shared_ptr<ExecutableUnit> line_res = line->resolve(
                global_functions, 
                global_variables, 
                output, 
                local_variables, 
                error_catcher
            );

            // get return variable if function has a return
            if (line->type == LineType::RETURN_STATEMENT) {
                return line_res;
            }
        }
    
        return nullptr;
    }

    shared_ptr<ExecutableUnit> call_system_function(
        unordered_map<string, shared_ptr<ExecutableUnit>>& global_variables,
        stringstream& output,
        unordered_map<string, shared_ptr<ExecutableUnit>>& parameter_variables
    ) {
        if (this->name == "print") {
            shared_ptr<Variable> first = static_pointer_cast<Variable>(
                parameter_variables.begin()->second
            );

            string as_str = first->get_value_as_string();
            print(as_str, output);
        }


        return nullptr;
    }
};