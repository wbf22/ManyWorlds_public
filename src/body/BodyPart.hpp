#pragma once


#include "../blocks/Block.h"
#include "util/BMaterial.hpp"
#include "animations/Animation.h"
#include "../blocks/MeshMaker.hpp"
#include "util/Util.hpp"
#include <unordered_map>
#include <fstream>
#include <filesystem>

#include <godot_cpp/variant/quaternion.hpp>
#include <godot_cpp/classes/multi_mesh_instance3d.hpp>
#include <godot_cpp/classes/area3d.hpp>



using namespace std;
using namespace godot;


enum class BodyPartType {
    HEAD,
    ARM,
    LEG,
    FLIPPER,
    WING,
    BODY,
    NECK
};

struct BodyPart
{

    unordered_map<string, MultiMeshInstance3D*> block_meshes;
    int mass;

    Quaternion currentRotation; // only used during game play or animation. While editing all limbs have no rotation
    shared_ptr<PositionDouble> start_pos;
    BodyPartType type;
    double length;
    vector<shared_ptr<Block>> blocks;

    shared_ptr<BodyPart> parent_part;
    vector<shared_ptr<BodyPart>> child_parts;

	Node* node;

    unordered_map<AnimationType, shared_ptr<Animation>> animations;

    BodyPart() {}


    BodyPart(vector<shared_ptr<Block>>& blocks, Node* parent_node, unordered_map<double, Ref<Mesh>>& meshes) {
        this->node = parent_node;
             
        for (int i = 0; i < blocks.size(); i++) {
            shared_ptr<Block>& block = blocks[i];
            this->spawn_block(block, meshes[blocks[0]->size]);
        }

    }

    ~BodyPart() = default;

    static void writeToFile(string file_name, vector<shared_ptr<Block>> & blocks) {
        ofstream out(file_name);

        if (out.is_open()) {
            // filesystem::path currentPath = std::filesystem::current_path();
            // string dir = currentPath.c_str();

            for (shared_ptr<Block>& block : blocks) {
                out << "SIZE " << block->size << endl;
                out << "MATERIAL " << block->material << endl;
                out << "POSITION " << block->position->x << " " << block->position->y << " " << block->position->z << endl;
                out << endl;
            }
    
            // Close the file 
            out.close();
        }
        else {
            cerr << "Couldn't open file for saving bodypart: " << file_name << endl;
        }
    }

    vector<shared_ptr<Block>> loadFromFile(string file_name) {
        ifstream inFile(file_name);

    
        string line;
    
        // Read the file line by line
        vector<shared_ptr<Block>> blocks;
        while (getline(inFile, line)) {
            cout << line << endl;
        }
    
        // Close the file
        inFile.close();

        return blocks;
    }

    void animate(int key_frame_index, AnimationType animation) {
        for (pair<string, MultiMeshInstance3D*> mesh : this->block_meshes ) {
            this->animations[animation]->animate(key_frame_index, mesh.second);
        }
    }

    void spawn_block(shared_ptr<Block>& block, Ref<Mesh> selected_mesh) {

        MultiMeshInstance3D* block_mesh;
        shared_ptr<Position> zero = std::make_shared<Position>(0,0,0);
        Vector3 position = CoordinateConversion::convert_position_double_to_fVector(
            block->position_double,
            zero, // fake worldsize and starting pos, since this is just a relative position to the limb
            Util::MAX_INT64_T,
            block->size
        );
        if (this->block_meshes.find(block->material) == this->block_meshes.end()) {

            StaticBody3D* collider = memnew(StaticBody3D);
            collider->set_collision_layer_value(20, true); // Set to only exist on layer 20 (bit index 19, zero-based)
            collider->set_collision_layer(1 << 19); // Disable all other layers
            collider->set_collision_mask(0); // Don't collide with anything
            MultiMeshInstance3D* mm_instance = MeshMaker::spawn_multimesh_instances(
                this->node,
                MeshMaker::get_material(block->material),
                selected_mesh,
                {position},
                collider,
                block->size
            );
            this->block_meshes[block->material] = mm_instance;
        }
        else {
            block_mesh = this->block_meshes[block->material];
            MeshMaker::add_instance_to_multimesh(block_mesh, position);
        }
        this->blocks.push_back(block);
    }

    void delete_block(int instance_index, MultiMeshInstance3D* block_mesh) {
        Ref<MultiMesh> mm = block_mesh->get_multimesh();
        if (mm->get_instance_count() != 0) {
            
            MeshMaker::remove_instance_from_multimesh(
                block_mesh,
                instance_index
            );
            
        }
        else {
            // if it's the last instance delete the mesh
            string material_name = block_mesh->get_material_override().is_valid()? block_mesh->get_material_override()->get_name().utf8().get_data() : "";
            this->block_meshes.erase(material_name);
            block_mesh->queue_free();
        }
    }

    static void parent_to_other(shared_ptr<BodyPart> parent, shared_ptr<BodyPart> child) {
        child->parent_part = parent;
        if (parent != nullptr) {
            parent->child_parts.push_back(child);
        }
    }


    void get_all_blocks_with_start_pos(vector<shared_ptr<Block>>& all_blocks) {
        for(shared_ptr<Block> block : this->blocks) {
            shared_ptr<Block> copy = std::make_shared<Block>();
            copy->material = block->material;
            copy->position_double = std::make_shared<PositionDouble>(
                block->position_double->x + this->start_pos->x,
                block->position_double->y + this->start_pos->y,
                block->position_double->z + this->start_pos->z  
            );
            copy->size = block->size;
            all_blocks.push_back(copy);
        }
    }


    vector<shared_ptr<Block>> get_all_blocks_with_start_pos() {
        vector<shared_ptr<Block>> adjusted;
        this->get_all_blocks_with_start_pos(adjusted);
        return adjusted;
    }
};
