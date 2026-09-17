#pragma once


#include "../Server.hpp"
#include <string>
#include <cstdint>
#include "RequestHandler.h"
#include "../Data.hpp"
#include "server/Interface.hpp"
#include "../security/captcha/PuzzleData.hpp"



using namespace std;


struct Puzzle : public RequestHandler<Interface::Puzzle> {

    ~Puzzle() override {}
    
    Interface::Puzzle parse_body(
        const int message_length,
        const char* message,
        const string& ip_address, 
        Data& data
    ) override {
        return Interface::Puzzle();
    }
    
    void fulfill_request(
        Interface::Puzzle body,
        const string& ip_address, 
        const int port,
        Data& data,
        shared_ptr<Request> request,
        const string& player_id
    ) override 
    {
        
        // get a puzzle
        Interface::Puzzle response;
        uint64_t puzzle_id;
        uint16_t square;
        uint8_t* image = data.puzzle_pool.run_with_lock([&](auto& pool) {
            auto result = pool.get_puzzle(puzzle_id, square);
            return result.image;
        });
        int image_size = square * square * 3;

        // copy array into vector
        for (int i = 0; i < image_size; i++) {
            response.image.push_back(image[i]);
        }

        // set puzzle id
        response.puzzle_id = puzzle_id;

        // send back response over tcp
        data.send_tcp(response, request->tcp_socket);
    }

};

