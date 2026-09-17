
#include "Line.h"


shared_ptr<ExecutableUnit> Line::resolve(
    unordered_map<string, shared_ptr<ExecutableUnit>>& global_functions, 
    unordered_map<string, shared_ptr<ExecutableUnit>>& global_variables,
    stringstream& output,
    unordered_map<string, shared_ptr<ExecutableUnit>>& local_variables,
    ErrorCatcher* error_catcher
) {
    shared_ptr<ExecutableUnit> result = nullptr;


    if (this->type == LineType::VARIABLE_SET) {

        shared_ptr<Variable> result = static_pointer_cast<Variable>(
            this->expression->resolve(global_functions, global_variables, output, local_variables, error_catcher)
        );
        result->name = variable_target;
        Variable::add_varaible_to_global_or_local_context(result, global_variables, local_variables);
    }
    else if (this->type == LineType::FUNCTION_CALL) {

        unordered_map<string, shared_ptr<ExecutableUnit>> input_params;
        int const_count = 0;
        for (shared_ptr<ExecutableUnit> exp : this->input_expressions) {
            shared_ptr<Variable> param = static_pointer_cast<Variable>(
                exp->resolve(global_functions, global_variables, output, local_variables, error_catcher)
            );
            if (param->type == VariableType::UNKNOWN_TYPE) {
                param->name = exp->parameter_binding;
                Variable::get_variable_value(param, global_variables, local_variables, this->number, error_catcher); 
            }
            param->name = exp->name;
            input_params[param->name] = param;
        }
        shared_ptr<ExecutableUnit> func = global_functions[function_name];
        result = func->resolve(global_functions, global_variables, output, input_params, error_catcher);
        
    }
    else if (this->type == LineType::FOR_LOOP) {
        
        local_variables[this->i_name] = this->i;
        if (this->end->type == VariableType::UNKNOWN_TYPE) {
            Variable::get_variable_value(this->end, global_variables, local_variables, this->number, error_catcher); 
        }
        if (this->step->type == VariableType::UNKNOWN_TYPE) {
            Variable::get_variable_value(this->step, global_variables, local_variables, this->number, error_catcher); 
        }

        for (int ii = this->i->value_int; ii < this->end->value_int; ii+=this->step->value_int) {

            for (shared_ptr<Line>& line : this->loop_block) {
                line->resolve(global_functions, global_variables, output, local_variables, error_catcher);
            }
            ++this->i->value_int;
        }
        local_variables.erase(this->i_name);

    }
    else if (this->type == LineType::FOR_EACH) {
        
    }
    else if (this->type == LineType::WHILE) {
        
    }
    else if (this->type == LineType::FUNCTION_DEFINITION) {
        // nothing since the function is parsed already
    }
    else if (this->type == LineType::RETURN_STATEMENT) {
        result = this->expression->resolve(global_functions, global_variables, output, local_variables, error_catcher);
    }

    return result;
}




