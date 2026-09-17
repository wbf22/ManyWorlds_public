#pragma once




#include <cstdint>
#include <unordered_set>




using namespace std;


struct PuzzleData {
    uint8_t* image = nullptr;
    uint32_t image_size;
    unordered_set<uint8_t> solution;
    uint8_t successful_submissions;


    PuzzleData() {};
    PuzzleData(uint8_t* image, uint32_t image_size, unordered_set<uint8_t> solution) : image(image), image_size(image_size), solution(solution), successful_submissions(0) {}

    
    // Copy constructor
    PuzzleData(const PuzzleData& other) 
        : image_size(other.image_size), solution(other.solution), successful_submissions(other.successful_submissions) {
        if (other.image != nullptr) {
            // Allocate memory and copy the image
            this->image = new uint8_t[other.image_size];
            memcpy(this->image, other.image, other.image_size);
        }
    }

    // Assignment operator
    PuzzleData& operator=(const PuzzleData& other) {
        if (this == &other) return *this; // Handle self-assignment

        // Free existing image
        delete[] this->image;

        // Copy data from other
        this->image_size = other.image_size;
        this->solution = other.solution;
        this->successful_submissions = other.successful_submissions;
        if (other.image != nullptr) {
            // Allocate memory and copy the image
            this->image = new uint8_t[other.image_size];
            memcpy(this->image, other.image, other.image_size);
        } else {
            this->image = nullptr;
        }

        return *this;
    }

    // Destructor
    ~PuzzleData() {
        delete[] image;
    }

};








