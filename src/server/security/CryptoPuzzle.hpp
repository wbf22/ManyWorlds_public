#pragma once


#include <string>
#include "sha256.h"
#include "util/CryptoRandom.hpp"


using namespace std;


/**
 * Class for generating and solving cryptographic puzzles.
 * 
 * This is useful to prevent DDoS attacks on the server. Common use cases
 * may include creating an account or logging in.
 * 
 * The puzzle starts by generating a random challenge string. The client then
 * has to solve the puzzle by finding 
 */
struct CryptoPuzzle {

    SHA256 sha = SHA256();

    string generate_challenge() {
        string buffer(32, '\0');
        CryptoRandom::crypto_random_bytes(buffer.data(), 32);

        return buffer;
    }

    bool verify_solution(string challenge, string nonce, int puzzle_difficulty) {
        string input = challenge + nonce;
        string hash = sha.hashString(input);
        return hasLeadingZeros(hash, puzzle_difficulty);
    }

    bool hasLeadingZeros(string hash, int difficulty) {
        for (int i = 0; i < difficulty; i++) {
            if (hash[i] != '0') return false;
        }
        return true;
    }

    string solvePuzzle(string challenge, int puzzle_difficulty = 6) {
        int nonce = 0;
        while (true) {
            string input = challenge + to_string(nonce);
            string hash = sha.hashString(input);
            if (hasLeadingZeros(hash, puzzle_difficulty)) {
                return to_string(nonce);
            }
            nonce++;
        }
    }

    /**
     * Server
     * - Encrypt with RSA a message
     * - Compute sha hash of the message
     * 
     * Client
     * - Decrypt the message
     * - Compute sha hash of the message
     * 
     */

    /**
     * Server
     * - make a soduku puzzle
     * - send to client
     * - review making sure no cheating and 
     * 
     * Client
     * - Decrypt the message
     * - Compute sha hash of the message
     * 
     */

    /**
     * Server
     * - send challenge hash
     * - wait for nonce
     * 
     * 
     */

};
