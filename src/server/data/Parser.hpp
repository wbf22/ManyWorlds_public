#pragma once



#include <unordered_map>
#include <vector>
#include <iostream>
#include <fstream>


using namespace std;


struct Parser {

    /**
     * Parses a file into a map of key-value pairs
     */
    static unordered_map<string, string> parse_map(const string& file_path) {

        unordered_map<string, string> map;
        ifstream file(file_path);
        string line;
        while (getline(file, line)) {
            size_t pos = line.find("=");
            if (pos == string::npos) {
                continue;
            }
            string key = line.substr(0, pos);
            string value = line.substr(pos + 1);
            map[key] = value;
        }

        return map;
    }

    /**
     * Parses a file into a list of strings
     */
    static vector<string> parse_list(const string& file_path) {

        vector<string> list;
        ifstream file(file_path);
        string line;
        while (getline(file, line)) {
            if (!_only_whitespace(line)) 
                list.push_back(line);
        }

        return list;
    }

    static bool _only_whitespace(const string& str) {
        for (char c : str) {
            if (!isspace(c)) {
                return false;
            }
        }
        return true;
    }


    /**
     * Writes list of pairs to a file
     */
    static void write(const vector<pair<string, string>>& pairs, const string& file_path) {
        ofstream file(file_path);
        for (auto& pair : pairs) {
            if (pair.first == "") 
                file << endl;
            else
                file << pair.first << "=" << pair.second << endl;
        }
    }

    /**
     * Writes list of strings to a file
     */
    static void write(const vector<string>& list, const string& file_path) {
        ofstream file(file_path);
        for (auto& str : list) {
            file << str << endl;
        }
    }

    
};
