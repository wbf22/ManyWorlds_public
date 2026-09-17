



#include "ServerUtil.h"


using namespace std;



void ServerUtil::get_ipaddr_port(sockaddr_in client, string& ipaddr, int& port) {
    // get ip address
    char ipStr[INET6_ADDRSTRLEN];
    if (client.sin_family == AF_INET) {
        // Handle IPv4
        inet_ntop(AF_INET, &((struct sockaddr_in *)&client)->sin_addr, ipStr, sizeof(ipStr));
    } else if (client.sin_family == AF_INET6) {
        // Handle IPv6
        inet_ntop(AF_INET6, &((struct sockaddr_in6 *)&client)->sin6_addr, ipStr, sizeof(ipStr));
    }
    ipaddr = string(ipStr);

    // get port
    port = ntohs(client.sin_port); // Convert network byte order to host byte order
}

void ServerUtil::convert_to_bytes(char* bytes, const int64_t value, int length_in_bytes, int offset) {
    for (int i = offset; i < offset + length_in_bytes; i++) {
        bytes[i] = (value >> (i * 8)) & 0xFF;
    }
}

    
int64_t ServerUtil::time()
{
    auto now = chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    int64_t seconds = chrono::duration_cast<chrono::seconds>(duration).count();
    return seconds;
}
    
int64_t ServerUtil::time_ms()
{
    auto now = chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    int64_t milliseconds = chrono::duration_cast<chrono::milliseconds>(duration).count();
    return milliseconds;
}

int64_t ServerUtil::time_nano() 
{
    auto now = chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    int64_t nano_seconds = chrono::duration_cast<chrono::nanoseconds>(duration).count();
    return nano_seconds;
}

void ServerUtil::set_length_and_type(char *message, int16_t length, int16_t type)
{
    convert_to_bytes(message, length, 2, 0);
    convert_to_bytes(message, type, 2, 2);
}


void ServerUtil::remove_new_lines_and_whitespace(char* buffer, int length) {
    for (int i = 0; i < length; i++) {
        if (isspace(buffer[i])) {
            buffer[i] = 'A';
        }
    }
}

string ServerUtil::to_hex_string(char* buffer, int length) {
    
    stringstream hex_stream;
    // Convert each byte to it's corresponding hex value
    for (int i = 0; i < length; ++i) {
        unsigned char byte = buffer[i];
        hex_stream << ServerUtil::hex_chars[(byte >> 4) & 0x0F] // High 4 bits
                << ServerUtil::hex_chars[byte & 0x0F];       // Low 4 bits
    }

    return hex_stream.str();
}

