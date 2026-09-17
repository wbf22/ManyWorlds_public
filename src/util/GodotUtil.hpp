#pragma once

#include <sstream>
#include <chrono>

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/immediate_mesh.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/physics_direct_space_state3d.hpp>
#include <godot_cpp/classes/physics_ray_query_parameters3d.hpp>
#include <godot_cpp/classes/world3d.hpp>


using namespace std;
using namespace godot;

struct GodotUtil {

    inline static Vector3 forward_v = Vector3(0, 0, -1);
    inline static Vector3 right_v = Vector3(1, 0, 0);
    inline static Vector3 up_v = Vector3(0, 1, 0);

    // Forward vector (world space)
    static Vector3 forward(Node3D* node) {
        /*
        From Godot source:

	constexpr Basis(const Vector3 &p_x_axis, const Vector3 &p_y_axis, const Vector3 &p_z_axis) :
			rows{
				{ p_x_axis.x, p_y_axis.x, p_z_axis.x },
				{ p_x_axis.y, p_y_axis.y, p_z_axis.y },
				{ p_x_axis.z, p_y_axis.z, p_z_axis.z },
			} {}


        Evidently Godot switches up the vectors to make multiplications simple
        */

        Basis basis = node->get_global_transform().basis;
        // return Vector3(-basis[0].z, -basis[1].z, -basis[2].z);
        return -basis.get_column(2);
    }

    
    // Right vector (world space)
    static Vector3 right(Node3D* node) {
        Basis basis = node->get_global_transform().basis;
        // return Vector3(basis[0].x, basis[1].x, basis[2].x);
        return basis.get_column(0);
    }

    // Up vector (world space)
    static Vector3 up(Node3D* node) {
        Basis basis = node->get_global_transform().basis;
        // return Vector3(basis[0].y, basis[1].y, basis[2].y);
        return basis.get_column(1);
    }

    static Vector3 safe_normalization(Vector3 &vec) {
		if (!vec.is_equal_approx(Vector3(0,0,0))) {
			vec = vec.normalized();
		}
        return vec;
    }


    static Node* get_root_node() {

        SceneTree* tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
        if (tree) {
            return tree->get_current_scene();
        }
        return nullptr;
    }

    static string vec_str(Vector3 vec) {
        stringstream ss;
        ss << "Vector(" << vec.x << ", " << vec.y  << ", " << vec.z << ")" << endl;
        
        return ss.str();
    }


    static Dictionary raycast(Vector3 start, Vector3 end, Array nodes_to_exclude, uint32_t mask = 0xFFFFFFFF) {


        // perform raycast
        Node* scene_root = GodotUtil::get_root_node()->get_child(0); // first child of root
        Node3D* root3d = Object::cast_to<Node3D>(scene_root);
        Ref<World3D> world = root3d->get_world_3d();
        PhysicsDirectSpaceState3D *space = world->get_direct_space_state();
        Ref<PhysicsRayQueryParameters3D> params;
        params.instantiate();
        params->set_from(start);
        params->set_to(end);
        params->set_exclude(nodes_to_exclude);

        if (mask != 0xFFFFFFFF) {
            params->set_collision_mask(mask);
        }

        return space->intersect_ray(params);
    }

    template<typename Container>
    static Array to_godot_array(const Container& container) {
        Array arr;
        for (const auto& item : container) {
            arr.push_back(item);
        }
        return arr;
    }


    static String g_str(string c_str) {
        return String(c_str.c_str());
    }

    static string c_str(String g_str) {
        return string(g_str.utf8().get_data());
    }


};

