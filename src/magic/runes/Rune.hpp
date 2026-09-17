#pragma once

#include "language_features/LogicExpression.h"
#include "language_features/Variable.hpp"
#include "language_features/Line.h"
#include "language_features/Function.hpp"
#include "language_features/Operator.h"
#include "language_features/ErrorCatcher.h"
#include <vector>
#include <sstream>
#include <memory>
#include <cctype>
#include <iostream>

#define BLUE "\033[1;34m"
#define RED "\033[1;31m"
#define ORANGE "\033[1;33m"
#define GREEN "\033[1;32m"
#define RESET "\033[0m"


using namespace std;


enum Magic  {
    SPIRIT_REALM,
    EXPLOSION,
    TEMPEST
};


/**
 * Compiles Runes into magical effects in the game.
 * 
 * Uses the same syntax as python but with a limited language feature set:
 *  - variables
 *  - functions
 *  - print statments
 *  - order of operations and math
 *  - lists
 *  - dictionaries
 *  - sets
 *  - tuples
 *  - deques
 *  - for and foreach loops
 *  - while loops
 * 
 * 
 * 
 * Doesn't bother with these features of python and any other features not listed above:
 *  - classes
 *  - type hints
 *  - imports
 * 
 * 
 * Cool fonts we could use in the game https://www.fontspace.com/category/runic
 * google runic font https://fonts.google.com/noto/specimen/Noto+Sans+Runic
 */
struct Rune : public ErrorCatcher {


    unordered_map<string, shared_ptr<ExecutableUnit>> global_functions = define_system_functions();
    unordered_map<string, shared_ptr<ExecutableUnit>> global_variables;
    stringstream output;
    bool error_thrown = false;
    string run(string& code) {

        
        vector<shared_ptr<Line>> lines; 

        // remove comments
        remove_comments(code);

        // convert code into function and varaibles and expressions
        istringstream stream(code);
        int line_number = 0;
        string line;
        int indents = 0;

        int status = 0;
        while(status != -1 && !error_thrown) {
            shared_ptr<Line> line_block = std::make_shared<Line>();
            status = parse_line(stream, line, indents, line_number, line_block);
            
            // save line if not indent change
            if (status == 0 && line_block->type != LineType::EMPTY_LINE) {
                lines.push_back(line_block);
            }
        }
        

        // execute what they do including system functions for magic
        unordered_map<string, shared_ptr<ExecutableUnit>> local_variables;
        for (shared_ptr<Line>& line : lines) {
            line->resolve( global_functions, global_variables, output, local_variables, this);
            if (error_thrown) {
                return output.str();
            }
        }

        
        return output.str();
    }


    int parse_line(istringstream& stream, string& line, int& indents, int& line_number, shared_ptr<Line>& result) {
        streampos pos = stream.tellg();
        if (std::getline(stream, line)) {
                
            int current_indents= 0;
            remove_leading_whitespace(line, current_indents);
            if (current_indents != indents) {
                stream.seekg(pos);
                indents = current_indents;
                return 1;
            }
            line_number++;
            result->number = line_number;

            if (line.length() == 0) {
                return 0;
            }
            // function definition
            else if (starts_with(line, "def ")) {
                shared_ptr<Function> function = parse_function(stream, line, current_indents, line_number);
                                
                global_functions[function->name] = function;

                return 0;
            }
            // return statement
            else if (starts_with(line, "return")) {
                string expression = line.substr(6);
                result->type = LineType::RETURN_STATEMENT;
                int i = 0;
                result->expression = parse_expression(expression, line_number, i);

                return 0;
            }
            // for loop
            else if (starts_with(line, "for ")) {
                int i = 0;
                string i_name = substr_to(line, 4, ' ', i);
                result->i_name = i_name;
                string expected_in = substr_to(line, i+1, ' ', i);
                if (expected_in != "in") this->error("Missing 'in' after 'for <var> '", line_number);
                if (line[line.length()-1] != ':') this->error("Missing colon on end of loop first line", line_number);

                int paran;
                substr_to(line, i, '(', paran);
                // foreach
                if (paran == -1) {
                    result->type = LineType::FOR_EACH;
                    result->for_each_target = substr_to(line, i, ':', i);
                }
                // for with range
                else {
                    result->type = LineType::FOR_LOOP;
                    string range_params = substr_to(line, paran+1, ')', i);
                    remove_whitespace(range_params);
                    vector<string> params = split(range_params, ",");
                    vector<shared_ptr<Variable>> param_vars;
                    for (string param : params) {
                        shared_ptr<Variable> var = std::make_shared<Variable>();
                        if (std::isdigit(param[0])) {
                            try {
                                if (contains(param, ".")) {
                                    this->error("Can't do a loop with values that aren't integers (whole numbers)", line_number);
                                }
                                else {
                                    var->type = VariableType::INT;
                                    var->value_int = std::stoi(param);
                                }
                            }
                            catch(exception& e) {
                                this->error("Error parsing number here from " + param, line_number);
                            }
                        }
                        else {
                            var->type = VariableType::UNKNOWN_TYPE;
                            var->name = param;
                        }

                        param_vars.push_back(var);
                    }
                    
                    // defaults
                    result->step = std::make_shared<Variable>();
                    result->step->type = VariableType::INT;
                    result->step->value_int = 1;
                    result->i = std::make_shared<Variable>();
                    result->i->type = VariableType::INT;
                    result->i->value_int = 0;
                    
                    // set from params
                    if (param_vars.size() == 1) {
                        result->end = param_vars[0];
                    }
                    else if (param_vars.size() == 2) {
                        result->end = param_vars[1];
                        result->i = param_vars[0];

                    }
                    else if (param_vars.size() == 3) {
                        result->step = param_vars[2];
                        result->end = param_vars[1];
                        result->i = param_vars[0];
                    }
                    else {
                        this->error("Weird range() function. Examples: range(<end>) range(<start>, <end>) range(<start>, <end>, <step>)", line_number);
                    }
                    
                }

                result->loop_block = parse_block(stream, line, current_indents, line_number);

                return 0;
            }
            // while loop
            else if (starts_with(line, "while ")) {
                
            }
            // assignments
            else if (contains(line, "=")) {

                vector<string> splits = split(line, "=");

                // parse variable name
                string var_name = splits[0];
                remove_whitespace(var_name);
                char c = var_name[var_name.length() -1];
                if ( c == '-' || c == '+') {
                    this->error("+= or -= aren't supported yet", line_number);
                }

                // parse expression
                string expression_str = splits[1];
                int i = 0;
                shared_ptr<ExecutableUnit> expression = parse_expression(expression_str, line_number, i);

                // set on result
                result->type = LineType::VARIABLE_SET;
                result->variable_target = var_name;
                result->expression = expression;

                return 0;
            }
            // plain function call (or nonsense expression)
            else if (contains(line, "(")) {

                int na = 0;
                result = parse_function_call(line, na, line_number);
                return 0;
            }

            return 0;
        }
        else {
            return -1;
        }

    }

    shared_ptr<ExecutableUnit> parse_expression(const string& str, const int& line_number, int& i) {


        // replace 'and' and 'or' keywords with '&' and '|'
        string exp_str = replace(str, " and ", "&");
        exp_str = replace(exp_str, " or ", "|");
        remove_whitespace(exp_str);

        // 4*(a+(b/9))/b
        // 4+5*7
        // var*19+2
        // 3*-2+5
        // -4*9
        // ("pink"==myvar)||("pink"==othervar)
        shared_ptr<ExecutableUnit> left = nullptr;
        Operator operato = Operator::NONE;
        shared_ptr<ExecutableUnit> right = nullptr;
        bool is_neg = false;
        int right_start_i = -1;
        while (i < exp_str.length()) {
            

            // parse number
            if (std::isdigit(exp_str[i])) {
                bool is_double = false;
                int start = i;
                while(std::isdigit(exp_str[i]) || exp_str[i] == '.') {
                    is_double = is_double || exp_str[i] == '.';
                    ++i;
                }

                shared_ptr<Variable> var = std::make_shared<Variable>();
                if (is_double) {
                    var->type = VariableType::DOUBLE;
                    var->value_double = parse_double(
                        exp_str.substr(start, i),
                        line_number
                    );
                }
                else {
                    var->type = VariableType::INT;
                    var->value_int = parse_int(
                        exp_str.substr(start, i),
                        line_number
                    );
                }

                if (is_neg) {
                    if (is_double)
                        var->value_double *= -1;
                    else
                        var->value_int *= -1;

                    is_neg = false;
                }

                if (left == nullptr) {
                    left = var;
                }
                else if (right == nullptr) {
                    right = var;
                }
            }
            // operators
            else if (is_operator(exp_str[i])) {
                Operator new_operator = parse_operator(exp_str, i);

                // first operator
                if (operato == Operator::NONE) {

                    // if the first thing in the expression is negative
                    if (left == nullptr) {
                        if (new_operator == Operator::SUBTRACT) {
                            is_neg = true;
                        }
                        else {
                            this->error("Extra operator at the start of the line", line_number);
                        }
                    }
                    // otherwise set operato
                    else {
                        operato = new_operator;
                        right_start_i = i;
                    }
                }
                // negative value
                else if (new_operator == Operator::SUBTRACT && right == nullptr) {
                    is_neg = true;
                }
                // boolean operators
                else if (is_boolean_operator(new_operator)) {
                    
                        // make a new expression and set it as 'left'
                        shared_ptr<LogicExpression> new_left = std::make_shared<LogicExpression>();
                        new_left->left_hand = left;
                        new_left->operato = operato;
                        new_left->right_hand = right;
                        left = new_left;

                        // set new operator
                        operato = new_operator;
                        right_start_i = i;
                }
                // math operators
                else {

                    if (is_higher_order(new_operator, operato)) {
                        
                        // if higher order reset to right operand and then parse the next expression
                        right = nullptr;
                        string exp_remainder = exp_str.substr(right_start_i);
                        int index = 0;
                        right = parse_expression(
                            exp_remainder,
                            line_number,
                            index
                        );
                        i += index;
                    }
                    else {

                        // make a new expression and set it as 'left'
                        shared_ptr<LogicExpression> new_left = std::make_shared<LogicExpression>();
                        new_left->left_hand = left;
                        new_left->operato = operato;
                        new_left->right_hand = right;
                        left = new_left;

                        // set new operator
                        operato = new_operator;
                        right = nullptr;
                        right_start_i = i;
                    }

                }
            }
            // parse parenthesis
            else if (exp_str[i] == '(') {

                // find matching end and parse that as a seperate expression
                ++i;
                int j = i;
                int parens = 0;
                while (exp_str[j] != ')' || parens != 0) {
                    if (exp_str[j] == '(') ++parens;
                    else if (exp_str[j] == ')') --parens;

                    j++;
                    if (j == exp_str.length()) {
                        this->error("Missing a matching ')'", line_number);
                    }
                }

                string exp_string = exp_str.substr(i, j - i);
                int index = 0;
                shared_ptr<ExecutableUnit> paren_expression = parse_expression(
                    exp_string,
                    line_number,
                    index
                );
                i += index;

                if (left == nullptr) {
                    left = paren_expression;
                }
                else {
                    right = paren_expression;
                }
                i++;
            }
            // parse string
            else if (exp_str[i] == '\"') {

                int start = i;
                i++;
                while (exp_str[i] != '\"') {
                    i++;
                }

                shared_ptr<Variable> var = std::make_shared<Variable>();
                var->type = VariableType::STRING;
                var->value_string = exp_str.substr(start+1, i-(start+1));

                if (left == nullptr) {
                    left = var;
                }
                else if (right == nullptr) {
                    right = var;
                }

                i++;
            }
            // parse variable name or function call
            else {
                int start = i;
                while (exp_str[i] != '(' && !is_operator(exp_str[i]) && i < exp_str.length()) {
                    i++;
                }

                bool is_function_call = exp_str[i] == '(';
                shared_ptr<ExecutableUnit> var_or_func_call;
                if (is_function_call) {
                    i = start;
                    string rest = exp_str.substr(start);
                    var_or_func_call = parse_function_call(rest, i, line_number);
                }
                else {
                    shared_ptr<Variable> var = std::make_shared<Variable>();
    
                    var->name = exp_str.substr(start, i - start);
                    var->type = VariableType::UNKNOWN_TYPE;
    
                    var_or_func_call = var;
                }


                if (left == nullptr) {
                    left = var_or_func_call;
                }
                else if (right == nullptr) {
                    right = var_or_func_call;
                }


            }

        }

        if (left != nullptr) left->line = line_number;
        if (right != nullptr) right->line = line_number;


        if (operato == Operator::NONE && right == nullptr) {
            return left;
        }


        shared_ptr<LogicExpression> expression = std::make_shared<LogicExpression>();
        expression->line = line_number;
        expression->left_hand = left;
        expression->operato = operato;
        expression->right_hand = right;
        expression->line = line_number;

        return expression;
    }

    shared_ptr<Line> parse_function_call(string& line, int& i, int line_number) {
        int index;
        string function_name = substr_to(line, 0, '(', index);
        i += function_name.length();

        // get function
        shared_ptr<Function> func;
        if (this->global_functions.find(function_name) != this->global_functions.end()) {
            func = static_pointer_cast<Function>(  
                this->global_functions[function_name]
            );
        }
        else {
            this->error("'" + function_name + "' hasn't be defined before this line", line_number);
        }
        
        // extract parameter expressions
        string range_params = line.substr(index+1, line.length()-2-index);
        vector<string> params;
        int paren_count = 0;
        int last = 0;
        for (int j = 0; j < range_params.length(); ++j) {
            if (range_params[j] == ',' && paren_count == 0) {
                string param = range_params.substr(last, j-last);
                remove_leading_and_trailing_whitespace(param);
                params.push_back(param);
                last = j + 1;
            }
            else if (range_params[j] == '(') {
                ++paren_count;
            }
            else if (range_params[j] == ')') {
                --paren_count;
            }
        }
        string param = range_params.substr(last);
        i += last + param.length() + 2;
        remove_leading_and_trailing_whitespace(param);
        params.push_back(param);
        if (params.size() != func->parameters.size()) {
            this->error(
                "'" + function_name + "' takes " + 
                    to_string(func->parameters.size()) + " arguments not " +
                    to_string(params.size())
                , line_number
            );
        }

        // collect params as vars
        vector<shared_ptr<ExecutableUnit>> param_vars;
        for (int i = 0; i < params.size(); ++i) {
            string param = params[i];
            int j = 0;
            shared_ptr<ExecutableUnit> var = parse_expression(param, line_number, j);
            var->name = func->parameters[i];
            var->parameter_binding = param;
            param_vars.push_back(var);
        }


        // make result
        shared_ptr<Line> result = std::make_shared<Line>();
        result->type = LineType::FUNCTION_CALL;
        result->function_name = function_name;
        result->input_expressions = param_vars;

        return result;
    }

    shared_ptr<Function> parse_function(istringstream& stream, string& line, int indents, int& line_number) {
        
        shared_ptr<Function> function = std::make_shared<Function>();
        if (line[line.length() - 1] != ':') {
            this->error("Missing colon on end of function first line", line_number);
        }
        
        // get name and parameters
        int found_index;
        function->name = substr_to(line, 4, '(', found_index);
        string params_string = substr_to(line, found_index+1, ')', found_index);
        remove_whitespace(params_string);
        vector<string> params = split(params_string, ",");
        for (string& param : params) {
            function->parameters.push_back(param);
        }

        function->lines_or_blocks = parse_block(stream, line, indents, line_number);


        return function;
    }

    vector<shared_ptr<Line>> parse_block(istringstream& stream, string& line, int indents, int& line_number) {
        vector<shared_ptr<Line>> lines_blocks;
        
        int signal = 0;
        int sub_indents = indents + 1;
        while (sub_indents == indents + 1 && signal == 0) {
            shared_ptr<Line> line_block = std::make_shared<Line>();
            signal = parse_line(stream, line, sub_indents, line_number, line_block);
            if (line_block->type != LineType::EMPTY_LINE) {
                lines_blocks.push_back(line_block);
            }

            line_number++;
        }

        return lines_blocks;
    }

    void remove_comments(string& code) {
        stringstream ss;
        istringstream stream(code);
        string line;
        while (std::getline(stream, line)) {
            int in;
            line = substr_to(line, 0, '#', in);
            ss << line << endl;
        }
        code = ss.str();
    }

    static unordered_map<string, shared_ptr<ExecutableUnit>> define_system_functions() {
        unordered_map<string, shared_ptr<ExecutableUnit>> functions;

        // print
        shared_ptr<Function> print_function = std::make_shared<Function>();
        print_function->parameters.push_back("text");
        print_function->is_system_function = true;
        print_function->name = "print";
        functions[print_function->name] = print_function;

        return functions;
    }
    
    void error(string msg, int line_number) override {
        error_thrown = true;
        output << RED << msg + "\n   line - " + std::to_string(line_number) << RESET << endl;
        cout << RED << msg + "\n   line - " + std::to_string(line_number) << RESET << endl;
    }


    // string utils
    vector<string> split(const string& str, string c) {
        vector<string> splits;
        int last = 0;
        if (c.length() == 1) {
            for (int i = 0; i < str.length(); ++i) {
                if (str[i] == c[0]) {
                    splits.push_back(
                        str.substr(last, i)
                    );
                    last = i + 1;
                }
            }
            if (last < str.length()) {
                splits.push_back(
                    str.substr(last)
                );
            }
        }
        else {
            for (int i = 0; i < str.length() - c.length(); ++i) {
                if (str.substr(i, i + c.length()) == c) {
                    splits.push_back(
                        str.substr(last, i)
                    );
                    last = i + c.length();
                }
            }
            if (last < str.length() - c.length()) {
                splits.push_back(
                    str.substr(last)
                );
            }
        }

        return splits;
    }

    string substr_to(const string& str, int start, char to, int& found_index) {
        for (int i = start; i < str.length(); ++i) {
            if (str[i] == to) {
                found_index = i;
                return str.substr(start, found_index - start);
            }
        }
        found_index = -1;
        return str.substr(start);
    }

    void walk_to_end_of(const string& str, int start, const string& to, int& end_index) {
        
        for (int i = 0; i < to.length(); ++i) {
            if (str[start + i] != to[i]) {
                end_index = -1;
                return;
            }
        }

        end_index = start + to.length();
    }

    void remove_leading_whitespace(string& line, int& count) {
        int i = 0; 
        int spaces = 0;
        while( (line[i] == '\t' || line[i] == ' ') && i < line.length()) {
            if (line[i] == '\t') {
                spaces += 4;
            }
            else if (line[i] == ' ') {
                spaces += 1;
            }

            ++i;
        }

        line = line.substr(i, line.length() - i);
        count = spaces / 4;
    }

    void remove_leading_and_trailing_whitespace(string& line) {
        int start = 0; 
        while( (line[start] == '\t' || line[start] == ' ') && start < line.length()) {
            ++start;
        }

        int end = line.length() - 1; 
        while( (line[end] == '\t' || line[end] == ' ') && end >= 0) {
            --end;
        }

        line = line.substr(start, end + 1 - start);
    }

    void remove_whitespace(string& str) {
        stringstream ss;
        for(int i = 0; i < str.length(); ++i) {
            if (str[i] != ' ' && str[i] != '\t' && str[i] != '\n') {
                ss << str[i];
            }
        }

        str = ss.str();
    }
    
    double parse_double(string str, const int& line_number) {
        try {
            return std::stod(
                str
            );
        }
        catch(...) {
            this->error("Failed trying to parse a number here", line_number);
        }
        return 0.0;
    }

    double parse_int(string str, const int& line_number) {
        try {
            return std::stoi(
                str
            );
        }
        catch(...) {
            this->error("Failed trying to parse a number here", line_number);
        }
        return 0;
    }

    string replace(const string& str, string to_replace, string replacement) {
        stringstream ss;
        for (int i = 0; i < str.length(); i++) {
            if (str[i] == to_replace[0]) {
                int j = 0;
                while(i+j < str.length() && j < to_replace.length() && str[i+j] == to_replace[j]) {
                    ++j;
                }
                if (j == to_replace.length()) {
                    i += j - 1;
                    ss << replacement;
                }
                else {
                    ss << str[i];
                }
            }
            else {
                ss << str[i];
            }
        }

        return ss.str();
    }

    bool starts_with(const string& str, string start) {
        for (int i = 0; i < start.length(); ++i) {
            if (start[i] != str[i]) {
                return false;
            }
        }
        return true;
    }

    bool contains(const string& str, string test) {
        for (int i = 0; i < str.length(); i++) {
            if (str[i] == test[0]) {
                int j = 0;
                while(i+j < str.length() && j < test.length() && str[i+j] == test[j]) {
                    ++j;
                }
                if (j == test.length()) {
                    return true;
                }
            }
            
        }

        return false;

    }

};