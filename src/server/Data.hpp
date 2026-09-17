#pragma once


#include <string>
#include <thread>
#include <vector>
#include <iostream>
#include "util/sockets/UDPSocket.hpp"
#include "util/sockets/TCPSocket.hpp"
#include <unordered_set>
#include <unordered_map>
#include "server/ServerUtil.h"
#include <queue>
#include <shared_mutex>
#include "util/TSafe.hpp"
#include "data/Request.hpp"
#include "data/Player.hpp"
#include "Settings.hpp"
#include "util/CryptoRandom.hpp"
#include "security/captcha/CaptchaPuzzles.hpp"
#include "../server/Interface.hpp"
#include "data/Parser.hpp"
#include <filesystem>
#include "data/Commands.hpp"
#include "util/Logger.hpp"
#include <sstream>
#include <condition_variable>


#define RED "\033[1;31m"
#define RESET "\033[0m"


using namespace std;

class Data {
public:

    static const constexpr char* SHRUG = "¯\\_(ツ)_/¯";

    static const constexpr int MAX_IP_LIST_SIZE = 1000000; // should never have more than 1,000,000 clients
    static const constexpr int MAX_TCP_SEND = 200000; // largest tcp message we send (puzzle images)

    // server
    bool running;


    // socket
    TSafe<UDPSocket> udp_socket;
    TSafe<TCPSocket> tcp_socket;


    // client requests
    condition_variable cv;
    TSafe<queue<shared_ptr<Request>>> client_requests;
    TSafe<queue<shared_ptr<Request>>> auth_requests;


    // security
    TSafe<unordered_map<string, string>> logged_in_ip_address_to_player_id; // list of players as ipaddr::port to player_id

    TSafe<unordered_map<string, chrono::steady_clock::time_point>> ip_address_type_last_request_time; 

    TSafe<unordered_set<string>> blacklist;

    TSafe<unordered_map<string, string>> player_id_to_salted_password;

    TSafe<unordered_map<string, chrono::steady_clock::time_point>> last_login_attempt_by_id; 

    TSafe<CaptchaPuzzles> puzzle_pool;

    TSafe<unordered_set<string>> op_levels;


    // players
    TSafe<vector<string>> player_ids;
    TSafe<unordered_map<string, shared_ptr<Player>>> player_id_to_player;


    Data(int port, int timeout_ms) 
    : udp_socket(UDPSocket(timeout_ms, port)),
    tcp_socket(TCPSocket(timeout_ms))
    {
        try {
            tcp_socket.run_with_lock([&](auto& sock) {
                sock.server_init(port + 1);
            });
        }
        catch(const runtime_error& e) {
            stringstream ss;
            ss << "Failed to start TCP server on port " << to_string(port + 1);
            ss << ". Manyworlds uses the port you provide and that port + 1 for auth requests.";
            ss << " It's possible this port is already in use by another app. Typically ports above";
            ss << " 20000 have a better change of being unused." << endl << endl;
            ss << e.what() << endl;

            Logger::error(ss.str());

            throw e;
        }
    }

    ~Data() {

    }


    //LOAD
    void load(const string& world_folder) {

        const string SERVER_SETTINGS = "server.settings";
        const string PLAYER_FOLDER = "players";
        const string PLAYER_GENERAL = "general";
        const string PLAYER_INVENTORY = "inventory";
        const string PLAYER_MONEY = "money";
        const string OP = "op";


        // XXX load settings

        // OP
        vector<string> op_files = get_files_in_directory(world_folder + "/" + OP);
        for (string& op_file : op_files) {
            string name = op_file.substr(op_file.find_last_of("/") + 1);

            vector<string> lines = Parser::parse_list(op_file);
            for (string& line : lines) {
                if (Commands::all_commands.find(line) != Commands::all_commands.end())
                    this->op_levels.run_with_lock([&](auto& o) { o.insert(line + name); });
                else
                    cout << RED << "Invalid command in op file " << op_file << ": "<< line << RESET << endl;
            }
        }


        // PLAYERS
        vector<string> player_folders = get_files_in_directory(world_folder + "/" + PLAYER_FOLDER);
        for (string& folder : player_folders) {

            // load general info
            unordered_map<string, string> general = Parser::parse_map(folder + "/" + PLAYER_GENERAL);
            string player_id = general["ID"];
            string salted_password = general["SALTED_PASSWORD"];
            string op_level = general["OP_LEVEL"];
            string username = general["USERNAME"];
            string health = general["HEALTH"];
            string placed_blocks = general["PLACED_BLOCKS"];

            shared_ptr<Player> player = make_shared<Player>();
            player->player_id = player_id;
            player->salted_password = salted_password;
            player->op_level = op_level;

            player->username = username;
            player->health = stod(health);
            player->placed_blocks = stoi(placed_blocks);

            // XXX load inventory
            // XXX load money

            // store in data object
            this->player_id_to_salted_password.run_with_lock([&](auto& creds) { creds[player_id] = salted_password; });
            this->player_ids.run_with_lock([&](auto& ids) { ids.push_back(player_id); });
            this->player_id_to_player.run_with_lock([&](auto& players) { players[player_id] = player; });

        }
    }

    vector<string> get_files_in_directory(const string& directory_path) {
        vector<string> files;
        for (const auto& entry : filesystem::directory_iterator(directory_path)) {
            string name = entry.path().filename();
            if (name.substr(name.size() - 3) != ".md") 
                files.push_back(entry.path().string());
        }
        return files;
    }


    //SOCKET
    template <typename T>
    int send(T& body, const string& host, int port) {
        uint8_t buffer[4096];
        uint32_t bits = body.pack(buffer);
        size_t length = (bits + 7) / 8;

        int res = this->udp_socket.run_with_lock([&](auto& sock) {
            return sock.send((char*)buffer, length, host, port);
        });

        return res;
    }

    template <typename T>
    void send_tcp(T& body, SOCKET client_socket) {
        // heap buffer sized for the largest message (puzzle images)
        std::vector<uint8_t> buffer(MAX_TCP_SEND);
        uint32_t bits = body.pack(buffer.data());
        size_t length = (bits + 7) / 8;

        this->tcp_socket.unsafe().send(client_socket, (char*)buffer.data(), length);
    }


    // QUEUE
    void add_client_request(shared_ptr<Request> request)
    {
        this->client_requests.run_with_lock([&](auto& q) {
            q.push(std::move(request));
        });
        this->cv.notify_one(); // notify one thread to wake up
    }

    shared_ptr<Request> pop_client_request()
    {
        shared_ptr<Request> request = nullptr;
        this->client_requests.run_with_lock([&](auto& q) {
            if (q.empty()) return;
            request = std::move(q.front());
            q.pop();
        });
        return request;
    }

    // SECURITY
    string get_id_if_logged_in(const string& ip_address)
    {
        return this->logged_in_ip_address_to_player_id.run_with_read_lock([&](auto& logged_in) {
            auto it = logged_in.find(ip_address);
            return it != logged_in.end() ? it->second : string(SHRUG);
        });
    }

    string str_ip_port(const string& ip_address, int port)
    {
        return ip_address + "::" + to_string(port);
    }

    chrono::steady_clock::time_point check_in(const string& ip_address_type, chrono::steady_clock::time_point now)
    {
        return this->ip_address_type_last_request_time.run_with_lock([&](auto& times) {
            // get last request
            chrono::steady_clock::time_point last_request;
            if(times.find(ip_address_type) != times.end()) {
                last_request = times[ip_address_type];
            }
            else {
                last_request = now - chrono::minutes(10); // 10 minutes ago
            }

            // update the last request time
            times[ip_address_type] = now;

            // wipe map if it's getting too large
            if (times.size() > MAX_IP_LIST_SIZE)
                times.clear();

            return last_request;
        });
    }

    string get_salted_password(const string& player_id)
    {
        return this->player_id_to_salted_password.run_with_lock([&](auto& creds) {
            return creds[player_id];
        });
    }

    void clear_attacker_accounts_if_needed(bool& limit_reached)
    {
        size_t player_count = this->player_id_to_player.run_with_read_lock([&](auto& players) {
            return players.size();
        });

        // if a lot of accounts have been created, try finding accounts that are empty and remove them
        // otherwise stop the creation of new accounts
        if (player_count > Settings::MAX_ACCOUNTS) {
            cout << RED << "POSSIBLE ATTACK DETECTED! max accounts reached " << Settings::MAX_ACCOUNTS << RESET << endl;

            size_t id_count = this->player_ids.run_with_read_lock([&](auto& ids) {
                return ids.size();
            });

            for (int i = 0; i < 10; ++i) {
                int64_t index = CryptoRandom::rand(0, id_count);
                string pid = this->player_ids.run_with_read_lock([&](auto& ids) {
                    return ids[index];
                });
                this->delete_player_if_suspicious(pid);
            }
        }

        limit_reached = this->player_id_to_player.run_with_read_lock([&](auto& players) {
            return players.size() > Settings::MAX_ACCOUNTS;
        });
    }

    chrono::steady_clock::time_point get_last_login_attempt(const string& player_id, chrono::steady_clock::time_point now) {
        return this->last_login_attempt_by_id.run_with_lock([&](auto& attempts) {
            // get last login time
            if (attempts.find(player_id) == attempts.end()) {
                return now - chrono::minutes(10);
            }
            chrono::steady_clock::time_point last = attempts[player_id];

            // set last login time
            attempts[player_id] = now;

            return last;
        });
    }

    bool has_permission(const string& player_id, const string& command) {
        string admin_level = this->player_id_to_player.run_with_read_lock([&](auto& players) {
            auto it = players.find(player_id);
            if (it == players.end() || !it->second) return string();
            return it->second->op_level;
        });

        if (admin_level.empty()) return false;

        return this->op_levels.run_with_read_lock([&](auto& levels) {
            return levels.find(command + admin_level) != levels.end();
        });
    }


    // PLAYERS
    void add_player(const string &player_id, const string &password_hash_salt)
    {
        this->player_ids.run_with_lock([&](auto& ids) {
            ids.push_back(player_id);
        });

        this->player_id_to_player.run_with_lock([&](auto& players) {
            auto player = make_shared<Player>();
            player->player_id = player_id;
            player->salted_password = password_hash_salt;
            players[player_id] = player;
        });

        this->player_id_to_salted_password.run_with_lock([&](auto& creds) {
            creds[player_id] = password_hash_salt;
        });
    }

    void change_password(const string &player_id, const string &password_hash_salt)
    {
        this->player_id_to_player.run_with_lock([&](auto& players) {
            if (players.find(player_id) == players.end()) return;
            players[player_id]->salted_password = password_hash_salt;
        });

        this->player_id_to_salted_password.run_with_lock([&](auto& creds) {
            creds[player_id] = password_hash_salt;
        });
    }

    void delete_player_if_suspicious(string &player_id)
    {
        this->player_id_to_player.run_with_lock([&](auto& players) {
            auto it = players.find(player_id);
            if (it == players.end()) return;
            shared_ptr<Player> player = it->second;
            if (!player) return;
            if (player->placed_blocks >= 100) return;

            players.erase(player_id);
        });

        this->player_id_to_salted_password.run_with_lock([&](auto& creds) {
            creds.erase(player_id);
        });

        this->player_ids.run_with_lock([&](auto& ids) {
            ids.erase(remove(ids.begin(), ids.end(), player_id), ids.end());
        });
    }


};
