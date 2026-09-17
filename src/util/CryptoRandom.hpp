#pragma once

#include <fstream>
#include <iostream>


// Windows
#ifdef _WIN32
#include <windows.h>
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")
#endif

#include <sstream>
#include <mutex>
#include <cmath>
#include <string>
#include <algorithm>
#include <cstring>


using namespace std;


struct CryptoRandom {

    
    static constexpr const char hex_chars[] = "0123456789abcdef";

    inline static mutex mtx;
    inline static char rand_bytes[65536];
    inline static int offset_index = 65536;


    static void init() {
        #ifdef _WIN32
        BCRYPT_ALG_HANDLE hAlgorithm = NULL;
        NTSTATUS status = BCryptOpenAlgorithmProvider(&hAlgorithm, BCRYPT_RNG_ALGORITHM, NULL, 0);
        if (status != 0) {
            throw runtime_error("Failed to open algorithm provider");
        }

        status = BCryptGenRandom(hAlgorithm, reinterpret_cast<PUCHAR>(CryptoRandom::rand_bytes), sizeof(CryptoRandom::rand_bytes), 0);
        if (status != 0) {
            BCryptCloseAlgorithmProvider(hAlgorithm, 0);
            throw runtime_error("Failed to generate random number");
        }

        BCryptCloseAlgorithmProvider(hAlgorithm, 0);
        #else


        std::ifstream urandom("/dev/urandom", std::ios::in|std::ios::binary);
        if(!urandom) throw runtime_error("Failed to open /dev/urandom");
    

        urandom.read(CryptoRandom::rand_bytes, sizeof(CryptoRandom::rand_bytes));
        if(!urandom) throw runtime_error("Failed to read from /dev/urandom");

        urandom.close();
        #endif
    }

    /**
     * Returns cryptographically secure random bytes on the following 
     * operating systems:
     * 
     * - Windows
     * - Linux
     * - macOS
     */
    static void crypto_random_bytes(char* buffer, int length) {
        lock_guard<mutex> lock(CryptoRandom::mtx);

        if (CryptoRandom::offset_index + length > 65536) {
            CryptoRandom::init();
            CryptoRandom::offset_index = 0;
        }
        memcpy(buffer, CryptoRandom::rand_bytes + CryptoRandom::offset_index, length);
        CryptoRandom::offset_index += length;
    }

    /**
     * Random int which is cryptographically secure
     * 
     * min max (inclusive, exclusive)
     */
    static int64_t rand(int64_t min, int64_t max) {

        int64_t random_number;
        CryptoRandom::crypto_random_bytes(reinterpret_cast<char*>(&random_number), sizeof(random_number));
        random_number = abs(random_number);

        int64_t diff = max - min;

        return random_number % diff + min;
    }

    /**
     * Random bool which is cryptographically secure
     * 
     * returns based on the given probability
     */
    static bool rand_bool(int probability = 50) {

        uint16_t random_number;
        CryptoRandom::crypto_random_bytes(reinterpret_cast<char*>(&random_number), sizeof(random_number));


        return random_number % 100 < probability;
    }

    /**
     * Random int which is cryptographically secure
     * 
     * min max (inclusive, exclusive)
     */
    static uint8_t rand_byte(uint8_t min, uint8_t max) {

        uint8_t random_number;
        CryptoRandom::crypto_random_bytes(reinterpret_cast<char*>(&random_number), sizeof(random_number));

        uint8_t diff = max - min;

        return random_number % diff + min;
    }

    /**
     * Random double which is cryptographically secure
     * 
     * min max (inclusive, exclusive)
     * 
     * NOTE: the max value is 18,446,744,073,709.551616 + min
     */
    static double rand_double(double min, double max) {

        uint64_t random_number;
        CryptoRandom::crypto_random_bytes(reinterpret_cast<char*>(&random_number), sizeof(random_number));

        double diff = max - min;

        double val = (double) random_number / 10000000.0;

        return fmod(val, diff) + min;
    }



    /**
     * Generates a crypographically secure random uuid. Complies to UUID 4 standard.
     * 
     * It's just a 128-bit random number represented as a 32-character hex string.
     * 
     * Looks like this: e885fb15-9bf4-c97f-1c0f-14e413a4549d
     * 
     */
    static string uuid() {
        
        // get random array of bytes
        char id[16]; // 128 bits
        CryptoRandom::crypto_random_bytes(id, 16);

        // Set the version to 4
        id[6] = (id[6] & 0x0F) | 0x40; // clears first 4 bits (& 0x0F) and sets them to '0100'

        // Set the variant to 1 (10xx)
        id[8] = (id[8] & 0x3F) | 0x80; // clears first 2 bits (& 0x3F)  and sets them to '10'

        // convert bytes to hex
        return convert_bytes_to_uuid(id);
    }

    static string convert_bytes_to_uuid(const char* bytes) {
        stringstream hex_stream;
        for (int i = 0; i < 16; ++i) {
            // Convert each byte to 2 hex characters
            unsigned char byte = bytes[i];
            hex_stream << CryptoRandom::hex_chars[(byte >> 4) & 0x0F] // High 4 bits
                    << CryptoRandom::hex_chars[byte & 0x0F];       // Low 4 bits

            // Add dashes
            if (i == 3 || i == 5 || i == 7 || i == 9)
                hex_stream << '-';
        }
        
        return hex_stream.str();
    }

    static void uuid_to_bytes(uint8_t* buffer, const string& uuid) {
        if (uuid.size() != 36 || uuid[8] != '-' || uuid[13] != '-' || uuid[18] != '-' || uuid[23] != '-') {
            throw invalid_argument("Invalid UUID format");
        }

        string hex_str = uuid;
        hex_str.erase(remove(hex_str.begin(), hex_str.end(), '-'), hex_str.end());

        for (size_t i = 0; i < 16; ++i) {
            string byte_str = hex_str.substr(i * 2, 2);
            buffer[i] = static_cast<uint8_t>(std::stoul(byte_str, nullptr, 16));
        }
    }

    static string ints_to_uuid(int64_t high, int64_t low) {
        char id[16];
        memcpy(id, &high, 8);
        memcpy(id + 8, &low, 8);

        id[6] = (id[6] & 0x0F) | 0x40;
        id[8] = (id[8] & 0x3F) | 0x80;

        return convert_bytes_to_uuid(id);
    }

    static int hex_to_int(char c) {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        throw runtime_error("Invalid hex character");
    }



};