// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include <string>
#include "util/PositionDouble.h"
#include "../blocks/Block.h"
#include "Rot.h"



using namespace std;


struct LeafTransform : public Block {

    LeafTransform(shared_ptr<PositionDouble> position_double, shared_ptr<Rot> pointing_direction, double scale) {
        this->position_double = position_double;
        this->pointing_direction = pointing_direction;
        this->scale = scale;
    }

    LeafTransform() {}

    shared_ptr<Rot> pointing_direction;
    double scale;
};


class Leaf
{
public:
	Leaf() {}
	~Leaf() {}

    vector<shared_ptr<Block>> blocks;
    
    string material;
    bool no_up_down_rotation;
    double length;

    vector<shared_ptr<LeafTransform>> transforms;


    /** */
    vector<shared_ptr<Block>> apply_transform(shared_ptr<LeafTransform>& transform) {
        
        // scale blocks (combining blocks that get the same position)
        vector<shared_ptr<Block>> t_blocks;
        unordered_set<string> block_tags;
        for (shared_ptr<Block> block : this->blocks) {
            shared_ptr<Block> t_block = std::make_shared<Block>();
            t_block->position_double = std::make_shared<PositionDouble>();
            t_block->position_double->x = block->position_double->x * transform->scale;
            t_block->position_double->y = block->position_double->y * transform->scale;
            t_block->position_double->z = block->position_double->z * transform->scale;
            t_block->material = block->material;
            t_block->size = block->size;

            string tag = t_block->stringTag(t_block->size, t_block->position_double->x, t_block->position_double->y, t_block->position_double->z);
            if (block_tags.find(tag) == block_tags.end()) {
                block_tags.insert(tag);
                t_blocks.push_back(t_block);
            }
        }
        block_tags.clear();


        // rotate blocks
        vector<shared_ptr<Block>> final_blocks;
        for (shared_ptr<Block> t_block : t_blocks) {
            t_block->position_double = Rot::rotate(t_block->position_double, transform->pointing_direction);
            t_block->position = std::make_shared<Position>();
            t_block->position->x = t_block->position_double->x + transform->position_double->x;
            t_block->position->y = t_block->position_double->y + transform->position_double->y;
            t_block->position->z = t_block->position_double->z + transform->position_double->z;
            t_block->position_double = t_block->position->to_position_double();

            string tag = t_block->stringTag();
            if (block_tags.find(tag) == block_tags.end()) {
                block_tags.insert(tag);
                final_blocks.push_back(t_block);
            }
        }
        
        return final_blocks;
    }

};

