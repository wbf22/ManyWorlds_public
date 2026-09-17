#pragma once




#include <type_traits>
#include <tuple>
#include <cstring>
#include <unordered_map>
#include <vector>
#include <unordered_set>
#include <string>
#include "Serializer.hpp"
#include <stdexcept>
#include <sstream>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <memory>


/**
 * This macro allows the byte serializer to know what fields a struct has, so it can
 * iterate over them and serialize/deserialize them.
 * 
 * used like so in a struct or class:
 * 
    DECLARE_NAMED_FIELDS(
        FIELD("type", &MyStruct::type),
        FIELD("status", &MyStruct::status)
    );
 */
#define DECLARE_NAMED_FIELDS(...) static constexpr auto fields() { return std::make_tuple(__VA_ARGS__); }

// Define a macro to create a pair of field name and field pointer
#define FIELD(name, field_ptr) std::make_pair(name, field_ptr)

// Helper to get the type of a field
template <typename T, typename F>
struct named_field_type;

template <typename T, typename R>
struct named_field_type<T, R T::*> {
    using type = R;
};


// stuff to check if a type is a vector during runtime
template <typename T, typename E = void>
struct is_vector_kv : std::false_type {};

template <typename E>
struct is_vector_kv<std::vector<E>> : std::true_type {};


// stuff to check if a type is an array during runtime
template <typename T>
struct is_std_array_kv : std::false_type {};

template <typename T, std::size_t N>
struct is_std_array_kv<std::array<T, N>> : std::true_type {};


// stuff to check if a type is a set during runtime
template <typename T, typename E = void>
struct is_set_kv : std::false_type {};

template <typename E>
struct is_set_kv<std::unordered_set<E>> : std::true_type {};


// stuff to check if a type is a map during runtime
template <typename T>
struct is_map_kv : std::false_type {};

template <typename K, typename V>
struct is_map_kv<std::unordered_map<K, V>> : std::true_type {};


// stuff to check if a type is a string during runtime
template <typename T>
struct is_string_kv : std::false_type {};

template <>
struct is_string_kv<std::string> : std::true_type {};


// Type trait to detect shared_ptr
template <typename T>
struct is_shared_ptr_kv : std::false_type {};

template <typename T>
struct is_shared_ptr_kv<std::shared_ptr<T>> : std::true_type {};


// stuff to check if a type is a struct or class during runtime
template <typename T>
struct is_struct_kv {
    static constexpr bool value = std::is_class<T>::value 
    && !is_vector_kv<T>::value 
    && !is_std_array_kv<T>::value 
    && !is_set_kv<T>::value 
    && !is_map_kv<T>::value 
    && !is_string_kv<T>::value
    && !is_shared_ptr_kv<T>::value;
};


// stuff to check if type is a primitive value during runtime
template <typename T>
struct is_primitive_kv {
    static constexpr bool value = !is_vector_kv<T>::value 
    && !is_std_array_kv<T>::value 
    && !is_set_kv<T>::value 
    && !is_struct_kv<T>::value 
    && !is_map_kv<T>::value 
    && !is_string_kv<T>::value
    && !is_shared_ptr_kv<T>::value;
};


// stuff to check if type is a float value during runtime
template <typename T>
struct is_float : std::false_type {};

template <>
struct is_float<float> : std::true_type {};


// stuff to check if type is a double value during runtime
template <typename T>
struct is_double : std::false_type {};

template <>
struct is_double<double> : std::true_type {};


// stuff to check if type is a decimal value during runtime
template <typename T>
struct is_decimal {
    static constexpr bool value = is_float<T>::value || is_double<T>::value;
};


// stuff to check if type is a decimal value during runtime
template <typename T>
struct is_int {
    static constexpr bool value = is_primitive<T>::value && !is_decimal<T>::value;
};


// stuff to iterate over a tuple
#include <utility> // for std::index_sequence and std::make_index_sequence
template <typename Tuple, typename Func, std::size_t... Is>
void for_each_impl_kv(Tuple&& tuple, Func&& func, std::index_sequence<Is...>) {
    (func(std::get<Is>(tuple)), ...);
}

template <typename Tuple, typename Func>
void for_each_kv(Tuple&& tuple, Func&& func) {
    constexpr auto size = std::tuple_size<std::decay_t<Tuple>>::value;
    for_each_impl_kv(std::forward<Tuple>(tuple), std::forward<Func>(func), std::make_index_sequence<size>{});
}
//

using namespace std;


/**
 * Key Value pair serializer
 * 
 */
struct KVSerializer {



    // SERIALIZE FUNCTIONS

    template <typename T>
    static std::enable_if_t<is_primitive_kv<T>::value, void>
    serialize(stringstream& ss, const T& value, int depth = 0) {
        ss << value;
    }

    template <typename T>
    static std::enable_if_t<is_string_kv<T>::value, void>
    serialize(stringstream& ss, const T& value, int depth = 0) {
        string string_value = value;

        // make sure string doesn't start with brackets or parenthesis
        if (string_value[0] == '[' || string_value[0] == '{') {
            // throw std::runtime_error("String values can't start with '[' or '{' characters as this confuses the serializer");
        }
            
        stringstream escaped;
        for(int i = 0; i < string_value.length(); i++) {
            // replace newline characters with escaped \\n
            if (string_value[i] == '\n') {
                string s = "\\n";
                escaped << s;
            }
            // replace # characters with escaped \\#
            else if (string_value[i] == '#') {
                string s = "\\#";
                escaped << s;
            }
            else {
                escaped << string_value[i];
            }
        }

        ss << escaped.str();
    }

    template <typename T>
    static std::enable_if_t<is_vector_kv<T>::value, void>
    serialize(stringstream& ss, const T& value, int depth = 0) {
        serialize_list(ss, value, depth);
    }

    template <typename T>
    static std::enable_if_t<is_std_array_kv<T>::value, void>
    serialize(stringstream& ss, const T& value, int depth = 0) {
        serialize_list(ss, value, depth);
    }

    template <typename T>
    static std::enable_if_t<is_set_kv<T>::value, void>
    serialize(stringstream& ss, const T& value, int depth = 0) {
        serialize_list(ss, value, depth);
    }

    template <typename T>
    static void serialize_list(stringstream& ss, const T& value, int depth = 0) {

        int size = value.size();
        ss << "[\n";
        for (const auto& elem : value) {
            add_white_space((depth) * 4, ss);
            serialize(ss, elem, depth + 1);
            ss << endl;
        }
        add_white_space((depth - 1) * 4, ss);
        ss << "]";

    }

    template <typename T>
    static std::enable_if_t<is_shared_ptr_kv<T>::value, void>
    serialize(stringstream& ss, const T& value, int depth = 0) {
        if (value) {
            serialize(ss, *value, depth);
        }
    }

    template <typename T>
    static std::enable_if_t<is_struct_kv<T>::value, void>
    serialize(stringstream& ss, const T& value, int depth = 0) {

        if (depth != 0) {
            ss << "{\n";
        }

        for_each_kv(T::fields(), [&ss, &depth, &value](auto field) {
            using FieldType = typename named_field_type<T, decltype(field.second)>::type;
            KVSerializer::add_white_space(depth * 4, ss);
            ss << field.first << "=";
            serialize(ss, value.*field.second, depth + 1);
            ss << "\n";
        });

        if (depth != 0) {
            add_white_space((depth - 1) * 4, ss);
            ss << "}";
        }
    }

    template <typename T>
    static std::enable_if_t<is_map_kv<T>::value, void>
    serialize(stringstream& ss, const T& value, int depth = 0) {

        if (depth != 0) {
            ss << "{\n";
        }

        for (const auto& [key, val] : value) {
            add_white_space((depth) * 4, ss);
            ss << key << "=";
            serialize(ss, val, depth + 1);
            ss << "\n";
        }

        if (depth != 0) {
            add_white_space((depth - 1) * 4, ss);
            ss << "}";
        }

        
    }


    static void add_white_space(int amount, stringstream& ss) {
        for (int i = 0; i < amount; ++i) {
            ss << " ";
        }
    }


    // DESERIALIZE FUNCTIONS

    template <typename T>
    static std::enable_if_t<is_decimal<T>::value, void>
    deserialize(const string& message, T& value) {
        double d_val = std::stod(message);
        value = d_val;
    }

    template <typename T>
    static std::enable_if_t<is_int<T>::value, void>
    deserialize(const string& message, T& value) {
        long l_val = 0;
        if (message != "") {
            l_val = std::stol(message);
        }
        value = l_val;
    }

    template <typename T>
    static std::enable_if_t<is_string_kv<T>::value, void>
    deserialize(const string& message, T& value) {
        
        stringstream escaped;
        for(int i = 0; i < message.length(); i++) {
            // replace \\n escaped characters with newlines
            if (i < message.length() - 1 && message[i] == '\\' && message[i+1] == 'n') {
                escaped << endl;
                ++i;
            }
            // replace \# escaped characters with #
            else if (i < message.length() - 1 && message[i] == '\\' && message[i+1] == '#') {
                escaped << '#';
                ++i;
            }
            else {
                escaped << message[i];
            }
        }

        value = escaped.str();
    }

    template <typename T>
    static std::enable_if_t<is_vector_kv<T>::value, void>
    deserialize(const string& message, T& value) {

        value.clear();
        vector<string> string_values = KVSerializer::parse_list(message);
        for (string value_str : string_values) {
            typename T::value_type elem;
            deserialize<typename T::value_type>(value_str, elem);
            value.push_back(elem);
        }

    }

    template <typename T>
    static std::enable_if_t<is_std_array_kv<T>::value, void>
    deserialize(const string& message, T& value) {

        vector<string> string_values = KVSerializer::parse_list(message);
        for (int j = 0; j < string_values.size(); ++j) {
            string value_str = string_values[j];
            typename T::value_type elem;
            deserialize<typename T::value_type>(value_str, elem);
            value[j] = elem;
        }
        
    }

    template <typename T>
    static std::enable_if_t<is_set_kv<T>::value, void>
    deserialize(const string& message, T& value) {

        value.clear();
        vector<string> string_values = KVSerializer::parse_list(message);
        for (string value_str : string_values) {
            typename T::value_type elem;
            deserialize<typename T::value_type>(value_str, elem);
            value.insert(elem);
        }
        
    }

    static vector<string> parse_list(const string& message) {

        // remove indents and collect lines
        vector<string> lines;
        int start = 0;
        for (int i = 0; i < message.length(); ++i) {
            if (message[i] == '\n') {
                lines.push_back(
                    message.substr(start, i - start)
                );
                ++i;
                while(i < message.length() && message[i] == ' ') ++i;
                start = i;
            }
        }

        // get each value or object
        vector<string> values;
        for (int i = 0; i < lines.size(); ++i)  {

            string line = lines[i];

            // objects and sub lists
            if (line[0] == '[' || line[0] == '{') {
                string value = get_sub_object_or_list(lines, i, 0);
                values.push_back(value);
            }
            // normal values
            else {
                values.push_back(line);
            }
        }

        return values;
    }

    template <typename T>
    static std::enable_if_t<is_shared_ptr_kv<T>::value, void>
    deserialize(const string& message, T& value) {
        if (!value) {
            value = std::make_shared<typename T::element_type>();
        }
        deserialize(message, *value);
    }

    template <typename T>
    static std::enable_if_t<is_struct_kv<T>::value, void>
    deserialize(const string& message, T& value) {
        // remove comments
        stringstream ss;
        for (int i = 0; i < message.length(); ++i) {
            if (i < message.length() - 1 && message[i] != '\\' && message[i+1] == '#') {
                while(i < message.length() && message[i] != '\n') ++i;
                if (i < message.length() && message[i] == '\n') ss << message[i];
            }
            else {
                ss << message[i];
            }
        }
        string no_comments_message = ss.str();

        unordered_map<string, string> map = KVSerializer::parse_map(no_comments_message);
        for_each_kv(T::fields(), [&map, &value](auto field) {
            using FieldType = typename named_field_type<T, decltype(field.second)>::type;
            string m_value = map[field.first];
            deserialize(m_value, value.*field.second);
        });
    }

    template <typename T>
    static std::enable_if_t<is_map_kv<T>::value, void>
    deserialize(const string& message, T& value) {
        
        unordered_map<string, string> map = KVSerializer::parse_map(message);
        for (pair<const string, string>& pair : map) {
            typename T::key_type key;
            typename T::mapped_type val;

            deserialize(pair.first, key);
            deserialize(pair.second, val);

            value[key] = val;
        }
    }


    static unordered_map<string, string> parse_map(const string& message) {

        // remove indents and collect lines
        vector<string> lines;
        int start = 0;
        int i = 0;
        while (i < message.length()) {
            if (message[i] == '\n') {
                string line = message.substr(start, i - start);
                bool is_only_whitespace = true;
                for (int j = 0; j < line.length(); ++j) {
                    if (line[j] != ' ' && line[j] != '\n' && line[j] != '\t') {
                        is_only_whitespace = false;
                        break;
                    }
                }
                if (!is_only_whitespace) {
                    lines.push_back(
                        line
                    );
                }
                ++i;
                while(i < message.length() && (message[i] == ' ' || message[i] == '\t')) ++i;
                start = i;
            }
            else {
                ++i;
            }
        }

        // add remaining content after last newline (if any)
        if (start < message.length()) {
            string line = message.substr(start);
            bool is_only_whitespace = true;
            for (int j = 0; j < line.length(); ++j) {
                if (line[j] != ' ' && line[j] != '\n' && line[j] != '\t') {
                    is_only_whitespace = false;
                    break;
                }
            }
            if (!is_only_whitespace) {
                lines.push_back(line);
            }
        }

        
        // split message into key value pairs
        unordered_map<string, string> map;
        for (int i = 0; i < lines.size(); ++i) {
            string line = lines[i];

            int j = 0;

            // collecting name
            while(j < line.length() && line[j] != '=') ++j;
            string name = line.substr(0, j);
            if (j >= line.length()) {
                // no '=' found — skip this line
                ++i;
                continue;
            }
            ++j;

            // collecting value
            string value;
            if (j < line.length() && (line[j] == '[' || line[j] == '{')) {
                value = get_sub_object_or_list(lines, i, j);
            }
            else {
                value = line.substr(j);
            }
            map[name] = value;
        }

        return map;
    }

    static string get_sub_object_or_list(vector<string>& lines, int& line_index, int index_on_line) {
        string line = lines[line_index];

        char open = line[index_on_line];
        char close = line[index_on_line] == '[' ? ']' : '}';

        stringstream list;
        ++line_index;
        int extra_opens = 0;
        while(line_index < lines.size()) {
            line = lines[line_index];
            if (line.empty()) break;
            if (line[0] == close && extra_opens == 0) break;
            if (contains(line, open)) ++extra_opens;
            if (contains(line, close)) --extra_opens;

            list << line << endl;
            ++line_index;
        }

        return list.str();
    }

    static bool contains(string& str, char c) {
        for (int i = 0; i < str.length(); ++i) {
            if (str[i] == c) return true;
        }
        return false;
    }


    static string read_file(const char* file_path) {
        ifstream inputFile(file_path);

        ostringstream ss;
        ss << inputFile.rdbuf();

        inputFile.close();

        return ss.str();
    }

};