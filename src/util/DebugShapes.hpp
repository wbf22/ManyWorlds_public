#pragma once



using namespace std;
using namespace godot;


#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/immediate_mesh.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>

#include "../blocks/MeshMaker.hpp"
#include "GodotUtil.hpp"

struct DebugShapes {

    static void draw_debug_line(Ref<ImmediateMesh> mesh, Vector3 start, Vector3 end) {
        mesh->clear_surfaces();
        mesh->surface_begin(Mesh::PRIMITIVE_LINES);
        mesh->surface_set_color(Color(1,0,0));
        mesh->surface_add_vertex(start);
        mesh->surface_add_vertex(end);
        mesh->surface_end();
    }


    static void spawn_debug_block_at(Vector3 pos) {

		Ref<Mesh> mesh = MeshMaker::make_cube(1, "assets/textures/SAND.png", 1);
		MeshInstance3D* mesh_instance = memnew(MeshInstance3D);
		mesh_instance->set_mesh(mesh);
		GodotUtil::get_root_node()->add_child(mesh_instance);
		mesh_instance->set_global_position(pos);
    }


    inline static MeshInstance3D* debug_box;
    static void draw_debug_box(Vector3 start, Vector3 end, Color color = Color(0.5, 0.5, 0.5)) {


        // create other corners
        Vector3 start_left = Vector3(start.x, start.y, end.z);
        Vector3 start_right = Vector3(end.x, start.y, start.z);
        Vector3 start_opposite = Vector3(end.x, start.y, end.z);
        Vector3 end_left = Vector3(start.x, end.y, end.z);
        Vector3 end_right = Vector3(end.x, end.y, start.z);
        Vector3 end_opposite = Vector3(start.x, end.y, start.z);


        // draw box edges
        SceneTree* tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
        if (tree) {

            if (DebugShapes::debug_box == nullptr) {
                Node* scene_node = tree->get_current_scene();
                Ref<ImmediateMesh> debug_line;
                debug_line.instantiate();
                DebugShapes::debug_box = memnew(MeshInstance3D);
                DebugShapes::debug_box->set_mesh(debug_line);
                scene_node->add_child(DebugShapes::debug_box);
            }

            Ref<ImmediateMesh> debug_line = DebugShapes::debug_box->get_mesh();
            debug_line->clear_surfaces();
            debug_line->surface_begin(Mesh::PRIMITIVE_LINES);

            // bottom square
            debug_line->surface_add_vertex(start);
            debug_line->surface_add_vertex(start_left);

            debug_line->surface_add_vertex(start_left);
            debug_line->surface_add_vertex(start_opposite);

            debug_line->surface_add_vertex(start_opposite);
            debug_line->surface_add_vertex(start_right);

            debug_line->surface_add_vertex(start_right);
            debug_line->surface_add_vertex(start);

            // sides
            debug_line->surface_add_vertex(start);
            debug_line->surface_add_vertex(end_opposite);

            debug_line->surface_add_vertex(start_left);
            debug_line->surface_add_vertex(end_left);

            debug_line->surface_add_vertex(start_right);
            debug_line->surface_add_vertex(end_right);

            debug_line->surface_add_vertex(end);
            debug_line->surface_add_vertex(start_opposite);

            // top
            debug_line->surface_add_vertex(end);
            debug_line->surface_add_vertex(end_left);

            debug_line->surface_add_vertex(end_left);
            debug_line->surface_add_vertex(end_opposite);

            debug_line->surface_add_vertex(end_opposite);
            debug_line->surface_add_vertex(end_right);

            debug_line->surface_add_vertex(end_right);
            debug_line->surface_add_vertex(end);

            debug_line->surface_end();


            Ref<StandardMaterial3D> material;
            material.instantiate();
            material->set_shading_mode(BaseMaterial3D::SHADING_MODE_UNSHADED);
            material->set_albedo(color); 
            DebugShapes::debug_box->set_material_override(material);



        }
        else {
            cout << "DebugLine: couldn't get root node of scene. Can't draw line" << endl;
        }

    }


    static void remove_debug_box() {
        DebugShapes::debug_box->queue_free();
        DebugShapes::debug_box = nullptr;
    }

};


struct DebugLine {

    inline static vector<MeshInstance3D*> meshes;




    static void draw_line(Vector3 start, Vector3 end, Color color = Color(0,1,0), bool clear_others = false) {


        SceneTree* tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
        if (tree) {

            Node* scene_node = tree->get_current_scene();

            Ref<ImmediateMesh> debug_line;
            debug_line.instantiate();
            MeshInstance3D* instance = memnew(MeshInstance3D);
            instance->set_mesh(debug_line);
            scene_node->add_child(instance);

            debug_line->clear_surfaces();
            debug_line->surface_begin(Mesh::PRIMITIVE_LINES);
            debug_line->surface_add_vertex(start);
            debug_line->surface_add_vertex(end);
            debug_line->surface_end();


            Ref<StandardMaterial3D> material;
            material.instantiate();
            material->set_shading_mode(BaseMaterial3D::SHADING_MODE_UNSHADED);
            material->set_albedo(color); 
            instance->set_material_override(material);

            if (clear_others) {
                DebugLine::clear_lines();
            }

            // int64_t time = chrono::system_clock::to_time_t(
            //     chrono::system_clock::now()
            // );
            DebugLine::meshes.push_back(instance);


        }
        else {
            cout << "DebugLine: couldn't get root node of scene. Can't draw line" << endl;
        }

    }


    static void clear_lines() {

        for (MeshInstance3D* instance : meshes) {
            instance->queue_free();
        }
        meshes.clear();

    }


};