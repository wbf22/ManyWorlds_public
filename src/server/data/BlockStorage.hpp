#pragma once

#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include "server/Settings.hpp"
#include "blocks/Block.h"
#include "util/StellarCoordinate.hpp"


using namespace std;


struct StorageKey {
    int64_t quadrant_x;
    int64_t quadrant_y;
    int64_t quadrant_z;
    uint16_t light_year_x;
    uint16_t light_year_y;
    uint16_t light_year_z;
    uint64_t km_x;
    uint64_t km_y;
    uint64_t km_z;

};


/*
    Store all player edits in one big file per planet or space 100,000km

    Once the file exceeds 4096 blocks split into sub files based on chunk sizes or divisions of 100k km

    In memory cache will be kept of most recently requested blocks limited by a size

    On a new block edit coming in, just append to the end of the file, 

    Then whenever a resize is done or the file is read into cache condense the edits


    Read
    - instant to slowish
    Write
    - near instant


    2b2t probably has a couple billion edits as one of the largest minecraft servers. Though with our
    building system we have a lot more, especially with one main server for the game. But we can compress 
    about 10 million blocks in plain text to about 1mb. So 100 billion block edits would be about 10gb.

    A 128x128x128 area could be easily transferred 

*/
struct BlockStorage {

    inline static constexpr double MIN_BLOCK_SIZE = 0.25;
    int MAX_CACHE_BLOCKS = 1'000'000;
    inline static constexpr int SIZE_BITS = 2;
    inline static constexpr int MAT_BITS = 13;   // max 8192 materials

    static constexpr uint8_t MATERIAL_NAME_LEN_BITS = 8;

    inline static unordered_map<string, uint16_t> material_to_index;
    inline static vector<string> material_names;

    static uint16_t getOrCreateMaterial(const string& name) {
        auto it = material_to_index.find(name);
        if (it != material_to_index.end()) return it->second;
        uint16_t idx = material_names.size();
        material_to_index[name] = idx;
        material_names.push_back(name);
        return idx;
    }

    unordered_map<string, vector<shared_ptr<Block>>> memory_cache;

    vector<shared_ptr<Block>> get_in_space(int64_t x, int64_t y, int64_t z, shared_ptr<StellarCoordinate> pos) {
        // TODO: load edits from disk for the 100,000km space bucket containing (x,y,z)
        return {};
    }

    vector<shared_ptr<Block>> get_on_planet(int64_t x, int64_t y, int64_t z, string planet_id) {
        // TODO: load edits from disk for the planet bucket containing (x,y,z)
        return {};
    }
   


    /**
     * A storage container for a 1024x1024x1024 block chunk.
     * 
     * Different compression methods are used dependeing on the sparsity of the chunk,
     * which implement the abstract functions in this struct. 
     */
    struct StorageChunk {

        // bit packed constants
        inline static constexpr int COOR_BITS = 12;  // 0-4096 quarter-block coords
        inline static constexpr int TOTAL_BITS = 3 * COOR_BITS + BlockStorage::SIZE_BITS + BlockStorage::MAT_BITS; // 51

        // data
        uint64_t data_size;
        uint64_t num_elements;
        uint8_t* data;

        virtual ~StorageChunk() = default;
        virtual void pack(const vector<shared_ptr<Block>>& blocks, int64_t chunk_x, int64_t chunk_y, int64_t chunk_z) = 0;
        virtual vector<shared_ptr<Block>> unpack(int64_t chunk_x, int64_t chunk_y, int64_t chunk_z) = 0;
        virtual void add(shared_ptr<Block> block, int64_t chunk_x, int64_t chunk_y, int64_t chunk_z) = 0;

        // Wire serialization. Self-contained: material names are shipped inline so
        // the client and server don't need to agree on a shared material registry.
        virtual uint64_t wire_bit_length() const = 0;
        virtual void serialize(uint8_t* buffer, uint32_t& bit_offset) const = 0;
        virtual void deserialize(uint8_t* buffer, uint32_t& bit_offset, uint64_t num_blocks) = 0;

        bool rangeCompressedIsLikelyBetter() {
            /*
            TOTAL_OVERHEAD + n * BITS_PER_BLOCK = n * TOTAL_BITS
            TOTAL_OVERHEAD = -n * BITS_PER_BLOCK + n * TOTAL_BITS
            TOTAL_OVERHEAD = n * (-BITS_PER_BLOCK + TOTAL_BITS)
            n = TOTAL_OVERHEAD / (-BITS_PER_BLOCK + TOTAL_BITS)

            With our intial estimates this might be around 20 blocks when range compressed is better.
            That's assuming the packed is 52 bits, and the compressed is 34 bits per block with 368 of overhead.
            That'd be about 100 million blocks making 405 MB in memory. 
            */
            double MATERIAL_BITS_PER_CHUNK_ESTIMATE = 4; // for ~16 unique materials a player has used
            int RANGES_OVERHEAD = 16*6;
            int MATERIALS_OVERHEAD = 16 + 16*16;
            int TOTAL_OVERHEAD =  RANGES_OVERHEAD + MATERIALS_OVERHEAD;
            int LIKELY_BITS_PER_COORINDATE = 9; // for a 128 block range or 512 0.25 blocks
            int SIZE_BITS = 3;
            int BITS_PER_BLOCK = MATERIAL_BITS_PER_CHUNK_ESTIMATE + 3 * LIKELY_BITS_PER_COORINDATE + SIZE_BITS;

            int NUM_ELEMENTS_WHEN_COMPRESSED_IS_BETTER = TOTAL_OVERHEAD / (TOTAL_BITS-BITS_PER_BLOCK);

            return this->num_elements > NUM_ELEMENTS_WHEN_COMPRESSED_IS_BETTER;
        }

    protected:

        /**
         * writes the lowest n bits of val into the buffer, starting at bit offset bit_off.
         */
        static void writeBits(uint8_t* buf, uint64_t bit_off, uint32_t n, uint64_t val) {
            val &= (n == 64 ? ~0ULL : (1ULL << n) - 1);
            for (uint32_t i = 0; i < n; ++i) {
                uint64_t byte_idx = (bit_off + i) >> 3;
                uint8_t bit_idx = (bit_off + i) & 7;
                buf[byte_idx] = (buf[byte_idx] & ~(1 << bit_idx)) | ((val >> i) & 1) << bit_idx;
            }
        }

        /**
         * reads n bits and sets them in the result in the lowest bits with unset bits in the more significant ones
         */
        static uint64_t readBits(const uint8_t* buf, uint64_t bit_off, uint32_t n) {
            uint64_t val = 0;
            for (uint32_t i = 0; i < n; ++i) {
                uint64_t byte_idx = (bit_off + i) >> 3;
                uint8_t bit_idx = (bit_off + i) & 7;
                val |= ((buf[byte_idx] >> bit_idx) & 1) << i;
            }
            return val;
        }

    };

    /**
     * Fixed-size per-block record: 52 bits = 12+12+12+3+13 (x,y,z,size,material).
     * Good for sparse chunks where range compression overhead doesn't pay off.
     */
    struct BitPackedStorageChunk : StorageChunk {

        unordered_map<uint16_t, uint16_t> material_remap; // sender global idx -> local global idx (from wire)

        ~BitPackedStorageChunk() override {
            delete[] data;
        }

        BitPackedStorageChunk() = default;

        void pack(const vector<shared_ptr<Block>>& blocks, int64_t chunk_x, int64_t chunk_y, int64_t chunk_z) override {
            auto toQuarter = [&](double coord, int64_t origin) {
                return int64_t(round(coord / MIN_BLOCK_SIZE)) - int64_t(round(origin / MIN_BLOCK_SIZE));
            };

            // Allocate fixed-size records
            num_elements = blocks.size();
            uint64_t total_bits = uint64_t(num_elements) * TOTAL_BITS;
            data_size = (total_bits + 7) / 8;
            data = new uint8_t[data_size]();

            // Pack each block using global material indices
            uint64_t bit_off = 0;
            for (auto& block : blocks) {
                if (!block->position_double)
                    block->position_double = block->position->to_position_double();
                int64_t x = toQuarter(block->position_double->x, chunk_x);
                int64_t y = toQuarter(block->position_double->y, chunk_y);
                int64_t z = toQuarter(block->position_double->z, chunk_z);
                uint32_t size_idx = block->size < 1 ? 0 : block->size > 1 ? 2 : 1;
                uint32_t mat_idx = getOrCreateMaterial(block->material);

                writeBits(data, bit_off, COOR_BITS, x); bit_off += COOR_BITS;
                writeBits(data, bit_off, COOR_BITS, y); bit_off += COOR_BITS;
                writeBits(data, bit_off, COOR_BITS, z); bit_off += COOR_BITS;
                writeBits(data, bit_off, BlockStorage::SIZE_BITS, size_idx); bit_off += BlockStorage::SIZE_BITS;
                writeBits(data, bit_off, BlockStorage::MAT_BITS, mat_idx);  bit_off += BlockStorage::MAT_BITS;
            }
        }

        vector<shared_ptr<Block>> unpack(int64_t chunk_x, int64_t chunk_y, int64_t chunk_z) override {
            if (num_elements == 0) return {};

            vector<shared_ptr<Block>> blocks;
            blocks.reserve(num_elements);

            uint64_t bit_off = 0;
            for (uint64_t i = 0; i < num_elements; ++i) {
                uint64_t x = readBits(data, bit_off, COOR_BITS); bit_off += COOR_BITS;
                uint64_t y = readBits(data, bit_off, COOR_BITS); bit_off += COOR_BITS;
                uint64_t z = readBits(data, bit_off, COOR_BITS); bit_off += COOR_BITS;
                uint64_t size_idx = readBits(data, bit_off, BlockStorage::SIZE_BITS); bit_off += BlockStorage::SIZE_BITS;
                uint64_t mat_idx = readBits(data, bit_off, BlockStorage::MAT_BITS);   bit_off += BlockStorage::MAT_BITS;

                double size = size_idx == 0 ? 0.25 : size_idx == 2 ? 4 : 1;

                auto pos = make_shared<PositionDouble>();
                pos->x = chunk_x + x * MIN_BLOCK_SIZE;
                pos->y = chunk_y + y * MIN_BLOCK_SIZE;
                pos->z = chunk_z + z * MIN_BLOCK_SIZE;

                auto remap = material_remap.find((uint16_t)mat_idx);
                uint16_t local_idx = remap != material_remap.end() ? remap->second : (uint16_t)mat_idx;
                blocks.push_back(make_shared<Block>(material_names[local_idx], nullptr, pos, size));
            }

            return blocks;
        }

        void add(shared_ptr<Block> block, int64_t chunk_x, int64_t chunk_y, int64_t chunk_z) override {
            if (num_elements == 0) {
                pack({block}, chunk_x, chunk_y, chunk_z);
                return;
            }

            uint32_t mat_idx = getOrCreateMaterial(block->material);

            // Expand buffer by 1.5x if needed
            uint64_t new_total_bits = uint64_t(num_elements + 1) * TOTAL_BITS;
            uint64_t new_data_size = (new_total_bits + 7) / 8;
            if (new_data_size > data_size) {
                uint64_t expanded = max(new_data_size, uint64_t(data_size * 1.5));
                uint8_t* new_data = new uint8_t[expanded]();
                memcpy(new_data, data, data_size);
                delete[] data;
                data = new_data;
                data_size = expanded;
            }

            // Convert position and append
            if (!block->position_double)
                block->position_double = block->position->to_position_double();
            auto toQuarter = [&](double coord, int64_t origin) {
                return int64_t(round(coord / MIN_BLOCK_SIZE)) - int64_t(round(origin / MIN_BLOCK_SIZE));
            };
            int64_t qx = toQuarter(block->position_double->x, chunk_x);
            int64_t qy = toQuarter(block->position_double->y, chunk_y);
            int64_t qz = toQuarter(block->position_double->z, chunk_z);
            uint32_t size_idx = block->size < 1 ? 0 : block->size > 1 ? 2 : 1;

            uint64_t bit_off = uint64_t(num_elements) * TOTAL_BITS;
            writeBits(data, bit_off, COOR_BITS, qx); bit_off += COOR_BITS;
            writeBits(data, bit_off, COOR_BITS, qy); bit_off += COOR_BITS;
            writeBits(data, bit_off, COOR_BITS, qz); bit_off += COOR_BITS;
            writeBits(data, bit_off, BlockStorage::SIZE_BITS, size_idx); bit_off += BlockStorage::SIZE_BITS;
            writeBits(data, bit_off, BlockStorage::MAT_BITS, mat_idx);
            num_elements++;
        }

        // distinct global material indices referenced by the records
        vector<uint16_t> distinct_materials() const {
            unordered_set<uint16_t> set;
            for (uint64_t i = 0; i < num_elements; ++i) {
                uint64_t idx = readBits(data, i * TOTAL_BITS + 3 * COOR_BITS + BlockStorage::SIZE_BITS, BlockStorage::MAT_BITS);
                set.insert((uint16_t)idx);
            }
            vector<uint16_t> sorted(set.begin(), set.end());
            sort(sorted.begin(), sorted.end());
            return sorted;
        }

        uint64_t wire_bit_length() const override {
            uint64_t bits = 8; // num_materials
            for (uint16_t g : distinct_materials()) {
                bits += BlockStorage::MAT_BITS + BlockStorage::MATERIAL_NAME_LEN_BITS + 8 * material_names[g].size();
            }
            bits += num_elements * TOTAL_BITS;
            return bits;
        }

        void serialize(uint8_t* buffer, uint32_t& bit_off) const override {
            vector<uint16_t> materials_used = distinct_materials();
            writeBits(buffer, bit_off, 8, materials_used.size()); bit_off += 8;
            for (uint16_t g : materials_used) {
                writeBits(buffer, bit_off, BlockStorage::MAT_BITS, g); bit_off += BlockStorage::MAT_BITS;
                const string& name = material_names[g];
                writeBits(buffer, bit_off, BlockStorage::MATERIAL_NAME_LEN_BITS, name.size()); bit_off += BlockStorage::MATERIAL_NAME_LEN_BITS;
                for (char c : name) { writeBits(buffer, bit_off, 8, (uint8_t)c); bit_off += 8; }
            }
            // raw fixed-size records
            uint64_t data_bits = num_elements * TOTAL_BITS;
            uint64_t full_bytes = data_bits / 8;
            for (uint64_t i = 0; i < full_bytes; ++i) { writeBits(buffer, bit_off, 8, data[i]); bit_off += 8; }
            if (data_bits % 8) { writeBits(buffer, bit_off, data_bits % 8, data[full_bytes]); bit_off += data_bits % 8; }
        }

        void deserialize(uint8_t* buffer, uint32_t& bit_off, uint64_t num_blocks) override {
            material_remap.clear();
            uint64_t num_materials = readBits(buffer, bit_off, 8); bit_off += 8;
            for (uint64_t m = 0; m < num_materials; ++m) {
                uint16_t sender_idx = readBits(buffer, bit_off, BlockStorage::MAT_BITS); bit_off += BlockStorage::MAT_BITS;
                uint64_t name_len = readBits(buffer, bit_off, BlockStorage::MATERIAL_NAME_LEN_BITS); bit_off += BlockStorage::MATERIAL_NAME_LEN_BITS;
                string name;
                for (uint64_t i = 0; i < name_len; ++i) { name += (char)readBits(buffer, bit_off, 8); bit_off += 8; }
                material_remap[sender_idx] = getOrCreateMaterial(name);
            }
            num_elements = num_blocks;
            uint64_t total_bits = num_elements * TOTAL_BITS;
            data_size = (total_bits + 7) / 8;
            delete[] data;
            data = new uint8_t[data_size]();
            uint64_t full_bytes = total_bits / 8;
            for (uint64_t i = 0; i < full_bytes; ++i) { data[i] = readBits(buffer, bit_off, 8); bit_off += 8; }
            if (total_bits % 8) { data[full_bytes] = readBits(buffer, bit_off, total_bits % 8); bit_off += total_bits % 8; }
        }

    };


    /**
     * Represents a 1024x1024x1024 chunk of blocks. 
     * Uses min max bounds in a chunk so positions can be stored with less bits
     * 
     * So for different ranges you only need this many bits (but to represent 0.25 blocks the full range would be 0-4096):
     * 
     * 0-0 : 0 bits
     * 0-1 : 1 bit
     * 0-3 : 2 bits
     * 0-7 : 3 bits
     * 0-15 : 4 bits
     * 0-31 : 5 bits
     * 0-63 : 6 bits
     * 0-127 : 7 bits
     * 0-255 : 8 bits
     * 0-511 : 9 bits
     * 0-1023 : 10 bits
     * 0-2047 : 11 bits
     * 0-4095 : 12 bits
     * 
     * For materials we could need anywhere from 1 bit to 16 bits in any entry, with that number corresponding to the index in
     * the materials array. (We'll probably never get above 9 bits or 512 materials in the game anyway)
     * 
     * The size will always take up 3 bits as blocks or size 0.25, 1, or 4 which correspond to 0.125m, 0.5m and 2m respectively
     * 
     */
    struct RangeCompressedStorageChunk : StorageChunk {

        uint16_t x_min;
        uint16_t x_max;
        uint16_t y_min;
        uint16_t y_max;
        uint16_t z_min;
        uint16_t z_max;

        uint16_t* materials;
        uint16_t materials_size;

        RangeCompressedStorageChunk()
            : x_min(0), x_max(0), y_min(0), y_max(0), z_min(0), z_max(0)
            , materials(nullptr), materials_size(0) {
            data_size = 0;
            num_elements = 0;
            data = nullptr;
        }

        ~RangeCompressedStorageChunk() override {
            delete[] data;
            delete[] materials;
        }

        RangeCompressedStorageChunk(const RangeCompressedStorageChunk&) = delete;
        RangeCompressedStorageChunk& operator=(const RangeCompressedStorageChunk&) = delete;

        void pack(const vector<shared_ptr<Block>>& blocks, int64_t chunk_x, int64_t chunk_y, int64_t chunk_z) override {
            // Phase 1 — collect bounds and unique global material indices in one pass
            x_min = y_min = z_min = 4096;
            x_max = y_max = z_max = 0;

            auto toQuarter = [&](double coord, int64_t origin) {
                return int64_t(round(coord / MIN_BLOCK_SIZE)) - int64_t(round(origin / MIN_BLOCK_SIZE));
            };

            unordered_map<uint16_t, uint32_t> mat_map;
            vector<uint16_t> mat_list;

            for (auto& block : blocks) {
                if (!block->position_double)
                    block->position_double = block->position->to_position_double();
                int64_t bx = toQuarter(block->position_double->x, chunk_x);
                int64_t by = toQuarter(block->position_double->y, chunk_y);
                int64_t bz = toQuarter(block->position_double->z, chunk_z);

                if (bx < x_min) x_min = bx;
                if (bx > x_max) x_max = bx;
                if (by < y_min) y_min = by;
                if (by > y_max) y_max = by;
                if (bz < z_min) z_min = bz;
                if (bz > z_max) z_max = bz;

                uint16_t global_idx = getOrCreateMaterial(block->material);
                if (mat_map.find(global_idx) == mat_map.end()) {
                    uint32_t idx = mat_list.size();
                    mat_map[global_idx] = idx;
                    mat_list.push_back(global_idx);
                }
            }

            // Exclusive max so range = max - min gives correct bit count
            x_max += 1; y_max += 1; z_max += 1;

            // Persist per-chunk palette (global material indices)
            materials_size = mat_list.size();
            materials = new uint16_t[materials_size];
            for (uint32_t i = 0; i < materials_size; ++i)
                materials[i] = mat_list[i];

            // Determine per-field bit widths from ranges
            uint32_t x_bits = bitsNeeded(x_max - x_min);
            uint32_t y_bits = bitsNeeded(y_max - y_min);
            uint32_t z_bits = bitsNeeded(z_max - z_min);
            uint32_t mat_bits = bitsNeeded(materials_size);

            // Allocate exact-size byte buffer for the bitstream
            uint32_t bits_per_block = x_bits + y_bits + z_bits + BlockStorage::SIZE_BITS + mat_bits;
            uint64_t total_bits = uint64_t(blocks.size()) * bits_per_block;
            data_size = (total_bits + 7) / 8;
            num_elements = blocks.size();
            data = new uint8_t[data_size]();

            // Phase 2 — pack each block as offset-from-min + size + palette index
            uint64_t bit_off = 0;
            for (auto& block : blocks) {
                int64_t bx = toQuarter(block->position_double->x, chunk_x);
                int64_t by = toQuarter(block->position_double->y, chunk_y);
                int64_t bz = toQuarter(block->position_double->z, chunk_z);

                uint32_t size_idx = block->size < 1 ? 0 : block->size > 1 ? 2 : 1;
                uint16_t global_idx = getOrCreateMaterial(block->material);
                uint32_t pal_idx = mat_map[global_idx];

                writeBits(data, bit_off, x_bits, bx - x_min);  bit_off += x_bits;
                writeBits(data, bit_off, y_bits, by - y_min);  bit_off += y_bits;
                writeBits(data, bit_off, z_bits, bz - z_min);  bit_off += z_bits;
                writeBits(data, bit_off, BlockStorage::SIZE_BITS, size_idx); bit_off += BlockStorage::SIZE_BITS;
                writeBits(data, bit_off, mat_bits, pal_idx);   bit_off += mat_bits;
            }
        }

        uint64_t wire_bit_length() const override {
            uint64_t bits = 6 * 16 + 8; // bounds + palette size
            for (uint16_t i = 0; i < materials_size; ++i) {
                bits += BlockStorage::MAT_BITS + BlockStorage::MATERIAL_NAME_LEN_BITS + 8 * material_names[materials[i]].size();
            }
            uint32_t bits_per_block = bitsNeeded(x_max - x_min) + bitsNeeded(y_max - y_min)
                                    + bitsNeeded(z_max - z_min) + BlockStorage::SIZE_BITS + bitsNeeded(materials_size);
            bits += num_elements * bits_per_block;
            return bits;
        }

        void serialize(uint8_t* buffer, uint32_t& bit_off) const override {
            writeBits(buffer, bit_off, 16, x_min); bit_off += 16;
            writeBits(buffer, bit_off, 16, x_max); bit_off += 16;
            writeBits(buffer, bit_off, 16, y_min); bit_off += 16;
            writeBits(buffer, bit_off, 16, y_max); bit_off += 16;
            writeBits(buffer, bit_off, 16, z_min); bit_off += 16;
            writeBits(buffer, bit_off, 16, z_max); bit_off += 16;
            writeBits(buffer, bit_off, 8, materials_size); bit_off += 8;
            for (uint16_t i = 0; i < materials_size; ++i) {
                const string& name = material_names[materials[i]];
                writeBits(buffer, bit_off, BlockStorage::MAT_BITS, materials[i]); bit_off += BlockStorage::MAT_BITS;
                writeBits(buffer, bit_off, BlockStorage::MATERIAL_NAME_LEN_BITS, name.size()); bit_off += BlockStorage::MATERIAL_NAME_LEN_BITS;
                for (char c : name) { writeBits(buffer, bit_off, 8, (uint8_t)c); bit_off += 8; }
            }
            uint32_t x_bits = bitsNeeded(x_max - x_min);
            uint32_t y_bits = bitsNeeded(y_max - y_min);
            uint32_t z_bits = bitsNeeded(z_max - z_min);
            uint32_t mat_bits = bitsNeeded(materials_size);
            uint32_t bits_per_block = x_bits + y_bits + z_bits + BlockStorage::SIZE_BITS + mat_bits;
            uint64_t data_bits = num_elements * bits_per_block;
            uint64_t full_bytes = data_bits / 8;
            for (uint64_t i = 0; i < full_bytes; ++i) { writeBits(buffer, bit_off, 8, data[i]); bit_off += 8; }
            if (data_bits % 8) { writeBits(buffer, bit_off, data_bits % 8, data[full_bytes]); bit_off += data_bits % 8; }
        }

        void deserialize(uint8_t* buffer, uint32_t& bit_off, uint64_t num_blocks) override {
            x_min = readBits(buffer, bit_off, 16); bit_off += 16;
            x_max = readBits(buffer, bit_off, 16); bit_off += 16;
            y_min = readBits(buffer, bit_off, 16); bit_off += 16;
            y_max = readBits(buffer, bit_off, 16); bit_off += 16;
            z_min = readBits(buffer, bit_off, 16); bit_off += 16;
            z_max = readBits(buffer, bit_off, 16); bit_off += 16;
            delete[] materials;
            materials_size = readBits(buffer, bit_off, 8); bit_off += 8;
            materials = new uint16_t[materials_size];
            for (uint16_t i = 0; i < materials_size; ++i) {
                readBits(buffer, bit_off, BlockStorage::MAT_BITS); bit_off += BlockStorage::MAT_BITS; // sender idx ignored
                uint64_t name_len = readBits(buffer, bit_off, BlockStorage::MATERIAL_NAME_LEN_BITS); bit_off += BlockStorage::MATERIAL_NAME_LEN_BITS;
                string name;
                for (uint64_t j = 0; j < name_len; ++j) { name += (char)readBits(buffer, bit_off, 8); bit_off += 8; }
                materials[i] = getOrCreateMaterial(name);
            }
            num_elements = num_blocks;
            uint32_t x_bits = bitsNeeded(x_max - x_min);
            uint32_t y_bits = bitsNeeded(y_max - y_min);
            uint32_t z_bits = bitsNeeded(z_max - z_min);
            uint32_t mat_bits = bitsNeeded(materials_size);
            uint32_t bits_per_block = x_bits + y_bits + z_bits + BlockStorage::SIZE_BITS + mat_bits;
            uint64_t total_bits = num_blocks * bits_per_block;
            data_size = (total_bits + 7) / 8;
            delete[] data;
            data = new uint8_t[data_size]();
            uint64_t full_bytes = total_bits / 8;
            for (uint64_t i = 0; i < full_bytes; ++i) { data[i] = readBits(buffer, bit_off, 8); bit_off += 8; }
            if (total_bits % 8) { data[full_bytes] = readBits(buffer, bit_off, total_bits % 8); bit_off += total_bits % 8; }
        }

        vector<shared_ptr<Block>> unpack(int64_t chunk_x, int64_t chunk_y, int64_t chunk_z) override {
            if (num_elements == 0) return {};

            // Re-derive bit widths from stored bounds
            uint32_t x_bits = bitsNeeded(x_max - x_min);
            uint32_t y_bits = bitsNeeded(y_max - y_min);
            uint32_t z_bits = bitsNeeded(z_max - z_min);
            uint32_t mat_bits = bitsNeeded(materials_size);

            uint32_t bits_per_block = x_bits + y_bits + z_bits + BlockStorage::SIZE_BITS + mat_bits;

            vector<shared_ptr<Block>> blocks;
            blocks.reserve(num_elements);

            uint64_t bit_off = 0;
            for (uint64_t i = 0; i < num_elements; ++i) {
                uint64_t ox = readBits(data, bit_off, x_bits);  bit_off += x_bits;
                uint64_t oy = readBits(data, bit_off, y_bits);  bit_off += y_bits;
                uint64_t oz = readBits(data, bit_off, z_bits);  bit_off += z_bits;
                uint64_t size_idx = readBits(data, bit_off, BlockStorage::SIZE_BITS); bit_off += BlockStorage::SIZE_BITS;
                uint64_t pal_idx = readBits(data, bit_off, mat_bits);   bit_off += mat_bits;

                double size = size_idx == 0 ? 0.25 : size_idx == 2 ? 4 : 1;

                auto pos = make_shared<PositionDouble>();
                pos->x = chunk_x + (x_min + ox) * MIN_BLOCK_SIZE;
                pos->y = chunk_y + (y_min + oy) * MIN_BLOCK_SIZE;
                pos->z = chunk_z + (z_min + oz) * MIN_BLOCK_SIZE;

                uint16_t global_idx = materials[pal_idx];
                blocks.push_back(make_shared<Block>(material_names[global_idx], nullptr, pos, size));
            }

            return blocks;
        }

        void add(shared_ptr<Block> block, int64_t chunk_x, int64_t chunk_y, int64_t chunk_z) override {
            if (num_elements == 0) {
                pack({block}, chunk_x, chunk_y, chunk_z);
                return;
            }

            auto toQuarter = [&](double coord, int64_t origin) {
                return int64_t(round(coord / MIN_BLOCK_SIZE)) - int64_t(round(origin / MIN_BLOCK_SIZE));
            };
            if (!block->position_double)
                block->position_double = block->position->to_position_double();
            int64_t bx = toQuarter(block->position_double->x, chunk_x);
            int64_t by = toQuarter(block->position_double->y, chunk_y);
            int64_t bz = toQuarter(block->position_double->z, chunk_z);

            bool in_bounds = bx >= x_min && bx < x_max
                          && by >= y_min && by < y_max
                          && bz >= z_min && bz < z_max;

            uint16_t global_idx = getOrCreateMaterial(block->material);
            uint32_t pal_idx = 0;
            bool in_palette = false;
            for (uint32_t i = 0; i < materials_size; ++i) {
                if (materials[i] == global_idx) {
                    pal_idx = i;
                    in_palette = true;
                    break;
                }
            }

            if (in_bounds && in_palette) {
                // Fast path: append
                uint32_t x_bits = bitsNeeded(x_max - x_min);
                uint32_t y_bits = bitsNeeded(y_max - y_min);
                uint32_t z_bits = bitsNeeded(z_max - z_min);
                uint32_t mat_bits = bitsNeeded(materials_size);
                uint32_t bits_per_block = x_bits + y_bits + z_bits + BlockStorage::SIZE_BITS + mat_bits;

                uint64_t new_total_bits = (num_elements + 1) * bits_per_block;
                uint64_t new_data_size = (new_total_bits + 7) / 8;

                if (new_data_size > data_size) {
                    uint64_t expanded = max(new_data_size, uint64_t(data_size * 1.5));
                    uint8_t* new_data = new uint8_t[expanded]();
                    memcpy(new_data, data, data_size);
                    delete[] data;
                    data = new_data;
                    data_size = expanded;
                }

                uint32_t size_idx = block->size < 1 ? 0 : block->size > 1 ? 2 : 1;
                uint64_t bit_off = num_elements * bits_per_block;
                writeBits(data, bit_off, x_bits, bx - x_min);  bit_off += x_bits;
                writeBits(data, bit_off, y_bits, by - y_min);  bit_off += y_bits;
                writeBits(data, bit_off, z_bits, bz - z_min);  bit_off += z_bits;
                writeBits(data, bit_off, BlockStorage::SIZE_BITS, size_idx); bit_off += BlockStorage::SIZE_BITS;
                writeBits(data, bit_off, mat_bits, pal_idx);
                num_elements++;
            } else {
                auto existing = unpack(chunk_x, chunk_y, chunk_z);
                existing.push_back(block);
                pack(existing, chunk_x, chunk_y, chunk_z);
            }
        }


        static uint32_t bitsNeeded(uint32_t range) {
            uint32_t bits = 0;
            while ((1u << bits) < range) ++bits;
            return bits;
        }

    };



};