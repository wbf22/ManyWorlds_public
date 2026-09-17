#pragma once


#include "CoreMinimal.h"
#include <unordered_map>
#include <fstream>
#include <filesystem>
#include <any>

using namespace std;





struct MapSerializer {

    static void writeToFile(string file_name, vector<unordered_map<string, any>>& maps) {
        ofstream out(file_name);
    
        if (out.is_open()) {
            // filesystem::path currentPath = std::filesystem::current_path();
            // string dir = currentPath.c_str();
    
            /*
                when debugging locally this will be under:
                - Packaged-Linux/Linux/ManyWorlds/Binaries/Linux
                - Binaries/Linux
    
                Or wherever the game executable is located
    
            */
            for (unordered_map<string, any>>& map : maps) {
                for (pair<string, string>& element : map) {
                    out << element.first << " " << element.second << endl;
                }
                out << endl;
            }
    
            // Close the file 
            out.close();
        }
        else {
            cerr << "Couldn't open file for saving bodypart: " << file_name << endl;
        }
    }
    
    static vector<unordered_map<string, any>> loadFromFile(string file_name) {
        //std::cout << std::any_cast<int>(pair.second);
        
        ifstream inFile(file_name);
    
    
        string line;
    
        // Read the file line by line
        vector<BodyBlock> blocks;
        while (getline(inFile, line)) {
            cout << line << endl;
        }
    
        // Close the file
        inFile.close();
    
        return blocks;
    }
};






