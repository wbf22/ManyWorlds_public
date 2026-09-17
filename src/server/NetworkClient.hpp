#pragma once

#include "util/sockets/UDPSocket.hpp"
#include "util/sockets/TCPSocket.hpp"
#include "../server/Interface.hpp"
#include "../server/Interface.hpp"
#include <memory>
#include <vector>
#include <cerrno>




using namespace std;


int MAX_BUFFER_SIZE = 200000; // 200kb for captcha images

/**
 * This folder
 */
struct MANYWORLDS_API NetworkClient {

    UDPSocket udp = UDPSocket(1000);
    bool connected = false;
    string server_ip = "127.0.0.1";
    int server_port = Settings::PORT;
    int server_tcp_port = Settings::PORT + 1;
    vector<pair<int, char*>> unexpected_responses;

    // persistent tcp connection reused across requests until it drops
    unique_ptr<TCPSocket> tcp;
    bool tcp_connected = false;
    vector<char> tcp_rx_buffer;

    NetworkClient() {}

    void ensure_tcp() {
        if (this->tcp_connected) return;
        this->tcp = make_unique<TCPSocket>(1000);
        this->tcp->connect(this->server_ip.c_str(), this->server_tcp_port);
        this->tcp_connected = true;
    }

    static bool is_recv_timeout() {
#ifdef _WIN32
        int err = WSAGetLastError();
        return err == WSAETIMEDOUT || err == WSAEWOULDBLOCK;
#else
        return errno == EAGAIN || errno == EWOULDBLOCK;
#endif
    }


    int ping() {
        try {
            ensure_tcp();
            Logger::trace("-Connected");

            // send ping
            Interface::Ping ping;
            ping.status = 4;

            uint8_t buffer[64];
            uint32_t bits = ping.pack(buffer);
            int length = (bits + 7) / 8;

            Logger::trace("-Sending ping request");
            this->tcp->send(this->tcp->socket_num, (char*)buffer, length);

            // check for server response
            char* response_buffer = new char[MAX_BUFFER_SIZE];
            int bytes_read;
            recieve_tcp(RequestType::PING, response_buffer, bytes_read);

            // deserialize response
            Interface::Ping response;
            if (bytes_read != -1) {
                response = Interface::Ping::unpack((uint8_t*)response_buffer);
            }
            delete[] response_buffer;

            return response.status;
        }
        catch(exception& e) {
            Logger::debug("Ping failed");
            Logger::trace(e.what());
            this->tcp_connected = false;
        }
        
        return -1;
    }

    Interface::Auth login(Interface::Login login) {
        try {
            ensure_tcp();

            // send login request
            uint8_t buffer[512];
            uint32_t bits = login.pack(buffer);
            int length = (bits + 7) / 8;

            Logger::trace("-Sending login request");
            this->tcp->send(this->tcp->socket_num, (char*)buffer, length);

            // check for server response
            char* response_buffer = new char[MAX_BUFFER_SIZE];
            int bytes_read;
            recieve_tcp(RequestType::AUTH, response_buffer, bytes_read);

            // deserialize response
            Interface::Auth response;
            if (bytes_read != -1) {
                response = Interface::Auth::unpack((uint8_t*)response_buffer);
            }
            delete[] response_buffer;

            return response;
        }
        catch(exception& e) {
            Logger::debug("Login failed");
            Logger::trace(e.what());
            this->tcp_connected = false;
            throw;
        }
    }

    Interface::Puzzle puzzle() {
        try {
            ensure_tcp();

            // send puzzle request
            Interface::Puzzle puzzle_request;

            uint8_t buffer[64];
            uint32_t bits = puzzle_request.pack(buffer);
            int length = (bits + 7) / 8;

            Logger::trace("-Sending puzzle request");
            this->tcp->send(this->tcp->socket_num, (char*)buffer, length);

            // check for server response
            char* response_buffer = new char[MAX_BUFFER_SIZE];
            int bytes_read;
            recieve_tcp(RequestType::PUZZLE, response_buffer, bytes_read);

            // deserialize response
            Interface::Puzzle response;
            if (bytes_read != -1) {
                response = Interface::Puzzle::unpack((uint8_t*)response_buffer);
            }
            delete[] response_buffer;

            return response;
        }
        catch(exception& e) {
            Logger::debug("Puzzle failed");
            Logger::trace(e.what());
            this->tcp_connected = false;
            throw;
        }
    }

    void logout(string& player_id) {
        // send stop request
        Interface::Logout logout_request;
        CryptoRandom::uuid_to_bytes(logout_request.uuid.data(), player_id);

        uint8_t buffer[64];
        uint32_t bits = logout_request.pack(buffer);
        int length = (bits + 7) / 8;

        Logger::trace("-Sending logout request");
        udp.send((char*)buffer, length, this->server_ip, this->server_port);
    }

    bool stop(string& player_id) {
        // send stop request
        Interface::Stop stop_request;
        CryptoRandom::uuid_to_bytes(stop_request.player_id.data(), player_id);

        uint8_t buffer[64];
        uint32_t bits = stop_request.pack(buffer);
        int length = (bits + 7) / 8;

        Logger::trace("-Sending stop request");
        udp.send((char*)buffer, length, this->server_ip, this->server_port);

        // check for server ping indicating success
        char* response_buffer = new char[MAX_BUFFER_SIZE];
        int bytes_read = -1;
        recieve_udp(RequestType::PING, response_buffer, bytes_read);
        Interface::Ping response;
        if (bytes_read != -1) {
            response = Interface::Ping::unpack((uint8_t*)response_buffer);
        }
        delete[] response_buffer;

        return response.status == 3;
    }

    bool change_password(string& player_id, string& old_password, string& new_password) {
        // send change password request
        Interface::ChangePassword change_password_request;
        CryptoRandom::uuid_to_bytes(change_password_request.player_id.data(), player_id);
        change_password_request.old_password = old_password;
        change_password_request.new_password = new_password;

        uint8_t buffer[256];
        uint32_t bits = change_password_request.pack(buffer);
        int length = (bits + 7) / 8;

        udp.send((char*)buffer, length, this->server_ip, this->server_port);

        // check for server ping indicating success
        char* response_buffer = new char[MAX_BUFFER_SIZE];
        int bytes_read = -1;
        recieve_udp(RequestType::PING, response_buffer, bytes_read);
        Interface::Ping response;
        if (bytes_read != -1) {
            response = Interface::Ping::unpack((uint8_t*)response_buffer);
        }
        delete[] response_buffer;

        return response.status == 3;
    }


    void recieve_udp(RequestType expected_type, char* buffer, int& bytes_read, bool block = true) {
        // recieve_udp message
        bytes_read = -1;
        while (bytes_read == -1) {
            sockaddr_in server_address = udp.recieve(buffer, MAX_BUFFER_SIZE, bytes_read);

            // check if response is actually from the server ( could still be spoofed, but good to check probably )
            string response_ip;
            int response_port;
            ServerUtil::get_ipaddr_port(server_address, response_ip, response_port);
            if (bytes_read != -1) {
                if (response_ip != this->server_ip || response_port != this->server_port) {
                    Logger::info("Response from unknown server");
                    bytes_read = -1;
                }

                // if wrong type, keep reading
                check_if_right_type(expected_type, buffer, bytes_read);
            }

            if (!block) break;
        }

    }

    void recieve_tcp(RequestType expected_type, char* buffer, int& bytes_read, bool block = true) {
        bytes_read = -1;
        while (bytes_read == -1) {
            // extract a complete message from buffered bytes if we have one
            int msg_len = Interface::message_length(this->tcp_rx_buffer.data(), (int)this->tcp_rx_buffer.size());
            if (msg_len > 0 && (int)this->tcp_rx_buffer.size() >= msg_len) {
                if (msg_len > MAX_BUFFER_SIZE) {
                    Logger::debug("TCP message too large for response buffer");
                    this->tcp_connected = false;
                    break;
                }
                memcpy(buffer, this->tcp_rx_buffer.data(), msg_len);
                this->tcp_rx_buffer.erase(this->tcp_rx_buffer.begin(), this->tcp_rx_buffer.begin() + msg_len);

                RequestType type;
                Interface::get_type(buffer, type);
                if (type != expected_type) {
                    char* response = new char[msg_len];
                    memcpy(response, buffer, msg_len);
                    this->unexpected_responses.push_back({msg_len, response});
                    Logger::debug("Recieved unexpected response TCP. Adding to process later");
                    continue;
                }
                bytes_read = msg_len;
                break;
            }

            // need more bytes from the socket
            char tmp[MAX_BUFFER_SIZE];
            int n = this->tcp->receive(this->tcp->socket_num, tmp, MAX_BUFFER_SIZE);
            if (n == 0) {
                this->tcp_connected = false;
                break;
            }
            if (n < 0) {
                // timeout just means idle; keep waiting for the response
                if (is_recv_timeout()) {
                    if (!block) break;
                    continue;
                }
                this->tcp_connected = false;
                break;
            }
            this->tcp_rx_buffer.insert(this->tcp_rx_buffer.end(), tmp, tmp + n);
            if (!block) break;
        }

    }

    void check_if_right_type(RequestType expected_type, char* buffer, int& bytes_read) {
        if (bytes_read < 2) return;
        RequestType type;
        Interface::get_type(buffer, type);
        if (type != expected_type) {

            // add to unexpected responses to process later
            char* response = new char[bytes_read];
            memcpy(response, buffer, bytes_read);
            this->unexpected_responses.push_back({bytes_read, response});

            Logger::debug("Recieved unexpected response UDP. Adding to process later");
            bytes_read = -1;
        }
    }

};


