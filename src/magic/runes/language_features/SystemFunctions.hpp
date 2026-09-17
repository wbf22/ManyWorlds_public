#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <iostream>


static void print(string& str, stringstream& output, bool new_line = true) {
    
    // XXX: display text in game later
    

    output << str;
    cout << str;
    if (new_line) {
        output << endl;
        cout << endl;
    }
}


