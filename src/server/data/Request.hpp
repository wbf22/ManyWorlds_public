#pragma once


#include <string>
#include "util/sockets/TCPSocket.hpp"


using namespace std;

struct Request {
    char* buffer;
    int length;
    sockaddr_in client;
    SOCKET tcp_socket;

    Request(char* buffer, int length, sockaddr_in client, SOCKET tcp_socket) {
        this->buffer = buffer;
        this->length = length;
        this->client = client;
        this->tcp_socket = tcp_socket;
    }

    ~Request() {
        delete[] buffer;
    }


    // Move Constructor
    Request(Request&& other) noexcept : buffer(other.buffer), length(other.length), client(std::move(other.client)), tcp_socket(other.tcp_socket) {
        other.buffer = nullptr;
    }

    // Move Assignement Operator
    Request& operator=(Request&& other) noexcept {
        if (this != &other) {
            delete[] buffer;
            buffer = other.buffer;
            length = other.length;
            client = std::move(other.client);
            tcp_socket = other.tcp_socket;
            other.buffer = nullptr;
        }
        return *this;
    }


    // Disabling assignment operator and copy constructor (inefficient)
    Request& operator=(const Request&) = delete;
    Request(const Request&) = delete;
    
    // // Copy Constructor
    // Request(const Request& other) : ip_address(other.ip_address), port(other.port) {
    //     size_t bufferSize = std::strlen(other.buffer) + 1; // +1 for null terminator
    //     buffer = new char[bufferSize];
    //     std::strcpy(buffer, other.buffer);
    // }

};

