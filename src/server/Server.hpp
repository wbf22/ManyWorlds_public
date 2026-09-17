#pragma once

#include <string>
#include <thread>
#include <vector>
#include <iostream>
#include <atomic>
#include <cerrno>
#include "util/sockets/UDPSocket.hpp"
#include <unordered_set>
#include "security/Security.h"
#include <queue>
#include <mutex>
#include "data/Request.hpp"
#include "Data.hpp"
#include "request_handlers/Ping.hpp"
#include "request_handlers/Login.hpp"
#include "request_handlers/Logout.hpp"
#include "request_handlers/Puzzle.hpp"
#include "request_handlers/Register.hpp"
#include "request_handlers/Stop.hpp"
#include "request_handlers/RequestHandler.h"
#include "server/ServerUtil.h"
#include "util/sockets/WindowsInit.hpp"
#include "../server/Interface.hpp"
#include "Settings.hpp"
#include <typeinfo>
#include "util/Logger.hpp"







using namespace std;

/**
 * Main server class that handles all incoming requests. This will be used either locally 
 * or on a remote server.
 * 
 * Certain requests that have to do with login and registration are handled over tcp. Otherwise,
 * requests are handled over udp and require that the client's ip address has completed login.
 * The TCP handshake provides anti-spoofing for auth; UDP requests are verified by IP binding.
 */
struct Server
{

    static constexpr int MAX_MESSAGE_SIZE = 1024;
    static constexpr int MAX_TCP_BUFFER = 512 * 1024; // cap for a single tcp message to prevent memory floods

    Data data;
    vector<thread> worker_threads;
    Security security;
    atomic<int> active_tcp_connections = 0;


    // request handlers
    unordered_map<RequestType, RequestHandlerBase*> request_handlers = {
        {RequestType::PING, new Ping()},
        {RequestType::REGISTER, new Register(Settings::NUM_CAPTCHA_PUZZLES, Settings::CAPTCHA_DIFFICULTY)},
        {RequestType::LOGIN, new Login(Settings::NUM_CAPTCHA_PUZZLES, Settings::CAPTCHA_DIFFICULTY)},
        {RequestType::LOGOUT, new Logout()},
        {RequestType::PUZZLE, new Puzzle()},
        {RequestType::STOP, new Stop()},
    };

    /**
     * Only these request are handled over tcp, the rest are udp
     * 
     * Doing these requests on TCP allows us to verify the client ip_address. This helps
     * in rate limiting these potentially dangerous requests. All the other requests are over
     * UDP and require the client's ip to be logged in.
     */
    unordered_set<RequestType> tcp_request_types = {
        RequestType::PING,
        RequestType::REGISTER,
        RequestType::LOGIN,
        RequestType::PUZZLE
    };

    Server(int port, int timeout_ms, int worker_threads)
        : data(port, timeout_ms)
    {
        this->data.running = true;

        // set up worker threads
        for (int i = 0; i < worker_threads; i++)
        {
            thread t([this]() {
                Logger::trace("Worker thread started");
                while(this->data.running) {
                    this->handle_request();
                }
                Logger::info("Worker thread shutting down");
            });

            this->worker_threads.push_back(std::move(t));
        }
    }

    ~Server()
    {
        this->data.running = false;
        for (auto& t : worker_threads) {
            if (t.joinable()) {
                t.join();
            }
        }
        // connection threads are detached; wait for them to unwind before freeing data.
        // each is blocked in recv for at most the socket timeout, then sees running==false.
        while (this->active_tcp_connections > 0) {
            this_thread::sleep_for(chrono::milliseconds(10));
        }
    }



    // MAIN SERVER LOOP
    void run()
    {
        Logger::info("Started ManyWorlds server");
        WindowsInit::init();

        // XXX load settings

        // XXX load data files from disk
        this->data.load(Settings::WORLD_FOLDER);


        // XXX spawn worker thread to save server data to disk every ~5 minutes

        // AUTH SERVER LOOP (TCP on seperate thread)
        thread tcp_thread([this]() {
            this->tcp_thread();
        });


        // MAIN SERVER LOOP
        char* message = new char[MAX_MESSAGE_SIZE];
        while(this->data.running) {
            // accept incoming connections
            int bytes_read = -1;

            sockaddr_in client = this->data.udp_socket.run_with_lock([&](auto& sock) {
                return sock.recieve(message, MAX_MESSAGE_SIZE, bytes_read);
            });
            if (bytes_read == -1) continue; // loop if nothing recieved

            // give to worker threads
            this->data.add_client_request(make_shared<Request>(message, bytes_read, client, -1));
            message = new char[MAX_MESSAGE_SIZE]; // fresh buffer for next iteration
        }
        delete[] message;


        Logger::info("Server shutting down");
        tcp_thread.join();
        Logger::info("stopped");
    }

    // TCP THREAD METHOD
    void tcp_thread() {
        Logger::trace("TCP thread started");

        while(this->data.running) {
            try {
                // accept incoming connections
                sockaddr_in client;
                SOCKET client_socket = this->data.tcp_socket.run_with_lock([&](auto& sock) {
                    return sock.acceptConnection(client);
                });
                Logger::trace("TCP connect socket: " + to_string(client_socket));
                if (client_socket == -1) continue; // loop if nothing recieved

                // at this point we know they aren't using a fake ip address, as we've completed the TCP handshake
                // with UDP we don't have that guarantee, so we require login for requests to be processed on UDP

                // one thread per connection, kept open until the client disconnects
                this->active_tcp_connections++;
                thread t([this, client_socket, client]() {
                    try {
                        this->handle_connection(client_socket, client);
                    }
                    catch (...) {
                        Logger::debug("Error in tcp connection thread");
                    }
                    this->active_tcp_connections--;
                });
                t.detach();
            }
            catch(exception& e) {
                // debug so hackers can slow us down with weird junk
                Logger::debug("Error in tcp thread");
                Logger::debug(e.what());
            }
        }

        Logger::info("TCP thread shutting down");
    }

    // WORKER THREAD METHOD
    void handle_request()
    {
        
        // get request
        shared_ptr<Request> request = this->data.pop_client_request();

        if (request == nullptr) return;

        Logger::trace(
            "Backlog: " + to_string(this->data.client_requests.run_with_read_lock([](auto& q) { return q.size(); }))
        );

        // PROCESS UDP REQUEST
        try {

            string ip_address;
            int port;
            ServerUtil::get_ipaddr_port(request->client, ip_address, port);

            // get message type
            if (request->length < 2) return;
            RequestType message_type;
            Interface::get_type(request->buffer, message_type);
            Logger::trace(
                "UDP: " + to_string(static_cast<int>(message_type))
            );

            // rate limit
            if (security.rate_limit(ip_address, message_type, this->data)) return;

            // check security
            if(security.check_client(ip_address, this->data)) return;

            // LOGIN and REGISTER requests should be handled on TCP connections (as well as some other requests)
            if (this->tcp_request_types.find(message_type) != this->tcp_request_types.end()) return; // these requests should be handled on TCP connections
            

            // handle request
                this->request_handlers[message_type]->handle_request(
                    message_type,
                    request->length,
                    request->buffer,
                    ip_address, 
                    port, 
                    this->data,
                    request,
                    this->tcp_request_types
                );
        }
        catch (exception& e) {
            // only logging in debug so attackers can't slow us down with weird junk
            Logger::debug("Error handling request");
            Logger::debug(e.what());
        }
        
    }

    // PER-CONNECTION LOOP: keep reading messages until the client disconnects
    void handle_connection(SOCKET client_socket, sockaddr_in client) {
        Logger::trace("TCP connect socket: " + to_string(client_socket));

        try {
            string ip_address;
            int port;
            ServerUtil::get_ipaddr_port(client, ip_address, port);

            vector<char> buffer;
            buffer.reserve(MAX_MESSAGE_SIZE);

            while (this->data.running) {
                int msg_len = Interface::message_length(buffer.data(), (int)buffer.size());
                if (msg_len < 0) {
                    Logger::debug("TCP malformed message from " + ip_address);
                    break;
                }
                if (msg_len > 0 && (int)buffer.size() >= msg_len) {
                    this->process_tcp_message(buffer.data(), msg_len, ip_address, port, client_socket);
                    buffer.erase(buffer.begin(), buffer.begin() + msg_len);
                    continue;
                }
                if (msg_len > MAX_TCP_BUFFER) {
                    Logger::debug("TCP message too large from " + ip_address);
                    break;
                }
                if ((int)buffer.size() > MAX_TCP_BUFFER) {
                    Logger::debug("TCP buffer overflow from " + ip_address);
                    break;
                }

                // not enough buffered yet, pull more from the socket
                char tmp[MAX_MESSAGE_SIZE];
                int n = this->data.tcp_socket.unsafe().receive(client_socket, tmp, MAX_MESSAGE_SIZE);
                if (n == 0) break; // client closed
                if (n < 0) {
                    // timeout just means idle; keep the connection alive
                    if (is_recv_timeout()) continue;
                    break;
                }
                buffer.insert(buffer.end(), tmp, tmp + n);
            }
        }
        catch (exception& e) {
            Logger::debug("Error in tcp connection");
            Logger::debug(e.what());
        }

        Logger::trace("TCP disconnect socket: " + to_string(client_socket));
        try {
            data.tcp_socket.unsafe().close_connection(client_socket);
        }
        catch (...) {
            // Catch all exceptions and do nothing
        }
    }

    // PROCESS A SINGLE TCP MESSAGE
    void process_tcp_message(const char* buffer, int length, string& ip_address, int port, SOCKET client_socket) {
        try {
            // get message type
            RequestType message_type;
            Interface::get_type(buffer, message_type);
            Logger::trace("TCP: " + to_string(static_cast<int>(message_type)));

            // rate limit
            if (security.rate_limit(ip_address, message_type, this->data)) return;

            // only handle correct request types on TCP
            if (this->tcp_request_types.find(message_type) != this->tcp_request_types.end()) {
                auto request = make_shared<Request>(nullptr, -1, sockaddr_in{}, client_socket);
                this->request_handlers[message_type]->handle_request(
                    message_type,
                    length,
                    buffer,
                    ip_address,
                    port,
                    this->data,
                    request,
                    this->tcp_request_types
                );
            }
        }
        catch (exception& e) {
            // don't log anything here unless in debug as attacks may try to send it weird junk to slow us down
            Logger::debug("Error handling tcp request");
            Logger::debug(e.what());
        }
    }

    static bool is_recv_timeout() {
#ifdef _WIN32
        int err = WSAGetLastError();
        return err == WSAETIMEDOUT || err == WSAEWOULDBLOCK;
#else
        return errno == EAGAIN || errno == EWOULDBLOCK;
#endif
    }


};

