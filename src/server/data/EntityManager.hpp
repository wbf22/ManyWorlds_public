#pragma once

#include <unordered_map>

#include "Player.hpp"
#include "server/Interface.hpp"
#include "util/Position.h"
#include "util/StellarCoordinate.hpp"


using namespace std;

struct EntityManager {

    struct Local {
        unordered_map<uint16_t, uint64_t> local_id_to_player_id;
    };

    struct LocalPos {
        bool on_planet_surface;
        shared_ptr<StellarCoordinate> stellar_coordinate;
        shared_ptr<Position> position;

        bool operator==(const LocalPos& o) const {
            if (on_planet_surface != o.on_planet_surface) return false;
            if (on_planet_surface) {
                if (position == o.position) return true;
                if (!position || !o.position) return false;
                return *position == *o.position;
            } else {
                if (stellar_coordinate == o.stellar_coordinate) return true;
                if (!stellar_coordinate || !o.stellar_coordinate) return false;
                return *stellar_coordinate == *o.stellar_coordinate;
            }
        }

        struct Hash {
            size_t operator()(const LocalPos& p) const {
                size_t h = hash<bool>()(p.on_planet_surface);
                if (p.on_planet_surface && p.position) {
                    h ^= p.position->hash();
                } else if (!p.on_planet_surface && p.stellar_coordinate) {
                    h ^= p.stellar_coordinate->hash();
                }
                return h;
            }
        };
    };

    struct HitEventAlerts {
        shared_ptr<Interface::Hit> hit;
        uint8_t attempts;
    };

    unordered_map<string, shared_ptr<Player>> player_id_to_player;
    unordered_map<string, shared_ptr<Player>> player_id_to_npcs;
    unordered_map<LocalPos, shared_ptr<Local>, LocalPos::Hash> locales;
    unordered_map<uint64_t, shared_ptr<HitEventAlerts>> player_id_to_hit_alerts;
    unordered_map<uint64_t, vector<shared_ptr<Interface::Hit>>> player_id_to_player_claimed_hits;


    void position_loop() {
        // loop players determining which npcs and other player updates to send based on distances
        // share data in a common pool so we don't recalculate much
        // very close npcs can have individual movement
        // npc's farther away are moved in groups and less frequently/detailed
        // determine any groups that should be split up into smaller groups or individuals based on distance to players
        // determine which npc's and npc groups to move (pathfinding if necessary with A* or something)

    }

    void macro_position_loop() {
        // do some work on different stellar levels (planet->universe) for major npc group movements (roaming bands, armies, migrations/refugees)
        // collect to send any of these updates if applicable
    }


    void hit_loop() {

    
        // loop through player_id_to_player_claimed_hits looking for player hit acknowlegments and remove any found from player_id_to_hit_alerts
        // verify anything else in  player_claimed_hits in queue for validity
        // add verified hits to data to send

        // determine attacking npcs and collect hit events to send to players
        // for each player make a list of npc and player hit events from the collected data and send
        // add any non resolved hit alerts to respective players (or remove if over 3 attempts)

    }


};
