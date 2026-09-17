#pragma once


#include <cstring>
#include <godot_cpp/classes/world_environment.hpp>
#include <godot_cpp/classes/environment.hpp>
#include <godot_cpp/classes/panorama_sky_material.hpp>
#include <godot_cpp/classes/sky.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/memory.hpp>

#include "space/Planet.h"
#include "util/StellarCoordinate.hpp"
#include "util/GodotUtil.hpp"


using namespace std;
using namespace godot;


struct GodotHdr {


    /**
     * Sets the in-game HDR skybox from raw RGBA float data.
     * Takes ownership of the float array and frees it via delete[] after copying
     * into the Godot texture. The data must be width * height * 4 floats
     * (RGBA channels, 4 bytes per float). Matching SkyGen output.
     * `parent` is any node in the scene so we can attach a WorldEnvironment.
     */
    static void set_hdr_skybox(Node* parent, float* data, int width, int height) {
        PackedByteArray pba;
        pba.resize(width * height * 4 * sizeof(float));
        memcpy(pba.ptrw(), data, pba.size());
        delete[] data;

        Ref<Image> img;
        img.instantiate();
        img->set_data(width, height, false, Image::FORMAT_RGBAF, pba);

        Ref<Environment> env;
        env.instantiate();
        Ref<PanoramaSkyMaterial> sky_mat;
        sky_mat.instantiate();
        Ref<ImageTexture> texture;
        texture.instantiate();
        texture->set_image(img);
        sky_mat->set_panorama(texture);
        Ref<Sky> sky;
        sky.instantiate();
        sky->set_material(sky_mat);
        env->set_background(Environment::BG_SKY);
        env->set_sky(sky);


        // for a sun like brightness, you'd want a rgb around 15.0f or something
        env->set_glow_enabled(true);
        env->set_glow_blend_mode(Environment::GLOW_BLEND_MODE_ADDITIVE);
        env->set_glow_hdr_bleed_threshold(10.0); // Only bright areas (like your sun)
        env->set_glow_hdr_bleed_scale(2.0);
        env->set_glow_strength(1.0);
        env->set_glow_intensity(0.1);
        env->set_glow_level(0, 0.0);  // Smallest - disable
        env->set_glow_level(1, 0.5);  // Small
        env->set_glow_level(2, 1.0);  // Medium - main effect
        env->set_glow_level(3, 0.5);  // Large
        env->set_glow_level(4, 0.0);  // Larger - disable
        env->set_glow_level(5, 0.0);  // Huge - disable
        env->set_glow_level(6, 0.0);  // Largest - disable

        set_skybox(parent, env);
    }

    // Attach the environment to a WorldEnvironment node under `parent` (create if needed)
    static void set_skybox(Node* parent, Ref<Environment> env) {
        if (!parent) return;
        Node* we_node = parent->get_node_or_null("WorldEnvironment");
        if (we_node == nullptr) {
            WorldEnvironment* we = memnew(WorldEnvironment);
            we->set_name("WorldEnvironment");
            we->set_environment(env);
            parent->add_child(we);
        } else {
            WorldEnvironment* we = Object::cast_to<WorldEnvironment>(we_node);
            if (we) we->set_environment(env);
        }
    }

};


