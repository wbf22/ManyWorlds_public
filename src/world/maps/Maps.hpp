#pragma once


#include "IsometricRenderer.hpp"
#include "../../chunks/RootChunk.h"
#include "util/CoordinateConversion.hpp"
#include "util/BMaterial.hpp"
#include <unordered_set>


using namespace std;


struct Maps {


    static ChunkType zoom_to_chunk_type(double zoom) {
        static unordered_map<int, ChunkType> table = {
            {0, ChunkType::CHUNK_1},
            {2, ChunkType::CHUNK_4},
            {4, ChunkType::CHUNK_16},
            {6, ChunkType::CHUNK_64},
            {9, ChunkType::CHUNK_512},
            {13, ChunkType::CHUNK_8192},
            {17, ChunkType::CHUNK_131072},
            {21, ChunkType::CHUNK_2097152}
        };
        double target = zoom * 21.0;
        if (target <= 0.0) return ChunkType::CHUNK_1;
        if (target >= 21.0) return ChunkType::CHUNK_2097152;

        int zoom_type = (int) target;
        while (table.find(zoom_type) == table.end()) {
            --zoom_type;
        }
        return table[zoom_type];
    }


    static uint8_t* make_default_texture() {
        int len = IsometricRenderer::IMAGE_TEXTURE_SIZE * IsometricRenderer::IMAGE_TEXTURE_SIZE * 4;
        uint8_t* tex = new uint8_t[len];
        for (int i = 0; i < len; i += 4) {
            int ind = i / 4;
            int edge = IsometricRenderer::IMAGE_TEXTURE_SIZE;
            if (ind % edge == 0 || ind / edge == 0 ||
                (ind + 1) % edge == 0 || ind / edge == edge - 1) {
                tex[i] = 210; tex[i+1] = 210; tex[i+2] = 210;
            } else {
                tex[i] = 150; tex[i+1] = 150; tex[i+2] = 150;
            }
            tex[i+3] = 255;
        }
        return tex;
    }


    static uint8_t* render_map(
        shared_ptr<RootChunk> rootChunk,
        double zoom,
        int64_t bottom_left_corner_x,
        int64_t bottom_left_corner_z,
        IsoAngle angle
    ) {
        ChunkType type = zoom_to_chunk_type(zoom);
        int64_t chunk_size = Chunk::CHUNK_SIZES[type];
        int64_t num_chunks = 128;
        int64_t world_size = rootChunk->world_size;

        int64_t start_x = (bottom_left_corner_x / chunk_size) * chunk_size;
        int64_t start_z = (bottom_left_corner_z / chunk_size) * chunk_size;
        int64_t end_x = start_x + num_chunks * chunk_size;
        int64_t end_z = start_z + num_chunks * chunk_size;

        vector<shared_ptr<Block>> blocks;
        blocks.reserve(num_chunks * num_chunks * 2);
        unordered_set<string> unique_materials;

        for (int64_t x = start_x; x < end_x; x += chunk_size) {
            for (int64_t z = start_z; z < end_z; z += chunk_size) {
                int64_t cx = x, cz = z;
                CoordinateConversion::wrap(cx, cz, world_size);
                shared_ptr<Chunk> chunk = rootChunk->getChunk(type, cx, cz);
                if (chunk == nullptr) continue;
                blocks.push_back(chunk);
                unique_materials.insert(chunk->material);
                for (auto& db : chunk->downBlocks) {
                    blocks.push_back(db);
                    unique_materials.insert(db->material);
                }
            }
        }

        if (blocks.empty()) return nullptr;

        IsometricRenderer::set_position_double_on_blocks(blocks);

        unordered_map<string, uint8_t*> textures_rgba;
        int width, height, n;
        for (auto& mat : unique_materials) {
            string path = BMaterial::path(mat);
            uint8_t* tex = stbi_load(path.c_str(), &width, &height, &n, 4);
            if (tex != nullptr) {
                textures_rgba[mat] = tex;
            }
        }

        if (textures_rgba.find("") == textures_rgba.end()) {
            textures_rgba[""] = make_default_texture();
        }

        uint8_t background[4] = {10, 10, 10, 255};
        double scale = 16.0 / chunk_size;

        uint8_t* result = IsometricRenderer::render_tile(
            bottom_left_corner_x,
            bottom_left_corner_z,
            num_chunks,
            blocks,
            textures_rgba,
            background,
            angle,
            scale
        );

        // clean up textures
        for (auto it = textures_rgba.begin(); it != textures_rgba.end(); ++it) {
            if (it->first == "") {
                delete[] it->second;
            } else {
                stbi_image_free(it->second);
            }
        }

        return result;
    }


};
