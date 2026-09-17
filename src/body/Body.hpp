#pragma once

#include "BodyPart.hpp"
#include "util/Random.h"
#include "util/Util.hpp"
#include "animations/Animation.h"
#include "util/BMaterial.hpp"



using namespace std;
using namespace godot;



struct Body
{


	// body
    vector<shared_ptr<BodyPart>> parts;

    double weight_kg;

    
    Body(){}

    Body(Node* parent_node, unordered_map<double, Ref<Mesh>>& meshes){

        // make body
        vector<shared_ptr<Block>> blocks;

        // left leg
        blocks.push_back(
            std::make_shared<Block>(
                BMaterial::ANDESTITE,
                std::make_shared<Position>(0,0,0),
                std::make_shared<PositionDouble>(0,0,0),
                0.25
            )  
        );            
        // blocks.push_back(
        //     BodyBlock(
        //         BMaterial::ANDESTITE,
        //         Vector3(0,0,-6.25),
        //         0.25
        //     )  
        // );
        // blocks.push_back(
        //     BodyBlock(
        //         BMaterial::ANDESTITE,
        //         Vector3(0,0,-12.5),
        //         0.25
        //     )  
        // );
        // blocks.push_back(
        //     BodyBlock(
        //         BMaterial::ANDESTITE,
        //         Vector3(0,0,-18.75),
        //         0.25
        //     )  
        // );
        BodyPart::writeToFile("default_leg", blocks);
        Transform3D offset = Transform3D(
            Quaternion(0,0,0,1), // zero rotation
            Vector3(0, 50, 0) + Vector3(0, 0, -25)
        );
        shared_ptr<BodyPart> left_leg = std::make_shared<BodyPart>(blocks, parent_node, meshes);
        // Quaternion(axis: Vector3, angle: float)
        Quaternion rot_1 = Quaternion::from_euler(Vector3(0, 0, 0));
        Quaternion rot_2 = Quaternion::from_euler(Vector3(23, 0, 0));
        Quaternion rot_3 = Quaternion::from_euler(Vector3(45, 0, 0));
        Quaternion rot_4 = Quaternion::from_euler(Vector3(23, 0, 0));
        Quaternion rot_5 = Quaternion::from_euler(Vector3(0, 0, 0));
        Quaternion rot_6 = Quaternion::from_euler(Vector3(-23, 0, 0));
        Quaternion rot_7 = Quaternion::from_euler(Vector3(-45, 0, 0));
        Quaternion rot_8 = Quaternion::from_euler(Vector3(-23, 0, 0));
        shared_ptr<Animation> animation = std::make_shared<Animation>();
        animation->defined_key_frames.push_back( std::make_shared<KeyFrame>(Vector3(0,0,0), rot_1, 0.0) );
        animation->defined_key_frames.push_back( std::make_shared<KeyFrame>(Vector3(0,0,0), rot_2, 0.2) );
        animation->defined_key_frames.push_back( std::make_shared<KeyFrame>(Vector3(0,0,0), rot_3, 0.4) );
        animation->defined_key_frames.push_back( std::make_shared<KeyFrame>(Vector3(0,0,0), rot_4, 0.6) );
        animation->defined_key_frames.push_back( std::make_shared<KeyFrame>(Vector3(0,0,0), rot_5, 0.8) );
        animation->defined_key_frames.push_back( std::make_shared<KeyFrame>(Vector3(0,0,0), rot_6, 1.0) );
        animation->defined_key_frames.push_back( std::make_shared<KeyFrame>(Vector3(0,0,0), rot_7, 1.2) );
        animation->defined_key_frames.push_back( std::make_shared<KeyFrame>(Vector3(0,0,0), rot_8, 1.4) );
        animation->length = 1.6;
        animation->pre_render(ANIMATION_DELTA);

        left_leg->animations[AnimationType::RUN] = animation;
        left_leg->animations[AnimationType::IDLE] = animation;

        this->parts.push_back(
            left_leg
        );
        
    }



    inline static int ANIMATION_FPS = 30;
    inline static float ANIMATION_DELTA = 1.0f / ANIMATION_FPS;
    double last_time_animated = Random::randDouble(Util::seed(), 0, 0, ANIMATION_DELTA); // get random last animation call to avoid entities being animated all at the same time
    double animation_elapsed = 0;
    AnimationType currentAnimation = AnimationType::IDLE;
    void animate(float delta_time, AnimationType animation) {
        

        // increment time or reset if new animation is starting
        if (animation != currentAnimation) {
            currentAnimation = animation;
            this->animation_elapsed = 0;
            this->last_time_animated = 0;
        }
        else {
            this->animation_elapsed += delta_time;
        }

        // animate every ANIMATION_DELTA seconds
        if ( (this->animation_elapsed - this->last_time_animated) > ANIMATION_DELTA) {
            this->last_time_animated = this->animation_elapsed;

            int index = this->animation_elapsed / ANIMATION_DELTA;

            // animate each part
            for (auto part : this->parts) {
                part->animate(index, animation);
            }
            
        }
    
    
    }


    void reset_animations() {
        this->animation_elapsed = 0;
        this->last_time_animated = -ANIMATION_DELTA + -1;
        this->animate(0, this->currentAnimation);
    }

    vector<shared_ptr<Block>> get_all_blocks() {
        vector<shared_ptr<Block>> all_blocks;
        for(shared_ptr<BodyPart> part : this->parts) {
            part->get_all_blocks_with_start_pos(all_blocks);
        }
        return all_blocks;
    }

    void spawn_block(
        MultiMeshInstance3D* target_mesh, 
        Ref<Mesh> selected_mesh, 
        Ref<Material> material, 
        shared_ptr<PositionDouble> position, 
        double size, 
        shared_ptr<Planet> planet,
        shared_ptr<Position> terrain_zero_zero
    ){
        
        
        for(shared_ptr<BodyPart>& part : parts) {

            bool has_mesh = false;
            // cout << "target " << TCHAR_TO_UTF8(*target_mesh->GetName()) << endl;
            cout << "num meshes " << part->block_meshes.size() << endl;
            for (pair<string, MultiMeshInstance3D*> mesh : part->block_meshes ) {
                if (mesh.second == target_mesh) has_mesh = true;
                // cout << "in mesh " << TCHAR_TO_UTF8(*mesh.second->GetName()) << endl;
            }
            cout << "has_mesh " << to_string(has_mesh) << endl;

            if (has_mesh) {

                // part->spawn_block()

                /*
                    - adding a new bone to make a new part

                
                */

                // get vector position and get offset from mesh root. Then convert back to position double without wrapping or anything (since we need to preserve negative offsets)
                Vector3 position_vec = CoordinateConversion::convert_position_double_to_fVector(
                    position,
                    terrain_zero_zero,
                    planet->worldSize,
                    size
                );
                StaticBody3D* collider = Object::cast_to<StaticBody3D>(target_mesh->get_child(0));
                Vector3 relative_block_pos = position_vec - collider->get_global_position();
                shared_ptr<PositionDouble> relative_position = CoordinateConversion::convert(
                    relative_block_pos, 
                    terrain_zero_zero, 
                    size
                );

                // make the block and add it
                shared_ptr<Block> block = std::make_shared<Block>();
                block->material = GodotUtil::c_str(material->get_name());
                block->position_double= relative_position;
                block->size = size;
                part->spawn_block(block, selected_mesh);

                cout << endl << "added and spawned block to part" << endl << endl;

                return;
            }
        }

        cout << endl << "No part found" << endl << endl;
    }

    void delete_block(MultiMeshInstance3D* target_mesh, int instance_index) {


        for(shared_ptr<BodyPart>& part : parts) {

            bool has_mesh = false;
            for (pair<string, MultiMeshInstance3D*> mesh : part->block_meshes ) {
                if (mesh.second == target_mesh) has_mesh = true;
            }
            if (has_mesh) {

                part->delete_block(instance_index, target_mesh);

                cout << endl << "deleted block on part" << endl << endl;

                return;
            }
        }

        cout << endl << "No part found" << endl << endl;
    }


};

