#pragma once

#include "space/Time.hpp"
#include "npcs/Intelligence.h"
#include <cmath>
#include <algorithm>

using namespace std;

class Planet;
class Chunk;
struct SolarSystem;
struct Cluster;
struct Galaxy;
struct Chamber;
struct Cosmic_River;
struct Continuum;

struct Loan;
struct Stock;
struct Product;
struct Economy;
struct Government;

static constexpr double PRICE_ADJUST_K = 0.25;
static constexpr double TRADE_FRICTION = 0.02;
static constexpr double RANDOM_EVENT_CHANCE = 0.03;
static constexpr double CORRUPTION_LEAK = 0.05;
static constexpr double SURPLUS_DAMPEN = 0.05;
static constexpr double SHIFT_RATE = 0.03;
static constexpr double POPULATION_WANT_PER_CAPITA = 0.1;
static constexpr double DISCRETIONARY_WANT_SCALE = 0.05;
static constexpr double WANT_DECAY_RAW = 0.05;
static constexpr double WANT_DECAY_MANUFACTURED = 0.10;
static constexpr double WANT_DECAY_SERVICE = 0.20;
static constexpr double TECH_BASELINE_DRIFT = 0.01;

struct EconomyEvent {
    enum class Type {
        TECH_LOSS,
        PRODUCTION_SHOCK,
    };
    Type type;
    double magnitude;
    shared_ptr<Product> product = nullptr;
};

struct Loan {
    shared_ptr<Economy> lender;
    shared_ptr<Economy> borrower;
    double principal;
    double interest_rate;
    Time maturity_date;
    double evaluated_risk;
    unordered_map<shared_ptr<Product>, double> collateral;
};

struct Stock {
    shared_ptr<Economy> owned_entity;
    double ownership_portion;
};

struct Product {
    enum class ProductCategory {
        RAW_MATERIAL,
        MANUFACTURED_GOOD,
        SERVICE,
    };
    ProductCategory category = ProductCategory::RAW_MATERIAL;
    string name;
    bool population_tied = false;

};

struct Government {
    // Any of the territory areas a government can control. If it only controls a small area then the rest can be empty
    vector<shared_ptr<Continuum>> continuum_territory;
    vector<shared_ptr<Cosmic_River>> cosmic_river_territory;
    vector<shared_ptr<Chamber>> chamber_territory;
    vector<shared_ptr<Galaxy>> galaxy_territory;
    vector<shared_ptr<Cluster>> cluster_territory;
    vector<shared_ptr<SolarSystem>> solar_system_territory;
    vector<shared_ptr<Planet>> planet_territory;
    vector<shared_ptr<Chunk>> chunk_territory;


    string uuid; // gernerating from seed and spike
    shared_ptr<Government> parent_government;
    vector<shared_ptr<Government>> subsidery_governments;

    
    vector<shared_ptr<Intelligence>> leaders;
    vector<shared_ptr<Intelligence>> staff;

    enum class Succession {
        HERIDTARY,
        CHOSEN_AMONG_LEADERS,
        OPEN_PUBLIC_ELECTION,
        LEADER_CHOSEN_CANDIDATES_PUBLIC_ELECTION,

        // however all governments have a chance to have succession with war or rebellion

    };
    Succession succession;

    enum class EconomicControl {
        NONE, 
        REGULATED, // monopoly busting, some state owned
        CORRUPT, // government power is abused to enrich leaders
        COMPLETE, // government completely controls the economy
    };
    EconomicControl economic_control;

    enum class SocialisticType {
        NONE,
        SOME_SERVICES,
        SERVICES_AND_SOME_WEALTH_REDISTRIBUTION,
        COMPETE_WEALTH_REDISTRIBUTION
    };
    SocialisticType socialisticType;
    
    enum class Legitimacy {
        RELIGION,
        WEALTH,
        FORCE,
        EXPERTISE    
    };
    Legitimacy legitimacy;

    double movement_freedom; // 0-100
    double speech_freedom; // 0-100
    double cultural_religious_freedom; // 0-100
    double militarism; // 0-100

};

struct Economy {
    unordered_map<shared_ptr<Product>, double> production_amounts; // how much has been produced and put up for sale, basically supply
    unordered_map<shared_ptr<Product>, double> desired_amounts; // how much we want to buy
    unordered_map<shared_ptr<Product>, double> stored_amounts;
    unordered_map<shared_ptr<Product>, double> local_prices;
    double max_storage_capacity;
    double cash;
    shared_ptr<StellarCoordinate> location;
    vector<Loan> loans;
    vector<Stock> stocks;
    shared_ptr<Government> controlling_government;
    double technological_advancement = 1.0;
    double total_productive_capacity = 0;
    unordered_map<shared_ptr<Product>, double> max_production;
    int64_t economy_seed = 0;
    int64_t tick_counter = 0;
    double population = 100;
    vector<EconomyEvent> pending_events;

    void register_event(const EconomyEvent& e) {
        pending_events.push_back(e);
    }

    virtual vector<shared_ptr<Economy>> get_child_economies() {
        return {};
    }

    virtual double get_physical_scale() {
        return 1.0; // this is the scale of a single player
    }

    virtual unordered_map<shared_ptr<Product>, double> get_natural_resources() {
        return {};
    }

    void init_economy(shared_ptr<StellarCoordinate> location) {
        economy_seed = location->to_seed();
        int64_t s = economy_seed;

        technological_advancement = Random::randInt(s++, 0, 100);

        double base_scale = get_physical_scale();
        double gov_factor = 1.0;
        if (controlling_government) {
            gov_factor = 1.0 - 0.2 * static_cast<int>(controlling_government->economic_control);
            gov_factor *= 1.0 - 0.1 * (controlling_government->militarism / 100.0);
        }
        double economic_scale = base_scale * (1.0 + technological_advancement * 0.05) * gov_factor;

        max_storage_capacity = economic_scale * 1000.0 * Random::randDouble(s++, 0.8, 1.2);

        auto resources = get_natural_resources();

        for (auto& [product, amount] : resources) {
            product->category = Product::ProductCategory::RAW_MATERIAL;
            stored_amounts[product] = amount * economic_scale;
            production_amounts[product] = amount * 0.1 * economic_scale;
            local_prices[product] = Random::randDouble(s++, 1.0, 100.0);
        }

        int num_products = Random::randInt(s++, 1, 5);
        for (int i = 0; i < num_products; ++i) {
            auto product = make_shared<Product>();
            product->category = Product::ProductCategory::MANUFACTURED_GOOD;
            stored_amounts[product] = Random::randDouble(s++, 0, 100) * economic_scale;
            desired_amounts[product] = Random::randDouble(s++, 0, 100) * economic_scale;
            local_prices[product] = Random::randDouble(s++, 10.0, 1000.0);
        }

        cash = economic_scale * Random::randDouble(s++, 100, 100000);
        population = max(1.0, economic_scale * Random::randDouble(s++, 0.5, 2.0));

        for (auto& [product, amount] : stored_amounts) {
            if (desired_amounts.find(product) == desired_amounts.end()) {
                desired_amounts[product] = 0.0;
            }
        }
    }

    static double distance_between_ptr(const Economy* a, const Economy* b) {
        if (!a->location || !b->location) return 0.0;
        auto& l1 = *a->location;
        auto& l2 = *b->location;
        double dx = static_cast<double>(l1.quadrant_x - l2.quadrant_x) * 1e6
                  + static_cast<double>(l1.light_year_x - l2.light_year_x);
        double dy = static_cast<double>(l1.quadrant_y - l2.quadrant_y) * 1e6
                  + static_cast<double>(l1.light_year_y - l2.light_year_y);
        double dz = static_cast<double>(l1.quadrant_z - l2.quadrant_z) * 1e6
                  + static_cast<double>(l1.light_year_z - l2.light_year_z);
        return sqrt(dx*dx + dy*dy + dz*dz);
    }

    double production_cost() const {
        return 100.0 / (1.0 + technological_advancement * 0.1);
    }

    // effective demand: base want rate modulated by price and inventory
    double effective_demand(const shared_ptr<Product>& product) const {
        double rate = 0;
        auto it = desired_amounts.find(product);
        if (it != desired_amounts.end()) rate = it->second;
        if (rate <= 0) return 0;

        double price = 0;
        auto jt = local_prices.find(product);
        if (jt != local_prices.end()) price = jt->second;

        double cost = production_cost();

        double stored = 0;
        auto kt = stored_amounts.find(product);
        if (kt != stored_amounts.end()) stored = kt->second;

        double elastic = 2.0 * cost / max(cost + price, 0.01);
        double saturation = max(0.0, 1.0 - stored / max(max_storage_capacity * 0.15, 1.0));
        return rate * elastic * saturation;
    }

    void balance(int iterations = 20) {
        double cost = production_cost();
        // save stored amounts and clear them so effective_demand ignores saturation
        auto saved_stored = stored_amounts;
        for (auto& [p, _] : stored_amounts) stored_amounts[p] = 0;

        for (int i = 0; i < iterations; ++i) {
            // price discovery
            for (auto& [product, price] : local_prices) {
                double supply = production_amounts[product];
                double demand = effective_demand(product);
                double imbalance = (demand + 0.01) / max(supply, 0.01);
                double target = price * pow(imbalance, 0.3);
                target = max(target, cost * 0.5);
                target = min(target, cost * 50.0);
                price = price * 0.5 + target * 0.5;
            }

            // capacity allocation toward profitable products
            if (total_productive_capacity > 0 && !max_production.empty()) {
                double total_score = 0;
                unordered_map<shared_ptr<Product>, double> scores;
                for (auto& [product, _] : production_amounts) {
                    double p = local_prices[product];
                    double score = max(0.01, p / cost);
                    scores[product] = score;
                    total_score += score;
                }
                if (total_score > 0) {
                    for (auto& [product, amt] : production_amounts) {
                        double share = scores[product] / total_score;
                        double target = total_productive_capacity * share;
                        double cap = max_production.count(product) ? max_production[product]
                                   : amt * 2.0;
                        double capped = min(target, cap);
                        amt += (capped - amt) * 0.5;
                        amt = max(amt, 0.0);
                    }
                }
            }
        }

        stored_amounts = saved_stored;
    }

    void tick(vector<shared_ptr<Economy>>& peers, int detail_level = 0) {
        technological_advancement += TECH_BASELINE_DRIFT;
        for (auto& peer : peers) {
            if (peer.get() != this && peer->technological_advancement > technological_advancement)
                technological_advancement += TECH_BASELINE_DRIFT;
        }

        int64_t s = economy_seed + tick_counter * 1009;
        tick_counter++;

        double cost = production_cost();

        // Phase 0 — capacity reallocation toward profitable products
        if (total_productive_capacity > 0 && !max_production.empty()) {
            double total_score = 0;
            unordered_map<shared_ptr<Product>, double> scores;
            for (auto& [product, _] : production_amounts) {
                double price = local_prices[product];
                double score = max(0.01, price / cost) * Random::randDouble(s++, 0.9, 1.1);
                scores[product] = score;
                total_score += score;
            }
            if (total_score > 0) {
                for (auto& [product, amt] : production_amounts) {
                    double share = scores[product] / total_score;
                    double target = total_productive_capacity * share;
                    double cap = max_production.count(product) ? max_production[product]
                               : amt * 2.0;
                    double capped = min(target, cap);
                    double diff = capped - amt;
                    amt += diff * SHIFT_RATE;
                    amt = max(amt, 0.0);
                }
            }
        }

        // shrink capacity of persistently unprofitable products
        for (auto& [product, amt] : production_amounts) {
            double price = local_prices[product];
            double profitability = price / max(cost, 0.01);
            if (profitability < 1.0) {
                double shrink = amt * 0.005 * (1.0 - profitability);
                amt -= shrink;
                total_productive_capacity = max(0.0, total_productive_capacity - shrink);
            }
        }

        // Phase 1 — production (price-responsive)
        for (auto& [product, base_amount] : production_amounts) {
            double price = local_prices[product];
            double profitability = price / max(cost, 0.01);
            double response = clamp(profitability * 0.5 + 0.5, 0.1, 3.0);
            double noise = Random::randDouble(s++, 0.85, 1.15);
            double produced = base_amount * response * noise;

            double raw_total = 0;
            for (auto& [p, a] : stored_amounts) {
                if (p->category == Product::ProductCategory::RAW_MATERIAL)
                    raw_total += a;
            }
            double material_factor = min(1.0, raw_total / max(1.0, max_storage_capacity * 0.05));
            produced *= (0.5 + 0.5 * material_factor);

            stored_amounts[product] += produced;
        }

        // Phase 2 — consumption (effective demand pulls from storage)
        for (auto& [product, _] : desired_amounts) {
            double want = effective_demand(product) * Random::randDouble(s++, 0.9, 1.1);
            double available = stored_amounts[product];
            double consumed = min(want, available);
            stored_amounts[product] -= consumed;
            if (stored_amounts[product] < 0) stored_amounts[product] = 0;
        }

        // enforce storage cap
        double total_stored = 0;
        for (auto& [prod, amt] : stored_amounts) total_stored += amt;
        if (total_stored > max_storage_capacity) {
            double scale = max_storage_capacity / max(total_stored, 1.0);
            for (auto& [prod, amt] : stored_amounts) amt *= scale;
        }

        // Phase 2b — want refresh / decay toward baseline
        for (auto& [product, rate] : desired_amounts) {
            if (product->population_tied) {
                desired_amounts[product] = population * POPULATION_WANT_PER_CAPITA;
            } else {
                double baseline = total_productive_capacity * DISCRETIONARY_WANT_SCALE;
                double decay = WANT_DECAY_RAW;
                if (product->category == Product::ProductCategory::MANUFACTURED_GOOD)
                    decay = WANT_DECAY_MANUFACTURED;
                else if (product->category == Product::ProductCategory::SERVICE)
                    decay = WANT_DECAY_SERVICE;
                desired_amounts[product] += (baseline - rate) * decay;
            }
        }

        // Phase 3 — trade with peers (sorted by distance)
        vector<pair<double, shared_ptr<Economy>>> ordered;
        for (auto& peer : peers) {
            if (peer.get() == this) continue;
            ordered.push_back({distance_between_ptr(this, peer.get()), peer});
        }
        sort(ordered.begin(), ordered.end(),
             [](auto& a, auto& b) { return a.first < b.first; });

        for (auto& [dist, peer] : ordered) {
            for (auto& [product, _] : local_prices) {
                double my_inventory = stored_amounts[product];
                if (my_inventory <= 0) continue;

                double peer_price = peer->local_prices[product];
                double my_price = local_prices[product];

                double peer_inventory = peer->stored_amounts[product];
                if (peer_inventory >= my_inventory * 0.5 && peer_price <= my_price * 1.1)
                    continue;

                double trade_price = (my_price + peer_price) * 0.5
                                   * (1.0 + TRADE_FRICTION * dist);

                double max_i_can_sell = my_inventory * 0.2;
                double max_they_can_afford = peer->cash / max(trade_price, 0.01);
                double peer_demand = peer->effective_demand(product);
                double max_they_need = max(0.0, peer_demand * 5 - peer_inventory);

                double volume = min({max_i_can_sell, max_they_can_afford,
                                     max_they_need, 100.0});
                volume = max(volume, 0.0);
                volume *= Random::randDouble(s++, 0.9, 1.0);
                if (volume < 0.01) continue;

                double cost_val = volume * trade_price;

                stored_amounts[product] -= volume;
                peer->stored_amounts[product] += volume;
                cash += cost_val;
                peer->cash -= cost_val;
            }
        }

        // Phase 4 — recalculate local prices toward equilibrium
        unordered_map<shared_ptr<Product>, double> prev_prices = local_prices;
        for (auto& [product, price] : local_prices) {
            double stored = stored_amounts[product];
            double supply_flow = production_amounts[product] + stored * SURPLUS_DAMPEN;
            double demand_flow = effective_demand(product);
            double imbalance = (demand_flow + 0.1) / max(supply_flow, 0.1);
            double target_price = price * pow(imbalance, PRICE_ADJUST_K);
            target_price = max(target_price, cost * 0.5);
            target_price = min(target_price, cost * 50.0);

            price = price * 0.8 + target_price * 0.2;
            price *= Random::randDouble(s++, 0.97, 1.03);
        }

        // Phase 5 — government effects
        if (controlling_government) {
            auto& gov = *controlling_government;
            if (gov.economic_control == Government::EconomicControl::COMPLETE) {
                local_prices = prev_prices;
            } else if (gov.economic_control == Government::EconomicControl::REGULATED) {
                for (auto& [prod, p] : local_prices) {
                    double stored = stored_amounts[prod];
                    double supply_flow = production_amounts[prod] + stored * SURPLUS_DAMPEN;
                    double demand_flow = effective_demand(prod);
                    double imbalance = (demand_flow + 0.1) / max(supply_flow, 0.1);
                    double target = p * pow(imbalance, PRICE_ADJUST_K * 0.5);
                    target = max(target, cost * 0.3);
                    target = min(target, cost * 25.0);
                    p = p * 0.9 + target * 0.1;
                }
            } else if (gov.economic_control == Government::EconomicControl::CORRUPT) {
                double leak = cash * CORRUPTION_LEAK * Random::randDouble(s++, 0.5, 1.5);
                cash -= leak;
            }

            double tax_rate = 0.01 * (gov.militarism / 100.0) * Random::randDouble(s++, 0.5, 1.5);
            cash -= cash * tax_rate;

            if (gov.socialisticType >= Government::SocialisticType::SERVICES_AND_SOME_WEALTH_REDISTRIBUTION) {
                double redistribution = 0.02 * Random::randDouble(s++, 0.5, 1.5);
                cash -= cash * redistribution;
            }
        }

        // Phase 6 — random events
        if (Random::randDouble(s++, 0, 1) < RANDOM_EVENT_CHANCE) {
            auto it = local_prices.begin();
            if (!local_prices.empty()) {
                advance(it, Random::randInt(s++, 0, local_prices.size() - 1));
                auto product = it->first;
                if (product->population_tied) {
                    // population-driven wants don't get random spikes
                } else if (Random::randDouble(s++, 0, 1) < 0.5) {
                    production_amounts[product] *= Random::randDouble(s++, 1.5, 3.0);
                    auto cap_it = max_production.find(product);
                    if (cap_it != max_production.end())
                        production_amounts[product] = min(production_amounts[product], cap_it->second);
                } else {
                    desired_amounts[product] += Random::randDouble(s++, 50, 200);
                }
            }
        }

        // Phase 7 — consume external events
        for (auto& event : pending_events) {
            if (event.type == EconomyEvent::Type::TECH_LOSS) {
                technological_advancement = max(0.0, technological_advancement
                                                - event.magnitude);
            } else if (event.type == EconomyEvent::Type::PRODUCTION_SHOCK) {
                double loss = event.magnitude;
                if (event.product && production_amounts.count(event.product)) {
                    double cur = production_amounts[event.product];
                    double lost = cur * loss;
                    production_amounts[event.product] = max(0.0, cur - lost);
                    total_productive_capacity = max(0.0, total_productive_capacity - lost);
                } else if (!event.product) {
                    double total_loss = total_productive_capacity * loss;
                    for (auto& [p, amt] : production_amounts) {
                        double share = amt / max(total_productive_capacity, 1.0);
                        production_amounts[p] = max(0.0, amt - total_loss * share);
                    }
                    total_productive_capacity = max(0.0, total_productive_capacity - total_loss);
                }
            }
        }
        pending_events.clear();

        // Phase 8 — recurse into child economies
        if (detail_level < 3) {
            auto children = get_child_economies();
            for (auto& child : children) {
                child->tick(peers, detail_level + 1);
            }
        }
    }

};


