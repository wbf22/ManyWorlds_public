#pragma once

#include <string>
#include "Intelligence.h"

#include <unordered_map>
#include <unordered_set>
#include <functional>
#include "util/Random.h"
#include "util/Util.hpp"



using namespace std;




using ResponseFunction = function<string(const shared_ptr<Relationship>&, Intelligence*, const vector<string>& history)>;

/**
 * Basically the brain of an npc. 
 * 
 * Handles player npc interactions including performing
 * conversations, as well as managaing the relationship
 * between the player and the npc. 
 * 
 * Also handles other npc available actions.
 */
struct FacadeIntelligenceOld : public Intelligence {



    void respond(string user_id, string user_prompt, string& response, vector<string>& options, vector<string>& history) {
        
        // history.push_back(user_prompt);

        // // get response
        // if (FacadeIntelligenceOld::prompt_resolver.find(user_prompt) != FacadeIntelligenceOld::prompt_resolver.end()) {
        //     auto resolver = FacadeIntelligenceOld::prompt_resolver[user_prompt];
        //     response = resolver(this->relationships[user_id], this, history);
        // }
        // else {
        //     response = WHAT;
        // }
        // history.push_back(response);

        // // get options for player or other npc

        // if (FacadeIntelligenceOld::prompt_to_options.find(response) != FacadeIntelligenceOld::prompt_to_options.end()) {
        //     options = FacadeIntelligenceOld::prompt_to_options[response];
        // }
        // else {
        //     if (FacadeIntelligenceOld::equivalents.find(response) != FacadeIntelligenceOld::equivalents.end()) {
        //         string eq = FacadeIntelligenceOld::equivalents[response];
        //         options = FacadeIntelligenceOld::prompt_to_options[eq];
        //     }
        //     else {
        //         options = FacadeIntelligenceOld::prompt_to_options[DEFAULT];
        //     }
        // }

        // // limit to 6 options
        // if (options.size() > 6) {
        //     unordered_set<string> new_options;
        //     for (int i = 0; i < 6; i++) {
        //         new_options.insert(
        //             options[random(0, options.size())]
        //         );
        //     }
        //     int i = 0;
        //     while(new_options.size() < options.size()) {
        //         new_options.insert(options[i]);
        //         ++i;
        //     }
        //     options.clear();
        //     for (string option : new_options) {
        //         options.push_back(option);
        //     }
        // }

        // ++this->relationships[user_id]->interactions;
    }



    // CONVERSATION STUFF
    inline static int CONVERSATION_SEED = Util::seed();

    // phrases
    inline static string DEFAULT = "DEFAULT";
    inline static string HI = "hi!";
    inline static string YOU_AGAIN = "you again?";
    inline static string HEY = "hey!";
    inline static string HELLO = "hello";
    inline static string HOW_DO_YOU_DO = "how do you do?";
    inline static string I_HATE_YOU = "I hate you";
    inline static string YOU_STINK = "you stink!";
    inline static string I_LOVE_YOU = "I love you";
    inline static string I_LOVE_YOU_TOO = "I love you too";
    inline static string HELLO_MY_FRIEND = "hello my friend";
    inline static string ABOMINATION = "abomination!";
    inline static string MARRY_ME = "marry me?";
    inline static string WE_RE_ALREADY_MARRIED = "we're already married";
    inline static string WHAT = "what?";
    inline static string SORRY_IM_NOT_AVAILABLE = "sorry I'm not available";
    inline static string I_HARDLY_KNOW_YOU = "I hardly know you";
    inline static string YOU_RE_PATHETIC = "you're pathetic!";
    inline static string REALLY = "really?";
    inline static string YOU_RE_PRETTY_CUTE = "you're pretty cute";
    inline static string COOL = "cool";
    inline static string PLEASE_STOP = "please stop!";
    inline static string AREN_T_WE_ENGAGED = "aren't we engaged?";
    inline static string BYE = "bye";
    inline static string WAIT_CAN_I_COME = "wait! can I come?";
    inline static string MAY_I_FOLLOW = "may I follow?";
    inline static string FARE_THEE_WELL = "fare thee well";
    inline static string GODSPEED = "godspeed";
    inline static string SO_LONG = "so long";
    inline static string FINALLY = "finally";
    inline static string GO_AWAY = "go away!!";
    inline static string PLEASE_GO_BACK = "please go back";
    inline static string AYE = "aye";
    inline static string YES = "yes";
    inline static string I_GUESS = "I guess";
    inline static string NO = "No!";
    inline static string AYE_ALLOW = "aye (allow)";
    inline static string YES_ALLOW = "yes (allow)";
    inline static string I_GUESS_ALLOW = "I guess (allow)";
    inline static string NO_DENY = "No! (deny)";
    inline static string PLEASE_PETITION = "please (petition)";
    inline static string SORRY_I_CANT_TOO_BUSY_DENY = "sorry I can't, too busy (deny)";
    inline static string THANKS = "thanks!";
    inline static string I_AM_BEHOLDEN_TO_YOU = "I am beholden to you";
    inline static string YAY = "yay!";
    inline static string OK = "ok";
    inline static string ANY_BARGAINS = "any bargains?";
    inline static string WHATS_YOUR_NAME = "what's your name?";
    inline static string NICE_TO_SEE_YOU = "nice to see you";
    inline static string CARE_TO_JOIN_ME = "care to join me?";
    inline static string CAN_YOU_STAY = "can you stay?";
    inline static string HAVE_A_GIFT_MY_FRIEND = "have a gift my friend";
    inline static string SHAME = "shame";
    
    // actions
    inline static string NONE = "none";
    inline static string FIGHT = "fight";
    inline static string KISS = "kiss";
    inline static string HUG = "hug";
    inline static string RUN = "run";
    inline static string TRADE = "trade";
    inline static string FOLLOW = "follow";
    inline static string FART = "fart";
    inline static string HAHA = "haha";
    


    // static int random(int inclusive, int exclusive) {
    //     ++FacadeIntelligenceOld::CONVERSATION_SEED;
    //     return Random::randInt(FacadeIntelligenceOld::CONVERSATION_SEED, inclusive, exclusive);
    // }

    // static bool random_bool() {
    //     ++FacadeIntelligenceOld::CONVERSATION_SEED;
    //     return Random::randBool(FacadeIntelligenceOld::CONVERSATION_SEED);
    // }

    // static float random_float(float min, float max) {
    //     ++FacadeIntelligenceOld::CONVERSATION_SEED;
    //     return Random::randDouble(FacadeIntelligenceOld::CONVERSATION_SEED, min, max);
    // }

    // static int clamp_0_10(float value) {
    //     return clamp(value, 0.001, 10);
    // }

    // static int clamp(float value, float low, float high) {
    //     if (value > high ) {
    //         return high;
    //     }
    //     else if (value < low) {
    //         return low;
    //     }
        
    //     return value;
    // }

    // inline static unordered_map<string, string> equivalents {
    //     {CAN_YOU_STAY, CARE_TO_JOIN_ME},
    //     {HEY, HI},
    //     {HELLO, HI},
    //     {HOW_DO_YOU_DO, HI},
    //     {HELLO_MY_FRIEND, HI},
    //     {HEY, HI},
    // };

    // inline static unordered_map<string, vector<string>> prompt_to_options {
    //    {    DEFAULT,
    //         {
    //             HI,
    //             HELLO,
    //             HOW_DO_YOU_DO,
    //             I_HATE_YOU,
    //             HELLO_MY_FRIEND,
    //             ABOMINATION,
    //             I_LOVE_YOU,
    //             YOU_AGAIN,
    //             MARRY_ME,
    //             BYE,
    //             ANY_BARGAINS,
    //         }
    //    },
    //    {    HI,
    //         {
    //             // informal
    //             HI,
    //             HEY,
    //             HOW_DO_YOU_DO,
    //             // formal
    //             HELLO,
    //             HELLO_MY_FRIEND,
    //             //romantic
    //             I_LOVE_YOU,
    //             // hate
    //             YOU_AGAIN,
    //             I_HATE_YOU,
    //             YOU_STINK,
    //             // disdain
    //             ABOMINATION,
    //             // quit
    //             BYE
    //         }
    //    },
    //    {    I_LOVE_YOU,
    //         {
    //             // romantic
    //             REALLY,
    //             YOU_RE_PRETTY_CUTE,
    //             I_LOVE_YOU_TOO,
    //             KISS,
    //             HUG,
    //             MARRY_ME,
    //             // freaked out
    //             RUN,
    //             FIGHT,
    //             // confused
    //             WHAT,
    //             COOL,
    //             PLEASE_STOP,
    //             I_HARDLY_KNOW_YOU
    //         }
    //    },
    //    {    MARRY_ME,
    //         {
    //             // romantic
    //             KISS,
    //             HUG,
    //             YES,
    //             // freak out
    //             RUN,
    //             FIGHT,
    //             // confused
    //             WE_RE_ALREADY_MARRIED,
    //             AREN_T_WE_ENGAGED,
    //             WHAT,
    //             PLEASE_STOP,
    //             NO,
    //             I_HARDLY_KNOW_YOU,
    //         }
    //    },
    //    {    BYE,
    //         {
    //             // romantic
    //             KISS,
    //             HUG,
    //             // other
    //             WAIT_CAN_I_COME,
    //             BYE,
    //             FARE_THEE_WELL,
    //             GODSPEED,
    //             SO_LONG,
    //             // agressive
    //             YOU_RE_PATHETIC,
    //             FINALLY,
    //             GO_AWAY,
    //             // other
    //             MAY_I_FOLLOW,
    //         }
    //    },
    //    {    WAIT_CAN_I_COME,
    //         {
    //             // yes
    //             I_GUESS_ALLOW,
    //             AYE_ALLOW,
    //             YES_ALLOW,
    //             // no
    //             NO,
    //         }
    //    },
    //    {    FOLLOW,
    //         {
    //             // expected
    //             NONE,       
    //             // nice
    //             PLEASE_GO_BACK,
    //             // angry
    //             GO_AWAY,
    //             FIGHT,
    //         }
    //    },
    //    {    YES_ALLOW,
    //         {
    //             NONE,     
    //             THANKS,
    //             I_AM_BEHOLDEN_TO_YOU,
    //             YAY,
    //             OK,
    //         }
    //    },
    //    {    CARE_TO_JOIN_ME,
    //         {
    //             NO_DENY,
    //             SORRY_I_CANT_TOO_BUSY_DENY,
    //             I_GUESS_ALLOW,
    //             YES_ALLOW,
    //             AYE_ALLOW,
    //         }
    //    },
    //    {    NO_DENY,
    //         {
    //             OK,
    //             SHAME,
    //         }
    //    },
    // };

    // static string fight_or_flight(Intelligence* me) {
    //     if (me->get_anxiety() > 7) {
    //         me->anxiety_mood++;
    //         return RUN;
    //     }
    //     else if (me->get_anger() > 7 || (me->craziness > 7 && random_bool())) {
    //         if (me->goodness < 7 || me->get_anger() > 9) {
    //             me->anger_mood++;
    //             return FIGHT;
    //         }
    //     }

    //     return "";
    // }

    // static string get_last(const vector<string>& history) {

    //     return history.size() > 1? history[history.size()-2] : "";
    // }

    // static string player_last(const vector<string>& history) {
    //     return history.size() > 0? history[history.size()-1] : "";
    // }

    // inline static ResponseFunction greeting = [](const shared_ptr<Relationship>& relationship, Intelligence* me, const vector<string>& history) -> string {
    //     vector<string> prompt_options = prompt_to_options[HI];

    //     string last = FacadeIntelligenceOld::get_last(history);
    //     if (std::find(prompt_options.begin(), prompt_options.end(), last) == prompt_options.end()) {
    //         if (me->craziness > 9 && random_bool()) {
    //             return prompt_options[random(0, prompt_options.size() - 1)];
    //         }
    //         else if (relationship->romance > 5 && random_bool()) {
    //             me->happiness_mood++;
    //             return prompt_options[5];
    //         }
    //         else if (relationship->friendship > 2) {

    //             int informality = (me->silliness + me->shyness);
    //             informality += me->age > 30? 10 : 0;
    //             informality /= 3;
    //             me->happiness_mood += 0.2;
                

    //             if (informality < 5) {
    //                 return prompt_options[random(3, 5)];
    //             }
    //             else {
    //                 return prompt_options[random(0, 3)];
    //             }
    //         }
    //         else {
    //             if (relationship->friendship < 0.4) {
    //                 if (me->get_anger() > 9) {
    //                     relationship->activated_goal = Goal::MAIME_OR_KILL;
    //                     relationship->attemping_goal = true;
    //                 }
    //                 return prompt_options[9];
    //             }
    //             else {
    //                 return prompt_options[random(6, 9)];
    //             }
    //         }
    //     }
    //     else {
    //         prompt_options = prompt_to_options[DEFAULT];

    //         // choose a goal
    //         Goal goal = me->choose_goal(relationship);
    //         if (goal == Goal::FRIEND) {
    //             relationship->activated_goal = goal;
    //             relationship->attemping_goal = true;
    //             if (relationship->interactions == 5) {
    //                 return WHATS_YOUR_NAME;
    //             }
    //             else {
    //                 vector<string> options = {
    //                     NICE_TO_SEE_YOU,
    //                     CARE_TO_JOIN_ME,
    //                     HAVE_A_GIFT_MY_FRIEND
    //                 };
    //                 string response = options[random(0, options.size()-1)];
    //                 if (response == HAVE_A_GIFT_MY_FRIEND) {
    //                     me->give_gift();
    //                 }
    //                 else if (response == CARE_TO_JOIN_ME) {
    //                     relationship->activated_goal = Goal::ASK_TO_STAY;
    //                     relationship->attemping_goal = true;
    //                 }

    //                 return response;
    //             }
    //         }



            
    //     }


    //     return DEFAULT;
    // };

    // Goal choose_goal(const shared_ptr<Relationship>& relationship) {
    //     Goal goal = Goal::NONE;

    //     // fight
    //     if (goal == Goal::NONE) {
    //         if (this->want_to_fight(relationship) > 8) {
    //             goal = Goal::MAIME_OR_KILL;
    //         }
    //     }


    //     // friendship
    //     if (goal == Goal::NONE) {
    //         float desire_for_friendship = this->want_to_be_friend(relationship);
    //         if (desire_for_friendship > 1 ) {//&& random_bool()) {
    //             if (relationship->interactions == 1) {
    //                 if (this->shyness > 5) {

    //                     bool willing_to_try = random_bool();
    //                     if (get_anxiety() > 10) willing_to_try = willing_to_try || random_bool();
    //                     if (willing_to_try) goal = Goal::FRIEND;
    //                 }
    //                 else {
    //                     goal = Goal::FRIEND;
    //                 }
    //             }
    //             else if (relationship->friendship > 2 && relationship->friendship < 9 && random_bool()) {
    //                     goal = Goal::FRIEND;
    //             }
    //         }
    //     }

    //     // business
    //     if (goal == Goal::NONE) {
    //         if (this->want_business_partner(relationship)) {
    //             goal = Goal::BUSINESS_PARTNER;
    //         }
    //     }


    //     // trade
    //     if (goal == Goal::NONE) {
    //         if (this->want_business()) {
    //             goal = Goal::TRADE;
    //         }
    //     }

    //     // romance
    //     if (goal == Goal::NONE) {
    //         if (random_bool() && this->want_romance(relationship)) {
    //             goal = Goal::ROMANCE;
    //         }
    //     }

    //     // fun
    //     if (goal == Goal::NONE) {
    //         if (random_bool()) {
    //             if (random_bool() || this->silliness > 5) {
    //                 if (this->get_happiness() > 5) {
    //                     goal = Goal::FUN;
    //                 }
    //             }
    //         }
    //     }


    //     return goal;
    // }

    // float want_to_be_friend(const shared_ptr<Relationship>& relationship) {
    //     float desire_for_friendship = relationship->friendship;
    //     desire_for_friendship += this->goodness;
    //     desire_for_friendship += this->anxiety;
    //     desire_for_friendship += this->silliness;
    //     desire_for_friendship -= this->laziness;
    //     desire_for_friendship -= get_anger();
    //     desire_for_friendship /= 6;

    //     return desire_for_friendship;
    // }

    // bool want_business_partner(const shared_ptr<Relationship>& relationship) {
    //     if (this->want_business()) {
    //         // business partner
    //         if (
    //             relationship->status != Relationship::Status::BUSINESS_PARTNER 
    //             && relationship->friendship > 8 
    //             && relationship->trust > 8
    //         ) {
    //             return true;
    //         }
    //     }
    //     return false;
    // }

    // bool want_business() {
    //     if (random(0,10) > 8 && this->laziness < 5 && this->weirdness > 5) {
    //         // confident
    //         if (this->get_anxiety() < 5 && this->age > 20) {
    //             return true;
    //         }
    //     }
    //     return false;
    // }

    // float want_to_fight(const shared_ptr<Relationship>& relationship) {
    //     float others_avg = this->anxiety + this->laziness + this->happiness;
    //     others_avg /= 3;

    //     float bloodlust = (this->get_anger() + others_avg) / 2;
    //     bloodlust /= this->goodness;

    //     return bloodlust;
    // }

    // bool want_romance(const shared_ptr<Relationship>& relationship) {
        
    //     if (relationship->romance < 5) {

    //         bool right_gender = this->gender != relationship->others_gender || this->gender == Gender::IN_BETWEEN;
    //         bool willingness = relationship->friendship > 6 || this->craziness > 6;
        
    //         // romance start
    //         if (right_gender && willingness && random_bool()) {
    //             relationship->friendship = clamp_0_10(++relationship->friendship);
    //             relationship->romance = clamp_0_10(++relationship->romance);
    //             this->happiness_mood++;
        
    //             return true;
    //         }
    //         // rejection
    //         else {
    //             return false;
    //         }
    //     }
    //     else {
    //         return true;
    //     }
    // }

    // inline static ResponseFunction parting = [](const shared_ptr<Relationship>& relationship, Intelligence* me, const vector<string>& history) -> string {
    //     vector<string> prompt_options = prompt_to_options[BYE];
    //     if (me->craziness > 9 && random_bool()) {
    //         return prompt_options[random(0, prompt_options.size() - 1)];
    //     }
    //     else if (relationship->activated_goal == Goal::STAY_WITH) {
    //         relationship->attemping_goal = true;
    //         if (me->age > 35) {
    //             return prompt_options[10];
    //         }
    //         return prompt_options[2];
    //     }
    //     else if ((relationship->romance > 5 || relationship->friendship > 8) && random_bool()) {
    //         relationship->activated_goal = Goal::STAY_WITH;
    //         relationship->attemping_goal = true;
    //         return prompt_options[2];
    //     }
    //     else if (relationship->friendship > 2) {
    //         return prompt_options[random(3,7)];
    //     }
    //     else {
    //         return prompt_options[random(7,10)];
    //     }

    //     return DEFAULT;
    // };

    // inline static ResponseFunction allowed_petition = [](const shared_ptr<Relationship>& relationship, Intelligence* me, const vector<string>& history) -> string {
    //     vector<string> prompt_options = prompt_to_options[YES_ALLOW];
    //     if (relationship->attemping_goal) {
    //         relationship->attemping_goal = false;
    //         if (relationship->activated_goal == Goal::STAY_WITH) {
    //             me->follow();
    //         }
    //         else if (relationship->activated_goal == Goal::ASK_TO_STAY) {
    //             relationship->activated_goal = Goal::NONE;
    //         }
    //     }
    //     return prompt_options[random(0,prompt_options.size()-1)];
    // };

    // inline static ResponseFunction denied_petition = [](const shared_ptr<Relationship>& relationship, Intelligence* me, const vector<string>& history) -> string {
    //     vector<string> prompt_options = prompt_to_options[NO_DENY];
    //     if (relationship->attemping_goal) {
    //         relationship->attemping_goal = false;
    //         bool am_dependent = (me->get_anxiety() > 10 && relationship->trust > 8);
    //         if (relationship->activated_goal == Goal::STAY_WITH) {
    //             if (am_dependent) {
    //                 me->follow();
    //             }
    //         }
    //         else if (relationship->activated_goal == Goal::ASK_TO_STAY) {
    //             bool ask_again = relationship->friendship < 8 && random_bool();
    //             ask_again = ask_again || am_dependent;
    //             string last = FacadeIntelligenceOld::get_last(history);
    //             if (last == PLEASE_PETITION || last == CARE_TO_JOIN_ME || last == CAN_YOU_STAY) {
    //                 ask_again = ask_again && random(0, 10) > 8;
    //             }

    //             if (FacadeIntelligenceOld::player_last(history) == NO_DENY && !am_dependent) {
    //                 relationship->friendship -= 0.5;
    //                 clamp_0_10(relationship->friendship);
    //                 me->anger_mood += 1;
    //                 clamp_0_10(me->anger_mood);
    //             }

    //             if (!ask_again) {
    //                 relationship->activated_goal = Goal::NONE;
    //             }
    //             else {
    //                 return PLEASE_PETITION;
    //             }
    //         }
    //     }
    //     return prompt_options[random(0,prompt_options.size()-1)];
    // };


    // inline static ResponseFunction can_follow = [](const shared_ptr<Relationship>& relationship, Intelligence* me, const vector<string>& history) -> string {
    //     vector<string> prompt_options = prompt_to_options[WAIT_CAN_I_COME];
    //     if (me->craziness > 9 && random_bool()) {
    //         return prompt_options[random(0, prompt_options.size() - 1)];
    //     }
    //     else if ((relationship->romance > 5 || relationship->friendship > 8) && random_bool()) {
    //         return prompt_options[random(0, 3)];
    //     }
    //     else {
    //         relationship->activated_goal = Goal::GET_AWAY;
    //         relationship->attemping_goal = true;
    //         me->increase_anger();
    //         return prompt_options[prompt_options.size() - 1];
    //     }

    //     return DEFAULT;
    // };

    // inline static unordered_map<string, ResponseFunction> prompt_resolver = unordered_map<string, ResponseFunction> {
    //     { HI, greeting },
    //     { HELLO, greeting },
    //     { HELLO_MY_FRIEND, greeting },
    //     { HOW_DO_YOU_DO, greeting },
    //     { I_LOVE_YOU, 
    //         [](const shared_ptr<Relationship>& relationship, Intelligence* me, const vector<string>& history) -> string {
    //             vector<string> prompt_options = prompt_to_options[I_LOVE_YOU];
    //             if (me->craziness > 9 && random_bool()) {
    //                 return prompt_options[random(0, prompt_options.size() - 1)];
    //             }
    //             else if (relationship->romance > 9 && random_bool() && relationship->status != Relationship::Status::MARRIED) {
    //                 me->happiness_mood++;
    //                 return prompt_options[5];
    //             }
    //             else {
    //                 if (relationship->romance < 5) {
    //                     bool want_romance = me->want_romance(relationship);

    //                     // romance start
    //                     if (want_romance) {
    //                         relationship->friendship = clamp_0_10(++relationship->friendship);
    //                         relationship->romance = clamp_0_10(++relationship->romance);
    //                         me->happiness_mood++;
                    
    //                         return prompt_options[random(0, 2)];
    //                     }
    //                     // rejection
    //                     else {
    //                         if (relationship->interactions < 100 && random_bool()) {
    //                             return prompt_options[11];
    //                         }

    //                         relationship->friendship = clamp_0_10(relationship->friendship - 2.0);
    //                         relationship->romance = clamp_0_10(relationship->romance - 2.0);
    //                         relationship->trust = clamp_0_10(relationship->trust - 2.0);


    //                         string flight_or_fight_res = fight_or_flight(me);
    //                         if (flight_or_fight_res == prompt_options[6]) {
    //                             return prompt_options[6];
    //                         }
    //                         else if (flight_or_fight_res == prompt_options[7]) {
    //                             return prompt_options[7];
    //                         }
    //                         else {
    //                             me->anxiety_mood++;
    //                             me->anger_mood++;
    //                             return prompt_options[random(8, 11)];
    //                         }
    //                     }
    //                 }
    //                 else {
    //                     me->anxiety_mood--;
    //                     me->anger_mood--;
    //                     float min = relationship->trust < 4? -1 : 0; 
    //                     relationship->romance = clamp_0_10(relationship->romance + random_float(min, 0.1));
    //                     return prompt_options[random(2, 5)];
    //                 }
    //             }

    //             return DEFAULT;
    //         }
    //     },
    //     { MARRY_ME, 
    //         [](const shared_ptr<Relationship>& relationship, Intelligence* me, const vector<string>& history) -> string {
    //             vector<string> prompt_options = prompt_to_options[MARRY_ME];
    //             if (me->craziness > 9 && random_bool()) {
    //                 return prompt_options[random(0, prompt_options.size() - 1)];
    //             }
    //             else if (relationship->status == Relationship::Status::MARRIED) {
    //                 relationship->romance = clamp_0_10(relationship->romance - 0.2);
    //                 return prompt_options[5];
    //             }
    //             else if (relationship->status == Relationship::Status::ENGAGED) {
    //                 relationship->romance = clamp_0_10(relationship->romance - 0.2);
    //                 return prompt_options[5];
    //             }
    //             else if (
    //                 relationship->romance > 9 
    //                 && random_bool()
    //             ) {
    //                 relationship->status = Relationship::Status::ENGAGED;
    //                 return prompt_options[random(0, 2)];
    //             }
    //             else {
    //                 if (relationship->romance < 5) {
    //                     me->anxiety_mood++;
    //                     me->anger_mood++;

    //                     if (relationship->interactions < 100 && random_bool()) {
    //                         return prompt_options[10];
    //                     }

    //                     relationship->friendship = clamp_0_10(relationship->friendship - 3.0);
    //                     relationship->romance = clamp_0_10(relationship->romance - 3.0);
    //                     relationship->trust = clamp_0_10(relationship->trust - 2.0);

    //                     string flight_or_fight_res = fight_or_flight(me);
    //                     if (flight_or_fight_res == prompt_options[3]) {
    //                         return prompt_options[3];
    //                     }
    //                     else if (flight_or_fight_res == prompt_options[4]) {
    //                         return prompt_options[4];
    //                     }
    //                     else {
    //                         return prompt_options[random(7, 10)];
    //                     }
    //                 }
    //             }

    //             return DEFAULT;
    //         }
    //     },
    //     { BYE, parting },
    //     { FARE_THEE_WELL, parting },
    //     { GODSPEED, parting },
    //     { SO_LONG, parting },
    //     { GO_AWAY, parting },
    //     { WAIT_CAN_I_COME, can_follow },
    //     { MAY_I_FOLLOW, can_follow },
    //     { YES_ALLOW, allowed_petition },
    //     { AYE_ALLOW, allowed_petition },
    //     { I_GUESS_ALLOW, allowed_petition },
    //     { NO_DENY, denied_petition },
    //     { SORRY_I_CANT_TOO_BUSY_DENY, denied_petition },
    //     { FOLLOW, 
    //         [](const shared_ptr<Relationship>& relationship, Intelligence* me, const vector<string>& history) -> string {
    //             vector<string> prompt_options = prompt_to_options[FOLLOW];
    //             if (me->craziness > 9 && random_bool()) {
    //                 return prompt_options[random(0, prompt_options.size() - 1)];
    //             }
    //             else if (relationship->activated_goal == Goal::GET_AWAY) {
                    
    //                 me->increase_anger();
    //                 if (me->get_anger() > 9 && relationship->friendship < 0.4) {
    //                     relationship->activated_goal = Goal::MAIME_OR_KILL;
    //                     relationship->attemping_goal = true;
    //                     return prompt_options[3];
    //                 }
    //                 else if (me->get_anger() > 9){

    //                     me->decrease_friendship(relationship);
                        
    //                     return prompt_options[random(0,2)];
    //                 }
    //             }
    //             else {
    //                 return prompt_options[0];
    //             }

    //             return DEFAULT;
    //         }
    //     },
    
    // };

};



