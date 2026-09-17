#pragma once

#include <string>
#include "../Data.hpp"
#include "server/Interface.hpp"

using namespace std;

struct RequestHandlerBase {
    virtual void handle_request(
        RequestType message_type,
        const int message_length,
        const char* message,
        const string& ip_address, 
        const int port,
        Data& data,
        shared_ptr<Request> request,
        unordered_set<RequestType>& tcp_request_types
    ) = 0;
    virtual ~RequestHandlerBase() {} // Virtual destructor to ensure proper cleanup
};