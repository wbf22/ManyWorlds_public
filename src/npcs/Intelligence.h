#pragma once

#include <string>
#include <vector>
#include "util/Util.hpp"
#include "util/Random.h"
#include "util/serialization/KVSerializer.hpp"


#include <unordered_map>

using namespace std;



// UTIL
inline int CONVERSATION_SEED = Util::seed();
inline bool random_bool() {
    ++CONVERSATION_SEED;
    return Random::randBool(CONVERSATION_SEED);
}

inline float random(float min, float max) {
    ++CONVERSATION_SEED;
    return Random::randDouble(CONVERSATION_SEED, min, max);
}

inline float random(float min, float max, float avg) {
    ++CONVERSATION_SEED;
    return Random::randDouble(CONVERSATION_SEED, min, avg, max);
}


struct Gender {
    inline static string MALE = "MALE";
    inline static string FEMALE = "FEMALE";
    inline static string IN_BETWEEN = "IN_BETWEEN";
};


struct Goal {
    inline static string ROMANCE = "ROMANCE";
    inline static string FRIEND = "FRIEND";
    inline static string FUN = "FUN";
    inline static string BUSINESS_PARTNER = "BUSINESS_PARTNER";
    inline static string TRADE = "TRADE";
    inline static string BUSINESS = "BUSINESS";
    inline static string CREATE_PRODUCT = "CREATE_PRODUCT";
    inline static string MAIME_OR_KILL = "MAIME_OR_KILL";
    inline static string STAY_WITH = "STAY_WITH";
    inline static string GET_AWAY = "GET_AWAY";
    inline static string ASK_TO_STAY = "ASK_TO_STAY";
    inline static string ROB = "ROB";
    inline static string CONTROL = "CONTROL";
    inline static string FAME = "FAME";
    inline static string NONE = "NONE";
};

/**
 * Relationship status between entities in the game.
 * Most will be between players and npcs, but npcs 
 * will also have some relationships.
 */
struct Relationship {

    // uuids of entities in the relationship
    string me_uuid;
    string other_uuid;

    // attributes of the realtionship (out of 10)
    float friendship = 3.0; 
    float romance = 0.0;
    float trust = 5.0;
    float random_tick = random(0, 10);

    // other
    int interactions = 0;
    struct Status {
        inline static string MARRIED = "MARRIED";
        inline static string ENGAGED = "ENGAGED";
        inline static string SIBLING = "SIBLING";
        inline static string PARENT = "PARENT";
        inline static string CHILD = "CHILD";
        inline static string ENEMY = "ENEMY";
        inline static string FRIEND = "FRIEND";
        inline static string BUSINESS_PARTNER = "BUSINESS_PARTNER";
        inline static string OTHER = "OTHER";
    };
    string status = Status::OTHER;

    // goals
    string goal = Goal::NONE;
    bool goal_asked_for;

    void randomize() {
        this->friendship = random(0, 10);
        this->romance = random(0, 10);
        this->trust = random(0, 10);
        this->interactions = random(0, 100);

        if (this->friendship > 4) this->interactions = random(10, 100);

        int status_value = random(0, 15);
        if (status_value == 0) {
            this->status = Status::MARRIED;
        }
        else if (status_value == 1) {
            this->status = Status::ENGAGED;
        }
        else if (status_value == 2) {
            this->status = Status::SIBLING;
        }
        else if (status_value == 3) {
            this->status = Status::PARENT;
        }
        else if (status_value == 4) {
            this->status = Status::CHILD;
        }
        else if (status_value == 5) {
            this->status = Status::ENEMY;
        }
        else if (status_value == 6) {
            this->status = Status::FRIEND;
        }
        else if (status_value == 7) {
            this->status = Status::BUSINESS_PARTNER;
        }
        else if (status_value >= 8) {
            this->status = Status::OTHER;
        }

    }

    float diff(const Relationship& other) {
        float dist = 0;

        dist += abs(this->friendship - other.friendship);
        dist += abs(this->romance - other.romance);
        dist += abs(this->trust - other.trust);
        dist += this->status == other.status? 0 : 10;
        dist += this->goal == other.goal? 0 : 10;
        dist += this->goal_asked_for == other.goal_asked_for? 0 : 10;

        return dist / 6;
    }

    void tick() {
        random_tick = random(0, 10);
    }

    string to_string() {
        vector<pair<string, string>> attributes = {
            {"friendship", Util::to_string(this->friendship)},
            {"romance", Util::to_string(this->romance)},
            {"trust", Util::to_string(this->trust)},
            {"interactions", Util::to_string(this->interactions)},
            {"status", this->status},
            {"goal", this->goal},
            {"goal_asked_for", Util::to_string(this->goal_asked_for)},
        }; 
        return Util::print_map_in_columns(attributes);
    }

    string summary() {
        stringstream ss;
        ss << "This npc with the other npc is ";
        if (this->friendship > 7) {
            ss << "a good friend, ";
        }
        else if (this->friendship > 4) {
            ss << "a friend, ";
        }

        if (this->romance > 7) {
            ss << "in love, ";
        }
        else if (this->romance > 3) {
            ss << "possibly in love, ";
        }

        if (this->trust < 5) {
            ss << "non trusting, ";
        }

        if (this->interactions < 5) {
            ss << "a stranger to, ";
        }

        ss << "a " << this->status;
        if (this->goal != Goal::NONE) {
            ss << ", ";
            ss << "and wants " << this->goal;
        } 

        return ss.str();
    }


    DECLARE_NAMED_FIELDS(
        FIELD("me_uuid", &Relationship::me_uuid),
        FIELD("other_uuid", &Relationship::other_uuid),
        FIELD("friendship", &Relationship::friendship),
        FIELD("romance", &Relationship::romance),
        FIELD("trust", &Relationship::trust),
        FIELD("random_tick", &Relationship::random_tick),
        FIELD("interactions", &Relationship::interactions),
        FIELD("status", &Relationship::status),
        FIELD("goal", &Relationship::goal),
        FIELD("goal_asked_for", &Relationship::goal_asked_for)
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
struct Intelligence {

    string uuid;

    // personality (out of 10)
    float goodness = 5.0;
    float shyness = 5.0;
    float anxiety = 5.0;
    float silliness = 5.0;
    float laziness = 5.0;
    float anger = 5.0;
    float happiness = 5.0;
    float weirdness = 5.0;
    float craziness = 5.0;
    float intelligence = 5.0;
    
    // mood (0 to 10)
    float anger_mood = 0.0;
    float anxiety_mood = 0.0;
    float happiness_mood = 0.0;

    // other
    float age = 35; // percent of life time
    float height = 2; // meters
    string gender;
    float last_update = Util::time();

    // relationships
    unordered_map<string, shared_ptr<Relationship>> relationships;
    shared_ptr<Relationship> marriage = nullptr;

    // goals mostly for occupation
    string goal = Goal::NONE;
    bool working_on_goal = false;


    

    static void reconcile_relationships(shared_ptr<Intelligence> intelligence, shared_ptr<Intelligence> other_intelligence, shared_ptr<Relationship>& relationship, shared_ptr<Relationship>& other_relationship) {
        relationship->me_uuid = intelligence->uuid;
        relationship->other_uuid = other_intelligence->uuid;
        intelligence->relationships[other_intelligence->uuid] = relationship;

        other_relationship->me_uuid = other_intelligence->uuid;
        other_relationship->other_uuid = intelligence->uuid;
        other_intelligence->relationships[intelligence->uuid] = other_relationship;

        // STATUS RECONCILIATION

        // inline static string MARRIED = "MARRIED";
        // inline static string ENGAGED = "ENGAGED";
        // inline static string SIBLING = "SIBLING";
        // inline static string CHILD = "CHILD";
        // inline static string ENEMY = "ENEMY";
        // inline static string FRIEND = "FRIEND";
        // inline static string BUSINESS_PARTNER = "BUSINESS_PARTNER";
        // inline static string OTHER = "OTHER";
        other_relationship->status = relationship->status;
        if (relationship->status == Relationship::Status::MARRIED || relationship->status == Relationship::Status::ENGAGED) {
            if (intelligence->gender == Gender::MALE) {
                other_intelligence->gender = Gender::FEMALE;
            }
            else if (intelligence->gender == Gender::FEMALE) {
                other_intelligence->gender = Gender::MALE;
            }
            else if (intelligence->gender == Gender::IN_BETWEEN) {
                other_intelligence->gender = Gender::IN_BETWEEN;
            }
            relationship->romance = random(4, 10);
            other_relationship->romance = random(4, 10);
            relationship->interactions = random(100, 1000);
            other_relationship->interactions = relationship->interactions;

            int age = random(14, 100);
            intelligence->age = age * random(0.95, 1.05);
            other_intelligence->age = age * random(0.95, 1.05);
        }
        else if (relationship->status == Relationship::Status::SIBLING) {
            other_intelligence->age = (intelligence->age + other_intelligence->age) / 2;
        }
        else if (relationship->status == Relationship::Status::PARENT) {
            other_relationship->status = Relationship::Status::CHILD;
            other_intelligence->age = intelligence->age;
            intelligence->age += random(16, 40);
        }
        else if (relationship->status == Relationship::Status::CHILD) {
            other_relationship->status = Relationship::Status::PARENT;
            other_intelligence->age = intelligence->age + random(16, 40);
        }
        else if (relationship->status == Relationship::Status::ENEMY) {
            relationship->friendship = random(0.0, 1.0);
            other_relationship->friendship = random(0.0, 1.0);
        }
        else if (relationship->status == Relationship::Status::FRIEND) {
            relationship->friendship = random(5, 10);
            other_relationship->friendship = random(5, 10);
        }




        // ROMANCE RECONCILIATION
        intelligence->reconcile_romance_level(other_intelligence, relationship);
        other_intelligence->reconcile_romance_level(intelligence, other_relationship);
    }

    void reconcile_romance_level(shared_ptr<Intelligence> other_intelligence, shared_ptr<Relationship>& relationship) {
        if (this->gender == other_intelligence->gender && this->gender != Gender::IN_BETWEEN) relationship->romance = random(0.0, 0.5);
        if (this->age < 8) relationship->romance = 0.0;

        // if not evil (not sex predator)
        bool sex_predator = random_bool() && this->goodness < 1 && this->age > other_intelligence->age;
        if (!sex_predator) {
            /*
                Age differences for romance

                20 and 18
                - 2/18=.111
                - 18/166=.10

                50 and 65
                - 15/50=.3
                - 50/166=.3

                100 and 70
                - 30/70=.42
                - 70/166=.42
            
            */
            float min_age = min(this->age, other_intelligence->age);
            float age_diff_tolerance = min_age / 166.0;
            float diff_div_age = abs(this->age - other_intelligence->age) / min_age;
            if (diff_div_age > age_diff_tolerance) relationship->romance = random(0.0, 1.0);

            if (
                relationship->status == Relationship::Status::PARENT ||
                relationship->status == Relationship::Status::CHILD ||
                relationship->status == Relationship::Status::SIBLING
            ) {
                relationship->romance = random(0.0, 1.0);
            }

        }


        

    }

    void randomize() {
        this->goodness = random(0, 6, 10);
        this->shyness = random(0, 10);
        this->anxiety = random(0, 10);
        this->silliness = random(0, 10);
        this->laziness = random(0, 10);
        this->anger = random(0, 10);
        this->happiness = random(0, 10);
        this->weirdness = random(0, 10);
        this->craziness = random(0, 10);
        this->intelligence = random(0, 10);

        this->anger_mood = random(0, 10);
        this->anxiety_mood = random(0, 10);
        this->happiness_mood = random(0, 10);

        this->age = random(3, 100);
        this->height = random(0.5, 5);
        
        float gender_v = random(0, 2.2);
        if (gender_v < 1) this->gender = Gender::FEMALE;
        else if (gender_v < 2) this->gender = Gender::MALE;
        else this->gender = Gender::IN_BETWEEN;

        int goal_int = random(0, 5);
        if (goal_int == 0) {
            this->goal = Goal::TRADE;
        }
        else if (goal_int == 1) {
            this->goal = Goal::FUN;
        }
        else if (goal_int == 2) {
            this->goal = Goal::CONTROL;
        }
        else if (goal_int == 3) {
            this->goal = Goal::ROB;
        }
        else {
            this->goal = Goal::NONE;
        }
        this->working_on_goal = random_bool();
    }

    float diff(const Intelligence& other) {

        float dist = 0;
        dist += abs(this->goodness - other.goodness);
        dist += abs(this->shyness - other.shyness);
        dist += abs(this->anxiety - other.anxiety);
        dist += abs(this->silliness - other.silliness);
        dist += abs(this->laziness - other.laziness);
        dist += abs(this->anger - other.anger);
        dist += abs(this->happiness - other.happiness);
        dist += abs(this->weirdness - other.weirdness);
        dist += abs(this->craziness - other.craziness);
        dist += abs(this->intelligence - other.intelligence);

        dist += abs(this->anger_mood - other.anger_mood);
        dist += abs(this->anxiety_mood - other.anxiety_mood);
        dist += abs(this->happiness_mood - other.happiness_mood);

        dist += abs(this->age - other.age);
        dist += abs(this->height - other.height);
        dist += this->gender == other.gender? 0 : 10;

        dist += this->goal == other.goal? 0 : 10;
        dist += this->working_on_goal == other.working_on_goal? 0 : 10;

        return dist / 18;
    }

    string to_string() {
        vector<pair<string, string>> attributes = {
            {"uuid", this->uuid},
            {"goodness", Util::to_string(this->goodness)},
            {"shyness", Util::to_string(this->shyness)},
            {"anxiety", Util::to_string(this->anxiety)},
            {"silliness", Util::to_string(this->silliness)},
            {"laziness", Util::to_string(this->laziness)},
            {"anger", Util::to_string(this->anger)},
            {"happiness", Util::to_string(this->happiness)},
            {"weirdness", Util::to_string(this->weirdness)},
            {"craziness", Util::to_string(this->craziness)},
            {"intelligence", Util::to_string(this->intelligence)},
            {"anger_mood", Util::to_string(this->anger_mood)},
            {"anxiety_mood", Util::to_string(this->anxiety_mood)},
            {"happiness_mood", Util::to_string(this->happiness_mood)},
            {"age", Util::to_string(this->age)},
            {"height", Util::to_string(this->height)},
            {"gender", this->gender},
            {"last_update", Util::to_string(this->last_update)},
            {"goal", this->goal},
            {"working_on_goal", Util::to_string(this->working_on_goal)},
        }; 
        return Util::print_map_in_columns(attributes);
    }

    string summary() {
        stringstream ss;
        ss << "This npc is ";
        if (this->goodness > 6) {
            ss << "a good person, ";
        }
        else if (this->goodness < 3) {
            ss << "evil, ";
        }

        if (this->shyness > 8) {
            ss << "pretty shy, ";
        }

        if (this->anxiety + this->anxiety_mood > 14) {
            ss << "anxious, ";
        }
        else if (this->anxiety + this->anxiety_mood < 6) {
            ss << "care free, ";
        }

        if (this->silliness > 7) {
            ss << "silly, ";
        }
        else if (this->silliness < 3) {
            ss << "serious, ";
        }

        if (this->laziness > 7) {
            ss << "lazy, ";
        }
        else if (this->laziness < 3) {
            ss << "hardworking, ";
        }

        if (this->anger + this->anger_mood > 14) {
            ss << "angry, ";
        }
        else if (this->anger + this->anger_mood < 6) {
            ss << "forgiving, ";
        }

        if (this->happiness + this->happiness_mood > 14) {
            ss << "happy, ";
        }
        else if (this->happiness + this->happiness_mood < 6) {
            ss << "sad, ";
        }

        if (this->weirdness > 7) {
            ss << "weird, ";
        }

        if (this->craziness > 7) {
            ss << "crazy, ";
        }

        if (this->intelligence > 7) {
            ss << "smart, ";
        }
        else if (this->intelligence < 3) {
            ss << "stupid, ";
        }
    
        ss << this->height << "m, ";
        ss << round(this->age) << ", ";

        ss << "and " << this->gender << ". ";

        ss << "Working to " << this->goal;
        if (this->working_on_goal) {
            ss << " and is currently at work. ";
        }
        else {
            ss << " but isn't at work currently. ";
        }

        return ss.str();
    }

    // ACTIONS

    void follow() {
        cout << "FOLLOW PLAYER TRIGGERED" << endl;
    };

    void kiss() {
        cout << "KISS PLAYER TRIGGERED" << endl;

    };

    void fight() {
        cout << "FIGHT PLAYER TRIGGERED" << endl;
        
    };

    void ask_for_help() {
        cout << "ASK FOR HELP TRIGGERED" << endl;
        
    };  

    void hug() {

        cout << "HUG PLAYER TRIGGERED" << endl;
    }

    void run() {

        cout << "RUN TRIGGERED" << endl;
    }

    void trade() {

        cout << "TRADE TRIGGERED" << endl;
    }

    void fart() {

        cout << "FART TRIGGERED" << endl;
    }

    void laugh() {
        cout << "LAUGH TRIGGERED" << endl;
    }

    void give_gift() {
        cout << "GIVE GIFT TRIGGERED" << endl;
    }



    DECLARE_NAMED_FIELDS(
        FIELD("uuid", &Intelligence::uuid),
        FIELD("goodness", &Intelligence::goodness),
        FIELD("shyness", &Intelligence::shyness),
        FIELD("anxiety", &Intelligence::anxiety),
        FIELD("silliness", &Intelligence::silliness),
        FIELD("laziness", &Intelligence::laziness),
        FIELD("anger", &Intelligence::anger),
        FIELD("happiness", &Intelligence::happiness),
        FIELD("weirdness", &Intelligence::weirdness),
        FIELD("craziness", &Intelligence::craziness),
        FIELD("intelligence", &Intelligence::intelligence),
        FIELD("anger_mood", &Intelligence::anger_mood),
        FIELD("anxiety_mood", &Intelligence::anxiety_mood),
        FIELD("happiness_mood", &Intelligence::happiness_mood),
        FIELD("age", &Intelligence::age),
        FIELD("height", &Intelligence::height),
        FIELD("gender", &Intelligence::gender),
        FIELD("last_update", &Intelligence::last_update),
        FIELD("goal", &Intelligence::goal),
        FIELD("working_on_goal", &Intelligence::working_on_goal)
    );


    //*********
    // OLD
    //*********


    float get_anger() {
        return this->anger + this->anger_mood;
    }

    float get_anxiety() {
        return this->anxiety + this->anxiety_mood;
    }

    float get_happiness() {
        return this->happiness + this->happiness_mood;
    }

    void update_mood() {
        double time_since = Util::time() - last_update;
        double normalizer = time_since / (60 * 60 * 24); // divide by days

        this->anger_mood *= normalizer;
        this->anxiety_mood *= normalizer;
        this->happiness_mood *= normalizer;
    }


    // DISPOSITION

    string choose_goal(const shared_ptr<Relationship>& relationship) { return Goal::NONE; }

    float want_to_be_friend(const shared_ptr<Relationship>& relationship) { return 0; };
    
    bool want_business_partner(const shared_ptr<Relationship>& relationship) { return false; };

    bool want_romance(const shared_ptr<Relationship>& relationship) { return false; };

    float want_to_fight(const shared_ptr<Relationship>& relationship) { return 0; };

    /**
     * Increases the anger mood of the player slightly. (0-0.1)
     * 
     * mult is multiplied against that number and added to anger_mood
     */
    void increase_anger(float mult = 1) {
        float mood_mod = this->anger * 0.001;
        mood_mod *= (10 - this->goodness);
        this->anger_mood += mood_mod * mult;
    }


    void decrease_friendship(const shared_ptr<Relationship>& relationship) {
        float decrease = this->get_anger() * 0.001;
        decrease *= (10 - this->goodness);
        relationship->friendship -= decrease;
    }

};



