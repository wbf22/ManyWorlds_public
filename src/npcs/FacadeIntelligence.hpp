#pragma once

#include <string>
#include "Intelligence.h"

#include <unordered_map>
#include <unordered_set>
#include <functional>
#include "util/Random.h"
#include "util/Util.hpp"
#include "util/CryptoRandom.hpp"
#include <iostream>
#include "serialization/KVSerializer.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>



using namespace std;



struct AttributeMod {
    string attribute;
    float amount; // to add

    DECLARE_NAMED_FIELDS(
        FIELD("attribute", &AttributeMod::attribute),
        FIELD("amount", &AttributeMod::amount)
    );
};

struct State {
    string uuid = CryptoRandom::uuid();
    Relationship relationship;
    Intelligence intelligence;
    vector<string> history;

    float diff(shared_ptr<State>& other_state) {
        float diff_relationship = this->relationship.diff(other_state->relationship);
        float diff_intelligence = this->intelligence.diff(other_state->intelligence);
        
        float diff_history = 0;
        int num_histories = min(this->history.size(), other_state->history.size());
        if (num_histories > 6) num_histories = 6;
        int history_size = this->history.size();
        int other_history_size = other_state->history.size();
        for (int i = 0; i < num_histories; i++) {
            if (this->history[history_size - i] != other_state->history[other_history_size - i]) {
                diff_history += 10.0;
            }
        }
        diff_history /= num_histories;

        
        return diff_relationship + diff_intelligence + diff_history;
    }

    DECLARE_NAMED_FIELDS(
        FIELD("uuid", &State::uuid),
        FIELD("relationship", &State::relationship),
        FIELD("intelligence", &State::intelligence),
        FIELD("history", &State::history)
    );
};

struct Response {
    string value;
    vector<string> options;
    unordered_map<string, unordered_map<string, vector<AttributeMod>>> options_to_state_uuid_to_attribute_mod;
    unordered_map<string, unordered_map<string, string>> options_to_state_uuid_response;


    DECLARE_NAMED_FIELDS(
        FIELD("value", &Response::value),
        FIELD("options", &Response::options),
        FIELD("options_to_state_uuid_to_attribute_mod", &Response::options_to_state_uuid_to_attribute_mod),
        FIELD("options_to_state_uuid_response", &Response::options_to_state_uuid_response)
    );
};  

/**
 * Basically the brain of an npc. 
 * 
 * Handles player npc interactions including performing
 * conversations, as well as managaing the relationship
 * between the player and the npc. 
 * 
 * Also handles other npc available actions.
 */
struct FacadeIntelligence : public Intelligence {

    inline static string TRAINING_FOLDER = "Source/ManyWorlds/entities/social/training";
    inline static string ANSII_RESET = "\033[0m";
    inline static string INTELLI_1_COLOR = "\033[38;2;218;239;179m"; // rgb(218, 239, 179)
    inline static string INTELLI_2_COLOR = "\033[38;2;214;69;80m"; // rgb(214, 69, 80)
    inline static string OPTION_COLOR = "\033[38;2;206;183;4m"; // rgb(206, 183, 4)
    inline static string SECTION_COLOR = "\033[38;2;28;63;153m"; // rgb(28, 63, 153)
    inline static string RESPONSE_COLOR = "\033[38;2;187;203;0m"; // rgb(187, 238, 172)

    // actions
    inline static string END = "END"; // for ending conversation
    inline static string NONE = "NONE";
    
    inline static string FIGHT = "FIGHT"; // ends conversation
    inline static string RUN = "RUN"; // ends conversation

    inline static string LEAD = "LEAD";
    inline static string FOLLOW = "FOLLOW";

    inline static string KISS = "KISS";
    inline static string HUG = "HUG";

    inline static string MANAGE_COMPANY = "MANAGE_COMPANY";

    inline static string GIVE = "GIVE";
    inline static string GO_TO = "GO_TO";
    inline static string TRADE = "TRADE";
    inline static string MAKE = "MAKE"; 
    inline static string EAT = "EAT"; 

    inline static string FART = "FART";
    inline static string SCREAM = "SCREAM";

    // insert keys ( used for replacing with in game data )
    inline static string FREQUENTED_PLACE = "<FREQUENTED_PLACE>";
    inline static string RECENT_ACTIVITY = "<RECENT_ACTIVITY>";
    inline static string OTHER_NPC_NAME = "<OTHER_NPC_NAME>";
    inline static string NPC_NAME = "<NPC_NAME>";
    inline static string INVENTORY_ITEM = "<INVENTORY_ITEM>";
    inline static string PRICE = "<PRICE>";
    inline static string EVIL_PRICE = "<EVIL_PRICE>";
    inline static string PET_ANIMAL = "<PET_ANIMAL>";
    inline static string WORK_CONTACT = "<WORK_CONTACT>";
    inline static string ENEMY_WORK_CONTACT = "<ENEMY_WORK_CONTACT>";
    inline static string VALUABLE_ITEM = "<VALUABLE_ITEM>";
    inline static string POLITICAL_GOAL = "<POLITICAL_GOAL>";
    inline static string FUN_PLACE = "<FUN_PLACE>";
    inline static string SECRET_CRIME_PLACE = "<SECRET_CRIME_PLACE>";
    inline static string TECH = "<TECH>";
    inline static string FOOD = "<FOOD>";
    inline static string BOMB = "<BOMB>";
    inline static string MONEY = "<MONEY>";
    inline static string ART = "<ART>";



    /*
        POSSIBLE ENHANCEMENTS
        - setting or context
            + work, fight, home, market
            + civilization
            + current occupation
            + planet specifics
        - refer to other people
            + recent encounters
            + talk about generally
    
    */


    shared_ptr<Response> respond(const shared_ptr<Response>& last_response, shared_ptr<State>& state) {
        
        // find closest match
        vector<shared_ptr<State>> states;
        string option;
        string closest_state_uuid;
        float smallest_diff = std::numeric_limits<float>().max();
        for (const pair<string, unordered_map<string, string>>& option_with_state_response : last_response->options_to_state_uuid_response) {
            string possible_option = option_with_state_response.first;
            for(const pair<string, string> state_and_response : option_with_state_response.second) {
                string state_uuid = state_and_response.first;
                string possible_response = state_and_response.second;
                shared_ptr<State> other_state = load_state(state_uuid);
                float diff = state->diff(other_state);
                if (diff < smallest_diff) {
                    closest_state_uuid = other_state->uuid;
                    option = possible_response;
                }
            }
        }


        // apply attribute mods
        vector<AttributeMod> mods = last_response->options_to_state_uuid_to_attribute_mod[option][closest_state_uuid];
        cout << SECTION_COLOR << "ATTRIBUTE MODS" << ANSII_RESET << endl << endl;
        for (AttributeMod& mod : mods) {
            cout << mod.attribute << " " << mod.amount << endl;

            if (mod.attribute == "anger_mood") {
                state->intelligence.anger_mood = clamp_0_10(state->intelligence.anger_mood + mod.amount);
            }
            else if (mod.attribute == "anxiety_mood") {
                state->intelligence.anxiety_mood = clamp_0_10(state->intelligence.anxiety_mood + mod.amount);
            }
            else if (mod.attribute == "happiness_mood") {
                state->intelligence.happiness_mood = clamp_0_10(state->intelligence.happiness_mood + mod.amount);
            }
            else if (mod.attribute == "friendship") {
                state->relationship.friendship = clamp_0_10(state->relationship.friendship + mod.amount);
            }
            else if (mod.attribute == "romance") {
                state->relationship.romance = clamp_0_10(state->relationship.romance + mod.amount);
            }
            else if (mod.attribute == "trust") {
                state->relationship.trust = clamp_0_10(state->relationship.trust + mod.amount);
            }
        }
        cout << endl;
        save_state(state);


        // load response for match
        return load_response(option);
    }


    static shared_ptr<Response> load_response(string response_value) {
        // KVSerializer

        ifstream file(FacadeIntelligence::TRAINING_FOLDER + "/responses" + "/" + response_value + ".kv");
        if (file) {
            stringstream buffer;
            buffer << file.rdbuf();
            string data_str = buffer.str();

            shared_ptr<Response> response = std::make_shared<Response>(); 
            KVSerializer::deserialize(data_str, *response);

            return response;
        }
        else {
            return nullptr;
        }
        
    }


    static void save_response(shared_ptr<Response>& response) {
        stringstream data_str;
        KVSerializer::serialize(data_str, *response);

        ofstream file(FacadeIntelligence::TRAINING_FOLDER + "/responses" + "/" + response->value + ".kv");
        if (file) {
            file << data_str.str();
            file.close();
        }
        else {
            cout << "can't open file" << endl;
        }
    }


    static shared_ptr<State> load_state(string uuid) {
        // KVSerializer

        ifstream file(FacadeIntelligence::TRAINING_FOLDER + "/states" + "/" + uuid + ".kv");
        if (file) {
            stringstream buffer;
            buffer << file.rdbuf();
            string data_str = buffer.str();

            shared_ptr<State> state = std::make_shared<State>(); 
            KVSerializer::deserialize(data_str, *state);

            return state;
        }
        else {
            return nullptr;
        }
        
    }


    static void save_state(shared_ptr<State>& state) {
        stringstream data_str;
        KVSerializer::serialize(data_str, *state);

        ofstream file(FacadeIntelligence::TRAINING_FOLDER + "/states" + "/" + state->uuid + ".kv");
        if (file) {
            file << data_str.str();
            file.close();
        }
        else {
            cout << "can't open file" << endl;
        }
    }


    static float clamp_0_10(float value) {
        return clamp(value, 0.001, 10);
    }

    static float clamp(float value, float low, float high) {
        if (value > high ) {
            return high;
        }
        else if (value < low) {
            return low;
        }
        
        return value;
    }


    static void train() { 

        // make intial intelligences and relationships
        shared_ptr<FacadeIntelligence> intelligence_1 = std::make_shared<FacadeIntelligence>();
        intelligence_1->uuid = CryptoRandom::uuid();
        intelligence_1->randomize();
        shared_ptr<Relationship> relationship_1 = std::make_shared<Relationship>();
        relationship_1->randomize();

        shared_ptr<FacadeIntelligence> intelligence_2 = std::make_shared<FacadeIntelligence>();
        intelligence_2->uuid = CryptoRandom::uuid();
        intelligence_2->randomize();
        shared_ptr<Relationship> relationship_2 = std::make_shared<Relationship>();
        relationship_2->randomize();

        Intelligence::reconcile_relationships(intelligence_1, intelligence_2, relationship_1, relationship_2);

        cout << endl << SECTION_COLOR << "SUMMARIES" << ANSII_RESET << endl << endl;
        cout << intelligence_1->uuid << endl << "- " << intelligence_1->summary() << relationship_1->summary() << endl << endl;
        cout << intelligence_2->uuid << endl << "- " << intelligence_2->summary() << relationship_2->summary() << endl << endl << endl;




        // loop training
        shared_ptr<FacadeIntelligence> current = intelligence_1;
        shared_ptr<FacadeIntelligence> other = intelligence_2;

        vector<string> files = get_all_response_values(TRAINING_FOLDER + "/responses");
        shared_ptr<Response> response = load_response(files[(int)random(0, files.size() - 1)]);
        vector<string> history;
        while(true) {
            history.push_back(response->value);

            // display response
            cout << INTELLI_1_COLOR << "RESPONSE" << ANSII_RESET << endl << endl;
            for (string line : history) {
                cout << RESPONSE_COLOR << line << ANSII_RESET << endl;  
            }
            cout << endl << endl << endl;

            // display state
            cout << SECTION_COLOR << "STATE" << ANSII_RESET << endl << endl;
            cout << current->to_string() << endl;
            cout << current->summary() << endl << endl;
            cout << current->relationships[other->uuid]->to_string() << endl;
            cout << current->relationships[other->uuid]->summary() << endl << endl;

            // modify state
            bool new_state = true;
            shared_ptr<Relationship> current_relationship = current->relationships[other->uuid];
            current_relationship->tick();
            
            // continue if making new response and state
            if (new_state) {

                // save state for before response
                shared_ptr<State> state = std::make_shared<State>();
                state->relationship = *current_relationship;
                state->intelligence = *current;
                state->history = history;
                save_state(state);
                cout << "created state " << state->uuid << endl;


                // determine response
                cout << endl << SECTION_COLOR << "SELECT OPTION" << ANSII_RESET << endl << endl;
                vector<pair<string, string>> options;
                for(int i = 0; i < response->options.size(); i++) {
                    string option = response->options[i];
                    options.push_back(
                        {std::to_string(i), option}
                    );
                }
                options.push_back(
                    {".", "new option"}
                );
                pretty_print_options(options);
                cout << endl;

                string option;
                string option_str = input();
                if (option_str == ".") {
                    cout << "New option: ";
                    option = input();
                    response->options.push_back(option);

                    // save new response
                    shared_ptr<Response> new_response = load_response(option);
                    if (new_response == nullptr) {
                        new_response = std::make_shared<Response>();
                        new_response->value = option;
                        save_response(new_response);
                    }
                }
                else {
                    int option_index = stoi(option_str);
                    option = response->options[option_index];
                }

                // modify state
                shared_ptr<Relationship> current_relationship = current->relationships[other->uuid];
                current_relationship->tick();
                vector<AttributeMod> attribute_mods;
                cout << endl << SECTION_COLOR << "STATE CHANGES" << ANSII_RESET << endl << endl;
                bool modifying = true;
                while(modifying) {
                    pretty_print_options(
                        {
                            {"0", "anger_mood"},
                            {"1", "anxiety_mood"},
                            {"2", "happiness_mood"},

                            {"3", "friendship"},
                            {"4", "romance"},
                            {"5", "trust"},

                            {".", "create new state and response"},
                        }
                    );
                    cout << endl;

                    string option = input();
                    if (option == " ") {
                        modifying = false;
                    }
                    else if (option == ".") {
                        modifying = false;
                    }
                    else {
                        int attribute_int = std::stoi(option);
                        cout << "Enter amount to add: ";
                        string amount_str = input();
                        float amount = std::stof(amount_str);

                        AttributeMod attribute_mod;
                        attribute_mod.amount = amount;
                        switch(attribute_int) {
                            case 0:
                                current->anger_mood = clamp_0_10(current->anger_mood + amount); 
                                attribute_mod.attribute = "anger_mood";
                                break;
                            case 1:
                                current->anxiety_mood = clamp_0_10(current->anxiety_mood + amount); 
                                attribute_mod.attribute = "anxiety_mood";
                                break;
                            case 2:
                                current->happiness_mood = clamp_0_10(current->happiness_mood + amount); 
                                attribute_mod.attribute = "happiness_mood";
                                break;
                            case 3:
                                current_relationship->friendship = clamp_0_10(current_relationship->friendship + amount); 
                                attribute_mod.attribute = "friendship";
                                break;
                            case 4:
                                current_relationship->romance = clamp_0_10(current_relationship->romance + amount); 
                                attribute_mod.attribute = "romance";
                                break;
                            case 5:
                                current_relationship->trust = clamp_0_10(current_relationship->trust + amount); 
                                attribute_mod.attribute = "trust";
                                break;
                            default: return; // abort
                        }
                        attribute_mods.push_back(attribute_mod);

                        cout << endl;

                    }
                }


                // save response with state
                response->options_to_state_uuid_to_attribute_mod[option][state->uuid] = attribute_mods;
                response->options_to_state_uuid_response[option][state->uuid] = option;
                save_response(response);
                cout << "saved response '" << response->value << "'" << endl;

                // set goal asked for if applicable
                cout << endl << "Goal? ( y/n ):";
                string input_str = input();
                if (input_str == "y") {
                    cout << endl << "Change goal? ( y/n ):";
                    input_str = input();
                    if (input_str == "y") {

                        // evaluate goals
                        cout << endl << SECTION_COLOR << "MODIFY GOAL" << ANSII_RESET << endl << endl;
                        pretty_print_options(
                            {
                                {"0", "ROMANCE"},
                                {"1", "FRIEND"},
                                {"2", "FUN"},
                                {"3", "BUSINESS_PARTNER"},
                                {"4", "TRADE"},
                                {"5", "MAIME_OR_KILL"},
                                {"6", "STAY_WITH"},
                                {"7", "GET_AWAY"},
                                {"8", "ASK_TO_STAY"},
                                {"9", "ROB"},
                                {"10", "CONTROL"},
                                {"11", "NONE"},

                                {"SPACE", "skip"},
                            }
                        );
                        cout << endl;

                        string goal_str = input();
                        if (goal_str != " ") {
                            int goal_int = stoi(goal_str);
                            switch (goal_int) {
                                case 0:
                                    current_relationship->goal = Goal::ROMANCE;
                                    break;
                                case 1:
                                    current_relationship->goal = Goal::FRIEND;
                                    break;
                                case 2:
                                    current_relationship->goal = Goal::FUN;
                                    break;
                                case 3:
                                    current_relationship->goal = Goal::BUSINESS_PARTNER;
                                    break;
                                case 4:
                                    current_relationship->goal = Goal::TRADE;
                                    break;
                                case 5:
                                    current_relationship->goal = Goal::MAIME_OR_KILL;
                                    break;
                                case 6:
                                    current_relationship->goal = Goal::STAY_WITH;
                                    break;
                                case 7:
                                    current_relationship->goal = Goal::GET_AWAY;
                                    break;
                                case 8:
                                    current_relationship->goal = Goal::ASK_TO_STAY;
                                    break;
                                case 9:
                                    current_relationship->goal = Goal::ROB;
                                    break;
                                case 10:
                                    current_relationship->goal = Goal::CONTROL;
                                    break;
                                default: current_relationship->goal = Goal::NONE;
                            }
                        }

                    }

                    cout << endl << endl << "Asking for response on current goal ( y/n ):";
                    input_str = input();
                    if (input_str == "y") {
                        current_relationship->goal_asked_for = true;
                    }
                    else {
                        current_relationship->goal_asked_for = false;
                    }


                    cout << endl << "Set status? ( y/n ):";
                    input_str = input();
                    if (input_str == "y") {

                        // evaluate goals
                        cout << endl << SECTION_COLOR << "SET STATUS" << ANSII_RESET << endl << endl;
                        pretty_print_options(
                            {
                                {"0", Relationship::Status::MARRIED},
                                {"1", Relationship::Status::ENGAGED},
                                {"2", Relationship::Status::SIBLING},
                                {"3", Relationship::Status::PARENT},
                                {"4", Relationship::Status::CHILD},
                                {"5", Relationship::Status::ENEMY},
                                {"6", Relationship::Status::FRIEND},
                                {"7", Relationship::Status::BUSINESS_PARTNER},
                                {"8", Relationship::Status::OTHER},
                            }
                        );
                        cout << endl;

                        string goal_str = input();
                        if (goal_str != " ") {
                            int goal_int = stoi(goal_str);
                            switch (goal_int) {
                                case 0:
                                    current_relationship->status = Relationship::Status::MARRIED;
                                    other->relationships[current->uuid]->status = Relationship::Status::MARRIED;
                                    break;
                                case 1:
                                    current_relationship->status = Relationship::Status::ENGAGED;
                                    other->relationships[current->uuid]->status = Relationship::Status::ENGAGED;
                                    break;
                                case 2:
                                    current_relationship->status = Relationship::Status::SIBLING;
                                    other->relationships[current->uuid]->status = Relationship::Status::SIBLING;
                                    break;
                                case 3:
                                    current_relationship->status = Relationship::Status::PARENT;
                                    other->relationships[current->uuid]->status = Relationship::Status::PARENT;
                                    break;
                                case 4:
                                    current_relationship->status = Relationship::Status::CHILD;
                                    other->relationships[current->uuid]->status = Relationship::Status::CHILD;
                                    break;
                                case 5:
                                    current_relationship->status = Relationship::Status::ENEMY;
                                    other->relationships[current->uuid]->status = Relationship::Status::ENEMY;
                                    break;
                                case 6:
                                    current_relationship->status = Relationship::Status::FRIEND;
                                    other->relationships[current->uuid]->status = Relationship::Status::FRIEND;
                                    break;
                                case 7:
                                    current_relationship->status = Relationship::Status::BUSINESS_PARTNER;
                                    other->relationships[current->uuid]->status = Relationship::Status::BUSINESS_PARTNER;
                                    break;
                                case 8:
                                    current_relationship->status = Relationship::Status::OTHER;
                                    other->relationships[current->uuid]->status = Relationship::Status::OTHER;
                                    break;
                                default:;
                            }
                        }

                    }

                }
                cout << endl << endl;

                // set next response
                response = load_response(option);
            }
            // otherwise use existing response
            else {
                shared_ptr<State> state = std::make_shared<State>();
                state->relationship = *current_relationship;
                state->intelligence = *current;
                state->history = history;
                response = current->respond(response, state);
            } 
            

            // switch to other intelligence
            if (current->uuid == intelligence_1->uuid) {
                current = intelligence_2;
                other = intelligence_1;
            }
            else {
                current = intelligence_1;
                other = intelligence_2;
            }

        }

    }


    static void pretty_print_options(vector<pair<string, string>> options) {

        for(pair<string, string>& option : options) {
            cout << OPTION_COLOR << option.first << " -> " << ANSII_RESET;
            cout << option.second << endl;
        }
    }

    static string input() {
        stringstream ss;
        char ch;
        while (std::cin.get(ch)) {
            if (ch == '\n') break;
            ss << ch;
        }

        return ss.str();
    }


    static vector<string> get_all_response_values(const string& directory_path) {
        vector<string> files;
        for (const auto& entry : filesystem::directory_iterator(directory_path)) {
            string name = entry.path().filename();
            name = name.substr(0, name.length()-3);
            files.push_back(name);
        }
        return files;
    }
};