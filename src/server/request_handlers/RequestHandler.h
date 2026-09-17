#pragma once


#include <string>
#include "../Data.hpp"
#include "server/Interface.hpp"
#include "RequestHandlerBase.h"


using namespace std;

template <typename T>
struct RequestHandler : public RequestHandlerBase {

    void handle_request(
        RequestType message_type,
        const int message_length,
        const char* message,
        const string& ip_address,
        const int port,
        Data& data,
        shared_ptr<Request> request,
        unordered_set<RequestType>& tcp_request_types
    ) {
        T body = this->parse_body(message_length, message, ip_address, data);

        // auth: IP binding via TCP handshake
        string player_id;
        bool is_tcp = tcp_request_types.find(message_type) != tcp_request_types.end();
        if (!is_tcp) {
            player_id = data.get_id_if_logged_in(ip_address);
            if (player_id == Data::SHRUG) return;
        }

        this->fulfill_request(body, ip_address, port, data, request, player_id);
    }
    
    virtual void fulfill_request(
        T body,
        const string& ip_address, 
        const int port,
        Data& data,
        shared_ptr<Request> request,
        const string& player_id
    ) = 0; // Making it abstract


    virtual T parse_body(
        const int message_length,
        const char* message,
        const string& ip_address, 
        Data& data
    ) = 0; // Making it abstract


    
    virtual ~RequestHandler() {} // Virtual destructor to ensure proper cleanup
};



