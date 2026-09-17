#pragma once

#ifdef _WIN32 // Check if the compilation target is Windows
#pragma comment(lib,"ws2_32.lib")
#define WIN32_LEAN_AND_MEAN
#undef TEXT
#include <winsock2.h>
#include <ws2tcpip.h>
// Windows-specific code here
#else
// Assume Linux if not Windows
#define sprintf_s sprintf
typedef int SOCKET;
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
// Linux-specific code here
#endif

#include <string>
#include <chrono>
#include <sstream>

using namespace std;

struct ServerUtil {
    static constexpr const char hex_chars[] = "0123456789abcdef";


    /**
     * Returns the port and ip given a sockaddr_in object
     */
    static void get_ipaddr_port(sockaddr_in client, string& ipaddr, int& port);

    /**
     * Converts a int64_t into bytes storing in the provided array
     */
    static void convert_to_bytes(char* bytes, const int64_t value, int length_in_bytes = 4, int offset = 0);

    /**
     * Gets the current time since the epoch in seconds
     */
    static int64_t time();


    /**
     * Gets the current time since the epoch in milliseconds
     */
    static int64_t time_ms();

    /**
     * Gets the current time since the epoch in nanoseconds
     */
    static int64_t time_nano();

    /**
     * Sets the first 2 bytes of the array with length, and the next 2 bytes with type
     */
    static void set_length_and_type(char *message, int16_t length, int16_t type);

    /**
     * Removes new lines and whitespace from a char array
     */
    static void remove_new_lines_and_whitespace(char* buffer, int length);

    /**
     * Convert bytes to a hex string
     */
    static string to_hex_string(char* buffer, int length);
};