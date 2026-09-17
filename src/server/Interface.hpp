#pragma once


#include <cstdint>
#include <cmath>
#include <functional>
#include <cstring> 
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include "util/BitUtil.hpp"
#include "util/Id64.hpp"
#include "server/data/BlockStorage.hpp"
#include <string>
#include <vector>
#include <cstdio>
#include <algorithm>
#include <array>
#include <sstream>
#include <memory>

        

using float32_t = float;
using namespace std;


enum class RequestType : uint8_t {


    // ---- short request types, 2 bit encoding ----------------------------

    /**
     * A client reporting their current position
     */
    CLIENT_LOCAL_POSITION_UPDATE,

    /**
     * A position update for nearby players, NPC's, vehicles or falling objects to the client
     */
    BATCH_POSITION_UPDATE,

    /**
     * A message containing hit data. Sent for a few reasons
     * - client claiming a hit
     * - server sending a hit to client
     * - client acknowleging a hit recieved from the server (exact match is checked)
     */
    HIT,

    /**
     * A message notifying that an attack should be shown
     */
    ATTACK,



    // ---- long request types, 8 bit encoding -----------------------------

    /**
     * A full position update for when players, NPC's, vehicles or falling objects cross between local areas 
     */
    POSITION_UPDATE,
    
    /**
     * Asks if the server is still alive, or if the client is still alive
     */
    PING,

    /**
     * Client attempting to create account. Server reporting client id and validaty time.
     */
    REGISTER,

    /**
     * Client attempting to login
     */
    LOGIN,

    /**
     * Client requesting a puzzle from the server to use in the auth process. Server sending back puzzle.
     */
    PUZZLE,

    /**
     * Client loging out
     */
    LOGOUT,

    /**
     * Server sending auth result to the client including a token for future requests.
     */
    AUTH,

    /**
     * Admin client requesting the server to shutdown
     */
    STOP,

    /**
     * Client requesting to change password providing player_id, token, old password, and new password
     */
    PASSWORD_CHANGE,

    /**
     * A list of blocks/removed blocks, and schemata placements.
     * Can be send from the client as an update, or from the server as the client moves
     */
    BLOCKS,

    /**
     * A schemata object sent from either the client or server. The id for the object and it's block data
     */
    SCHEMATA,


    /**
     * Server telling client an npc or player has died
     */
    DEAD,

    /**
     * A periodic resync telling clients which players 
     * are dead vs alive
     */
    RESYNC,


    /**
     * Placement of a block/removing a block, or placement of a schemata
     */
    PLACEMENT,


    /**
     * Announcing a player made vehicle to the client
     */
    VEHICLE,


    /**
     * Announces a new player, npc, vehicle, or falling object to clients, or that the entity is no longer present
     * 
     * All space objects are vehicles
     */
    NEW_ENTITY,


    
    

};



/**
 * This struct defines the interface between the client and the server. It contains structs
 * which make up the requests and responses between the two. Each has a RequestType field which 
 * helps the server/client determine what kind of request they are recieving.
 * 
 * 
 * 100-500 KB is reasonable for chunk loading requests. 
 */
namespace Interface {
    
    static constexpr uint8_t SHORT_TYPE = 0;
    static constexpr uint8_t LONG_TYPE = 1;
    static constexpr uint8_t SHORT_TYPE_LENGTH = 2;
    static constexpr uint8_t LONG_TYPE_LENGTH = 8;
    static constexpr uint8_t TYPE_TYPE_LENGTH = 1;
    static constexpr uint8_t ID_LENGTH_BITS = 2;


    // for local coordinates, this is the size of the offset in size 1 blocks (which are 0.5 m as of 8/4/26)
    static constexpr uint8_t LOCAL_POSITION_BITS = 12;
    static constexpr uint8_t ROTATION_BITS = 4;
    static constexpr uint8_t PLAYER_LOCAL_MAX_OFFSET = 512;
    static constexpr uint16_t BLOCK_LOCAL_MAX_OFFSET = 1024;
    static constexpr uint8_t MATERIAL_BITS = 13; // for 8192 possible materials

    /**
     * It's poetic that 2**13 is 8192 and that is our value for damage basically. As 13 is a cursed number, so is
     * the injuring of one's fellow man
     */
    static constexpr uint8_t HIT_VELOCITY_BITS = 13; // MAX 8192
    
    static constexpr uint8_t PNEG_CHUNK_1024_OFFSET_BITS = 54; // 2**64 / 1024 to get 54 bits. Basicaly 1024 offset in a MAX_UINT64_T range ( which is +/- ) 
    static constexpr uint64_t PNEG_BIAS = (1ULL << 53);       // offset used to encode signed coords into the 54 bit range
    static constexpr uint8_t KM_BITS_PER_LY = 44;             // per axis: 0-9460730472581 km in a light year fits 44 bits

    static constexpr uint8_t MAX_SCHEMATA_BLOCKS_BITS = 22;
    static constexpr uint32_t MAX_SCHEMATA_BLOCKS = 1 << MAX_SCHEMATA_BLOCKS_BITS;

    static constexpr uint8_t BLOCK_STORAGE_BIT_PACKED = 0;
    static constexpr uint8_t BLOCK_STORAGE_RANGE_COMPRESSED = 1;


    // ---- tiered id helpers (see util/Id64.hpp) ---------------------------
    // An id is packed as ID_LENGTH_BITS for which tier, then the id in that
    // tier's bit width. The smallest tier that fits the id is always chosen.
    static uint8_t id_tier_for(uint64_t id) {
        if (id < (1ULL << (uint8_t)Id64::IdLengthTier::TIER_0)) return 0; // 8 bits
        if (id < (1ULL << (uint8_t)Id64::IdLengthTier::TIER_1)) return 1; // 13 bits
        if (id < (1ULL << (uint8_t)Id64::IdLengthTier::TIER_2)) return 2; // 16 bits
        return 3;                                                          // 64 bits
    }

    static uint8_t id_tier_bits(uint8_t tier) {
        switch (tier) {
            case 0: return (uint8_t)Id64::IdLengthTier::TIER_0;
            case 1: return (uint8_t)Id64::IdLengthTier::TIER_1;
            case 2: return (uint8_t)Id64::IdLengthTier::TIER_2;
            default: return (uint8_t)Id64::IdLengthTier::TIER_3;
        }
    }

    static void pack_id(uint8_t* buffer, uint32_t& offset, uint64_t id) {
        uint8_t tier = id_tier_for(id);
        BitUtil::pack(buffer, offset, tier, ID_LENGTH_BITS);
        BitUtil::pack(buffer, offset, id, id_tier_bits(tier));
    }

    static uint64_t unpack_id(uint8_t* buffer, uint32_t& offset) {
        uint8_t tier = BitUtil::unpack(buffer, offset, ID_LENGTH_BITS);
        return BitUtil::unpack(buffer, offset, id_tier_bits(tier));
    }

    // reads a signed two's-complement value stored in `bits` and sign-extends
    static int64_t unpack_signed(uint8_t* buffer, uint32_t& offset, uint8_t bits) {
        uint64_t v = BitUtil::unpack(buffer, offset, bits);
        if (bits < 64 && (v & (1ULL << (bits - 1)))) v |= ~((1ULL << bits) - 1);
        return (int64_t)v;
    }

    // ---- PNEG signed 54-bit coords ---------------------------------------
    static uint64_t pack_pneg(int64_t v) {
        return (uint64_t)(v + (int64_t)PNEG_BIAS);
    }

    static int64_t unpack_pneg(uint64_t enc) {
        return (int64_t)(enc - PNEG_BIAS);
    }


    // ---- block payload helpers (see BlockStorage) ------------------------
    // Block data is prefixed with a 1 bit storage type flag, then the wire
    // serialization of the matching StorageChunk. Material names are shipped
    // inside the payload so both sides don't need a shared material registry.
    static void pack_block_payload(uint8_t* buffer, uint32_t& offset, const vector<shared_ptr<Block>>& blocks, int64_t ox, int64_t oy, int64_t oz) {
        if (blocks.empty()) return;
        // range compressed pays off once the fixed record overhead is exceeded
        bool use_range = blocks.size() > 20;
        unique_ptr<BlockStorage::StorageChunk> chunk = use_range
            ? (unique_ptr<BlockStorage::StorageChunk>)make_unique<BlockStorage::RangeCompressedStorageChunk>()
            : (unique_ptr<BlockStorage::StorageChunk>)make_unique<BlockStorage::BitPackedStorageChunk>();
        chunk->pack(blocks, ox, oy, oz);
        BitUtil::pack(buffer, offset, use_range ? BLOCK_STORAGE_RANGE_COMPRESSED : BLOCK_STORAGE_BIT_PACKED, 1);
        chunk->serialize(buffer, offset);
    }

    static vector<shared_ptr<Block>> unpack_block_payload(uint8_t* buffer, uint32_t& offset, uint32_t num_blocks, int64_t ox, int64_t oy, int64_t oz) {
        if (num_blocks == 0) return {};
        uint8_t flag = BitUtil::unpack(buffer, offset, 1);
        unique_ptr<BlockStorage::StorageChunk> chunk = flag == BLOCK_STORAGE_RANGE_COMPRESSED
            ? (unique_ptr<BlockStorage::StorageChunk>)make_unique<BlockStorage::RangeCompressedStorageChunk>()
            : (unique_ptr<BlockStorage::StorageChunk>)make_unique<BlockStorage::BitPackedStorageChunk>();
        chunk->deserialize(buffer, offset, num_blocks);
        return chunk->unpack(ox, oy, oz);
    }

    /**
     * Advances `bit_offset` past a block payload with `num_blocks` blocks.
     * Returns false if the whole payload isn't buffered yet. Mirrors
     * pack_block_payload so message_length can frame the containing message.
     */
    static bool skip_block_payload(const char* message, uint32_t& bit_offset, uint32_t avail_bits, uint64_t num_blocks) {
        auto read = [&](uint64_t bits) -> bool {
            if ((uint64_t)bit_offset + bits > avail_bits) return false;
            bit_offset += (uint32_t)bits;
            return true;
        };
        auto take = [&](uint32_t bits, uint64_t& out) -> bool {
            if ((uint64_t)bit_offset + bits > avail_bits) return false;
            out = BitUtil::unpack((uint8_t*)message, bit_offset, bits); // advances bit_offset
            return true;
        };
        auto skip_name = [&]() -> bool {
            if (!read(BlockStorage::MAT_BITS + BlockStorage::MATERIAL_NAME_LEN_BITS)) return false;
            uint32_t len_off = bit_offset - BlockStorage::MATERIAL_NAME_LEN_BITS;
            uint64_t name_len = BitUtil::unpack((uint8_t*)message, len_off, BlockStorage::MATERIAL_NAME_LEN_BITS);
            return read(name_len * 8);
        };

        if (num_blocks == 0) return true;

        uint64_t flag = 0;
        if (!take(1, flag)) return false;

        if (flag == BLOCK_STORAGE_BIT_PACKED) {
            uint64_t num_materials = 0;
            if (!take(8, num_materials)) return false;
            for (uint64_t m = 0; m < num_materials; ++m) {
                if (!skip_name()) return false;
            }
            uint64_t data_bits = num_blocks * (3 * BlockStorage::StorageChunk::COOR_BITS + BlockStorage::SIZE_BITS + BlockStorage::MAT_BITS);
            return read(data_bits);
        } else {
            uint64_t bounds[6] = {0};
            for (int i = 0; i < 6; ++i) {
                if (!take(16, bounds[i])) return false;
            }
            uint64_t materials_size = 0;
            if (!take(8, materials_size)) return false;
            for (uint64_t m = 0; m < materials_size; ++m) {
                if (!skip_name()) return false;
            }
            auto bits_needed = [](uint64_t range) -> uint64_t {
                uint64_t bits = 0;
                while ((1ULL << bits) < range) ++bits;
                return bits;
            };
            uint64_t bits_per_block = bits_needed(bounds[1] - bounds[0])
                                    + bits_needed(bounds[3] - bounds[2])
                                    + bits_needed(bounds[5] - bounds[4])
                                    + BlockStorage::SIZE_BITS
                                    + bits_needed(materials_size);
            return read(num_blocks * bits_per_block);
        }
    }


    /**
     * The ping request is for checking if the server is alive.
     */
    struct Ping {
        static constexpr int BIT_LENGTH = TYPE_TYPE_LENGTH + LONG_TYPE_LENGTH + 1;
        static constexpr int BYTES = 2;

        RequestType type = RequestType::PING;
        uint8_t status = 0;

        uint32_t pack(uint8_t* buffer) {
            uint32_t offset = 0;
            BitUtil::pack(buffer, offset, LONG_TYPE, TYPE_TYPE_LENGTH);
            BitUtil::pack(buffer, offset, (uint8_t)type, LONG_TYPE_LENGTH);
            BitUtil::pack(buffer, offset, status, 1);
            return offset;
        }

        static Ping unpack(uint8_t* buffer) {
            Ping ping;
            uint32_t offset = 0;
            BitUtil::unpack(buffer, offset, TYPE_TYPE_LENGTH);
            BitUtil::unpack(buffer, offset, LONG_TYPE_LENGTH);
            ping.status = BitUtil::unpack(buffer, offset, 1);
            return ping;
        }
    };


    /**
     * Register request from the client 
     */
    struct Register {
        RequestType type = RequestType::REGISTER;
        string password;
        uint8_t num_captchas;
        unordered_map<uint64_t, unordered_set<uint8_t>> captcha_answers;

        uint32_t pack(uint8_t* buffer) {
            uint32_t offset = 0;
            BitUtil::pack(buffer, offset, LONG_TYPE, TYPE_TYPE_LENGTH);
            BitUtil::pack(buffer, offset, (uint8_t)type, LONG_TYPE_LENGTH);
            BitUtil::pack(buffer, offset, password.size(), 5);
            for (char c : password) {
                BitUtil::pack(buffer, offset, (uint8_t)c, 8);
            }
            BitUtil::pack(buffer, offset, captcha_answers.size(), 4);
            vector<pair<uint64_t, unordered_set<uint8_t>>> sorted(captcha_answers.begin(), captcha_answers.end());
            sort(sorted.begin(), sorted.end(), [](auto& a, auto& b) { return a.first < b.first; });
            for (auto& [pid, answers] : sorted) {
                BitUtil::pack(buffer, offset, pid, 64);
                BitUtil::pack(buffer, offset, answers.size(), 4);
                for (uint8_t a : answers) {
                    BitUtil::pack(buffer, offset, a, 4);
                }
            }
            return offset;
        }

        static Register unpack(uint8_t* buffer) {
            Register r;
            uint32_t offset = 0;
            BitUtil::unpack(buffer, offset, TYPE_TYPE_LENGTH);
            BitUtil::unpack(buffer, offset, LONG_TYPE_LENGTH);
            uint8_t pw_len = BitUtil::unpack(buffer, offset, 5);
            r.password.resize(pw_len);
            for (uint8_t i = 0; i < pw_len; i++) {
                r.password[i] = BitUtil::unpack(buffer, offset, 8);
            }
            r.num_captchas = BitUtil::unpack(buffer, offset, 4);
            for (uint8_t i = 0; i < r.num_captchas; i++) {
                uint64_t pid = BitUtil::unpack(buffer, offset, 64);
                uint8_t num_answers = BitUtil::unpack(buffer, offset, 4);
                unordered_set<uint8_t> answers;
                for (uint8_t j = 0; j < num_answers; j++) {
                    answers.insert(BitUtil::unpack(buffer, offset, 4));
                }
                r.captcha_answers[pid] = answers;
            }
            return r;
        }
    };


    /**
     * Login request from the client
     */
    struct Login {
        RequestType type = RequestType::LOGIN;
        string player_id;
        string password;
        uint8_t num_captchas;
        unordered_map<uint64_t, unordered_set<uint8_t>> captcha_answers;

        uint32_t pack(uint8_t* buffer) {
            uint32_t offset = 0;
            BitUtil::pack(buffer, offset, LONG_TYPE, TYPE_TYPE_LENGTH);
            BitUtil::pack(buffer, offset, (uint8_t)type, LONG_TYPE_LENGTH);
            BitUtil::pack(buffer, offset, player_id.size(), 6);
            for (char c : player_id) {
                BitUtil::pack(buffer, offset, (uint8_t)c, 8);
            }
            BitUtil::pack(buffer, offset, password.size(), 5);
            for (char c : password) {
                BitUtil::pack(buffer, offset, (uint8_t)c, 8);
            }
            BitUtil::pack(buffer, offset, captcha_answers.size(), 4);
            vector<pair<uint64_t, unordered_set<uint8_t>>> sorted(captcha_answers.begin(), captcha_answers.end());
            sort(sorted.begin(), sorted.end(), [](auto& a, auto& b) { return a.first < b.first; });
            for (auto& [pid, answers] : sorted) {
                BitUtil::pack(buffer, offset, pid, 64);
                BitUtil::pack(buffer, offset, answers.size(), 4);
                for (uint8_t a : answers) {
                    BitUtil::pack(buffer, offset, a, 4);
                }
            }
            return offset;
        }

        static Login unpack(uint8_t* buffer) {
            Login l;
            uint32_t offset = 0;
            BitUtil::unpack(buffer, offset, TYPE_TYPE_LENGTH);
            BitUtil::unpack(buffer, offset, LONG_TYPE_LENGTH);
            uint8_t id_len = BitUtil::unpack(buffer, offset, 6);
            l.player_id.resize(id_len);
            for (uint8_t i = 0; i < id_len; i++) {
                l.player_id[i] = BitUtil::unpack(buffer, offset, 8);
            }
            uint8_t pw_len = BitUtil::unpack(buffer, offset, 5);
            l.password.resize(pw_len);
            for (uint8_t i = 0; i < pw_len; i++) {
                l.password[i] = BitUtil::unpack(buffer, offset, 8);
            }
            l.num_captchas = BitUtil::unpack(buffer, offset, 4);
            for (uint8_t i = 0; i < l.num_captchas; i++) {
                uint64_t pid = BitUtil::unpack(buffer, offset, 64);
                uint8_t num_answers = BitUtil::unpack(buffer, offset, 4);
                unordered_set<uint8_t> answers;
                for (uint8_t j = 0; j < num_answers; j++) {
                    answers.insert(BitUtil::unpack(buffer, offset, 4));
                }
                l.captcha_answers[pid] = answers;
            }
            return l;
        }
    };

    
    /**
     * Puzzle request from the client, server sends back puzzle
     */
    struct Puzzle {
        RequestType type = RequestType::PUZZLE;
        uint64_t puzzle_id;
        vector<uint8_t> image;

        uint32_t pack(uint8_t* buffer) {
            uint32_t offset = 0;
            BitUtil::pack(buffer, offset, LONG_TYPE, TYPE_TYPE_LENGTH);
            BitUtil::pack(buffer, offset, (uint8_t)type, LONG_TYPE_LENGTH);
            BitUtil::pack(buffer, offset, puzzle_id, 64);
            BitUtil::pack(buffer, offset, image.size(), 18);
            for (uint8_t b : image) {
                BitUtil::pack(buffer, offset, b, 8);
            }
            return offset;
        }

        static Puzzle unpack(uint8_t* buffer) {
            Puzzle p;
            uint32_t offset = 0;
            BitUtil::unpack(buffer, offset, TYPE_TYPE_LENGTH);
            BitUtil::unpack(buffer, offset, LONG_TYPE_LENGTH);
            p.puzzle_id = BitUtil::unpack(buffer, offset, 64);
            uint32_t img_size = BitUtil::unpack(buffer, offset, 18);
            p.image.resize(img_size);
            for (uint32_t i = 0; i < img_size; i++) {
                p.image[i] = BitUtil::unpack(buffer, offset, 8);
            }
            return p;
        }
    };
    
    
    /**
     * Logout request from the client
     */
    struct Logout {
        RequestType type = RequestType::LOGOUT;
        array<uint8_t, 16> uuid;

        static constexpr int BIT_LENGTH = TYPE_TYPE_LENGTH + LONG_TYPE_LENGTH + 128;
        static constexpr int BYTES = (BIT_LENGTH + 7) / 8;

        uint32_t pack(uint8_t* buffer) {
            uint32_t offset = 0;
            BitUtil::pack(buffer, offset, LONG_TYPE, TYPE_TYPE_LENGTH);
            BitUtil::pack(buffer, offset, (uint8_t)type, LONG_TYPE_LENGTH);
            for (int i = 0; i < 16; i++) {
                BitUtil::pack(buffer, offset, uuid[i], 8);
            }
            return offset;
        }

        static Logout unpack(uint8_t* buffer) {
            Logout l;
            uint32_t offset = 0;
            BitUtil::unpack(buffer, offset, TYPE_TYPE_LENGTH);
            BitUtil::unpack(buffer, offset, LONG_TYPE_LENGTH);
            for (int i = 0; i < 16; i++) {
                l.uuid[i] = BitUtil::unpack(buffer, offset, 8);
            }
            return l;
        }
    };
    

    /**
     * Auth response from the Server
     */
    struct Auth {
        RequestType type = RequestType::AUTH;
        array<uint8_t, 16> uuid;

        static constexpr int BIT_LENGTH = TYPE_TYPE_LENGTH + LONG_TYPE_LENGTH + 128;
        static constexpr int BYTES = (BIT_LENGTH + 7) / 8;

        uint32_t pack(uint8_t* buffer) {
            uint32_t offset = 0;
            BitUtil::pack(buffer, offset, LONG_TYPE, TYPE_TYPE_LENGTH);
            BitUtil::pack(buffer, offset, (uint8_t)type, LONG_TYPE_LENGTH);
            for (int i = 0; i < 16; i++) {
                BitUtil::pack(buffer, offset, uuid[i], 8);
            }
            return offset;
        }

        static Auth unpack(uint8_t* buffer) {
            Auth a;
            uint32_t offset = 0;
            BitUtil::unpack(buffer, offset, TYPE_TYPE_LENGTH);
            BitUtil::unpack(buffer, offset, LONG_TYPE_LENGTH);
            for (int i = 0; i < 16; i++) {
                a.uuid[i] = BitUtil::unpack(buffer, offset, 8);
            }
            return a;
        }
    };


    /**
     * Stop request from admin client
     */
    struct Stop {
        RequestType type = RequestType::STOP;
        array<uint8_t, 16> player_id;

        static constexpr int BIT_LENGTH = TYPE_TYPE_LENGTH + LONG_TYPE_LENGTH + 128;
        static constexpr int BYTES = (BIT_LENGTH + 7) / 8;

        uint32_t pack(uint8_t* buffer) {
            uint32_t offset = 0;
            BitUtil::pack(buffer, offset, LONG_TYPE, TYPE_TYPE_LENGTH);
            BitUtil::pack(buffer, offset, (uint8_t)type, LONG_TYPE_LENGTH);
            for (int i = 0; i < 16; i++) {
                BitUtil::pack(buffer, offset, player_id[i], 8);
            }
            return offset;
        }

        static Stop unpack(uint8_t* buffer) {
            Stop s;
            uint32_t offset = 0;
            BitUtil::unpack(buffer, offset, TYPE_TYPE_LENGTH);
            BitUtil::unpack(buffer, offset, LONG_TYPE_LENGTH);
            for (int i = 0; i < 16; i++) {
                s.player_id[i] = BitUtil::unpack(buffer, offset, 8);
            }
            return s;
        }
    };


    /**
     * Change password request from admin client
     */
    struct ChangePassword {
        RequestType type = RequestType::PASSWORD_CHANGE;
        array<uint8_t, 16> player_id;
        string old_password;
        string new_password;

        uint32_t pack(uint8_t* buffer) {
            uint32_t offset = 0;
            BitUtil::pack(buffer, offset, LONG_TYPE, TYPE_TYPE_LENGTH);
            BitUtil::pack(buffer, offset, (uint8_t)type, LONG_TYPE_LENGTH);
            for (int i = 0; i < 16; i++) {
                BitUtil::pack(buffer, offset, player_id[i], 8);
            }
            BitUtil::pack(buffer, offset, old_password.size(), 5);
            for (char c : old_password) {
                BitUtil::pack(buffer, offset, (uint8_t)c, 8);
            }
            BitUtil::pack(buffer, offset, new_password.size(), 5);
            for (char c : new_password) {
                BitUtil::pack(buffer, offset, (uint8_t)c, 8);
            }
            return offset;
        }

        static ChangePassword unpack(uint8_t* buffer) {
            ChangePassword c;
            uint32_t offset = 0;
            BitUtil::unpack(buffer, offset, TYPE_TYPE_LENGTH);
            BitUtil::unpack(buffer, offset, LONG_TYPE_LENGTH);
            for (int i = 0; i < 16; i++) {
                c.player_id[i] = BitUtil::unpack(buffer, offset, 8);
            }
            uint8_t old_len = BitUtil::unpack(buffer, offset, 5);
            c.old_password.resize(old_len);
            for (uint8_t i = 0; i < old_len; i++) {
                c.old_password[i] = BitUtil::unpack(buffer, offset, 8);
            }
            uint8_t new_len = BitUtil::unpack(buffer, offset, 5);
            c.new_password.resize(new_len);
            for (uint8_t i = 0; i < new_len; i++) {
                c.new_password[i] = BitUtil::unpack(buffer, offset, 8);
            }
            return c;
        }
    };


    /**
     * Assumes both the client and server know the stellar position, and the type of the player/npc, and just sends coarse offsets 
     * from start of 500 block offset (space), or 512 offset (terrain) and a coarse rotation. (we're calling it a Local)
     * 
     * When the player changes locals, they'll just send a StellarCoordinate or a PlanetPosition and the server 
     * will handle that
     * 
     * 3*LOCAL_POSITION_BITS + 3*ROTATION_BITS
     * 
     */
    struct ClientLocalPositionUpdate {
        static constexpr int BIT_LENGTH = TYPE_TYPE_LENGTH + SHORT_TYPE_LENGTH + 3*LOCAL_POSITION_BITS + 3*ROTATION_BITS;
        static constexpr int BYTES = (BIT_LENGTH + 7) / 8;

        RequestType type = RequestType::CLIENT_LOCAL_POSITION_UPDATE;
        uint16_t x;
        uint16_t y;
        uint16_t z;
        uint8_t rot_x;
        uint8_t rot_y;
        uint8_t rot_z;

        uint32_t pack(uint8_t* buffer) {
            uint32_t offset = 0;
            BitUtil::pack(buffer, offset, SHORT_TYPE, TYPE_TYPE_LENGTH);
            BitUtil::pack(buffer, offset, (uint8_t)type, SHORT_TYPE_LENGTH);
            BitUtil::pack(buffer, offset, x, LOCAL_POSITION_BITS);
            BitUtil::pack(buffer, offset, y, LOCAL_POSITION_BITS);
            BitUtil::pack(buffer, offset, z, LOCAL_POSITION_BITS);
            BitUtil::pack(buffer, offset, rot_x, ROTATION_BITS);
            BitUtil::pack(buffer, offset, rot_y, ROTATION_BITS);
            BitUtil::pack(buffer, offset, rot_z, ROTATION_BITS);
            return offset;
        }

        static ClientLocalPositionUpdate unpack(uint8_t* buffer) {
            ClientLocalPositionUpdate u;
            uint32_t offset = 0;
            BitUtil::unpack(buffer, offset, TYPE_TYPE_LENGTH);
            BitUtil::unpack(buffer, offset, SHORT_TYPE_LENGTH);
            u.x = BitUtil::unpack(buffer, offset, LOCAL_POSITION_BITS);
            u.y = BitUtil::unpack(buffer, offset, LOCAL_POSITION_BITS);
            u.z = BitUtil::unpack(buffer, offset, LOCAL_POSITION_BITS);
            u.rot_x = BitUtil::unpack(buffer, offset, ROTATION_BITS);
            u.rot_y = BitUtil::unpack(buffer, offset, ROTATION_BITS);
            u.rot_z = BitUtil::unpack(buffer, offset, ROTATION_BITS);
            return u;
        }
    };


    /**
     * A list of updates for different player ids near the player (these can be players, npcs, or vehicle)
     * 
     * ID_LENGTH_BITS + (one of the tiers) bits for player id
     * 3*LOCAL_POSITION_BITS + 3*ROTATION_BITS
     * 
     * So we'll just pack everything into one big byte array, with the the first 11 bits being the number of updates coming in,
     * and then every subsequent n bits will be a position update
     * 
     */
    struct BatchPositionUpdate {

        struct Entry {
            uint64_t player_id;
            uint16_t x;
            uint16_t y;
            uint16_t z;
            uint8_t rot_x;
            uint8_t rot_y;
            uint8_t rot_z;
        };

        RequestType type = RequestType::BATCH_POSITION_UPDATE;
        vector<Entry> entries;

        uint32_t pack(uint8_t* buffer) {
            uint32_t offset = 0;
            BitUtil::pack(buffer, offset, SHORT_TYPE, TYPE_TYPE_LENGTH);
            BitUtil::pack(buffer, offset, (uint8_t)type, SHORT_TYPE_LENGTH);
            BitUtil::pack(buffer, offset, entries.size(), 11);
            for (auto& e : entries) {
                pack_id(buffer, offset, e.player_id);
                BitUtil::pack(buffer, offset, e.x, LOCAL_POSITION_BITS);
                BitUtil::pack(buffer, offset, e.y, LOCAL_POSITION_BITS);
                BitUtil::pack(buffer, offset, e.z, LOCAL_POSITION_BITS);
                BitUtil::pack(buffer, offset, e.rot_x, ROTATION_BITS);
                BitUtil::pack(buffer, offset, e.rot_y, ROTATION_BITS);
                BitUtil::pack(buffer, offset, e.rot_z, ROTATION_BITS);
            }
            return offset;
        }

        static BatchPositionUpdate unpack(uint8_t* buffer) {
            BatchPositionUpdate u;
            uint32_t offset = 0;
            BitUtil::unpack(buffer, offset, TYPE_TYPE_LENGTH);
            BitUtil::unpack(buffer, offset, SHORT_TYPE_LENGTH);
            uint16_t count = BitUtil::unpack(buffer, offset, 11);
            u.entries.resize(count);
            for (int i = 0; i < count; i++) {
                u.entries[i].player_id = unpack_id(buffer, offset);
                u.entries[i].x = BitUtil::unpack(buffer, offset, LOCAL_POSITION_BITS);
                u.entries[i].y = BitUtil::unpack(buffer, offset, LOCAL_POSITION_BITS);
                u.entries[i].z = BitUtil::unpack(buffer, offset, LOCAL_POSITION_BITS);
                u.entries[i].rot_x = BitUtil::unpack(buffer, offset, ROTATION_BITS);
                u.entries[i].rot_y = BitUtil::unpack(buffer, offset, ROTATION_BITS);
                u.entries[i].rot_z = BitUtil::unpack(buffer, offset, ROTATION_BITS);
            }
            return u;
        }
    };


    /**
     * A position update for a player, npc, or vehicle used ONLY when changing locals!
     * 
     * Otherwise the more efficient local position updates should be used. But these message are
     * pretty important since subsequent local position updates rely on them. So they should be
     * sent over tcp
     * 
     * 9,460,730,472,580,800
     * 9460730472580.8 km/ly
     * 
     * bits
     * - 1 bit for space or terrain
     * - if terrain:
     *      + 63x3 for position
     * - if space:
     *      + 2 bit for quadrant change, light year change, or km change
     *      + if quadrant change:
     *          * 64*3 quadrant
     *          * 14*3 light year
     *          * KM_BITS_PER_LY*3 
     *      + if lightyear change:
     *          * 14*3 light year
     *          * KM_BITS_PER_LY*3 
     *      + if km change:
     *          * KM_BITS_PER_LY*3 
     */
    struct PositionUpdate {
        static constexpr int BIT_LENGTH_TERRAIN = TYPE_TYPE_LENGTH + LONG_TYPE_LENGTH + 1 + 63*3;
        static constexpr int BIT_LENGTH_SPACE_QUADRANT = TYPE_TYPE_LENGTH + LONG_TYPE_LENGTH + 1 + 2 + 64*3 + 14*3 + KM_BITS_PER_LY*3;
        static constexpr int BIT_LENGTH_SPACE_LIGHTYEAR = TYPE_TYPE_LENGTH + LONG_TYPE_LENGTH + 1 + 2 + 14*3 + KM_BITS_PER_LY*3;
        static constexpr int BIT_LENGTH_SPACE_KM = TYPE_TYPE_LENGTH + LONG_TYPE_LENGTH + 1 + 2 + KM_BITS_PER_LY*3;

        RequestType type = RequestType::POSITION_UPDATE;

        bool space;                  // 1 bit: 0 = terrain, 1 = space
        // terrain (space == false)
        int64_t t_x;                 // 63 bits
        int64_t t_y;                 // 63 bits
        int64_t t_z;                 // 63 bits
        // space (space == true)
        uint8_t space_change;        // 2 bits: 0 = quadrant, 1 = lightyear, 2 = km
        int64_t q_x;                 // 64 bits (quadrant change only)
        int64_t q_y;                 // 64 bits (quadrant change only)
        int64_t q_z;                 // 64 bits (quadrant change only)
        uint16_t ly_x;               // 14 bits (quadrant or lightyear change)
        uint16_t ly_y;               // 14 bits
        uint16_t ly_z;               // 14 bits
        uint64_t km_x;               // KM_BITS_PER_LY bits each (any space change)
        uint64_t km_y;               // KM_BITS_PER_LY bits each
        uint64_t km_z;               // KM_BITS_PER_LY bits each

        uint32_t pack(uint8_t* buffer) {
            uint32_t offset = 0;
            BitUtil::pack(buffer, offset, LONG_TYPE, TYPE_TYPE_LENGTH);
            BitUtil::pack(buffer, offset, (uint8_t)type, LONG_TYPE_LENGTH);
            BitUtil::pack(buffer, offset, space ? 1 : 0, 1);
            if (!space) {
                BitUtil::pack(buffer, offset, (uint64_t)t_x, 63);
                BitUtil::pack(buffer, offset, (uint64_t)t_y, 63);
                BitUtil::pack(buffer, offset, (uint64_t)t_z, 63);
                return offset;
            }
            BitUtil::pack(buffer, offset, space_change, 2);
            if (space_change == 0) {
                BitUtil::pack(buffer, offset, (uint64_t)q_x, 64);
                BitUtil::pack(buffer, offset, (uint64_t)q_y, 64);
                BitUtil::pack(buffer, offset, (uint64_t)q_z, 64);
            }
            if (space_change <= 1) {
                BitUtil::pack(buffer, offset, ly_x, 14);
                BitUtil::pack(buffer, offset, ly_y, 14);
                BitUtil::pack(buffer, offset, ly_z, 14);
            }
            BitUtil::pack(buffer, offset, km_x, KM_BITS_PER_LY);
            BitUtil::pack(buffer, offset, km_y, KM_BITS_PER_LY);
            BitUtil::pack(buffer, offset, km_z, KM_BITS_PER_LY);
            return offset;
        }

        static PositionUpdate unpack(uint8_t* buffer) {
            PositionUpdate p;
            uint32_t offset = 0;
            BitUtil::unpack(buffer, offset, TYPE_TYPE_LENGTH);
            BitUtil::unpack(buffer, offset, LONG_TYPE_LENGTH);
            p.space = BitUtil::unpack(buffer, offset, 1) != 0;
            if (!p.space) {
                p.t_x = unpack_signed(buffer, offset, 63);
                p.t_y = unpack_signed(buffer, offset, 63);
                p.t_z = unpack_signed(buffer, offset, 63);
                return p;
            }
            p.space_change = BitUtil::unpack(buffer, offset, 2);
            if (p.space_change == 0) {
                p.q_x = (int64_t)BitUtil::unpack(buffer, offset, 64);
                p.q_y = (int64_t)BitUtil::unpack(buffer, offset, 64);
                p.q_z = (int64_t)BitUtil::unpack(buffer, offset, 64);
            }
            if (p.space_change <= 1) {
                p.ly_x = BitUtil::unpack(buffer, offset, 14);
                p.ly_y = BitUtil::unpack(buffer, offset, 14);
                p.ly_z = BitUtil::unpack(buffer, offset, 14);
            }
            p.km_x = BitUtil::unpack(buffer, offset, KM_BITS_PER_LY);
            p.km_y = BitUtil::unpack(buffer, offset, KM_BITS_PER_LY);
            p.km_z = BitUtil::unpack(buffer, offset, KM_BITS_PER_LY);
            return p;
        }
    };


    /**
     * A hit event for calculating damage. The server and client should already know the mass of the object, the
     * position of the target and source entities, and any damage modifiers on the object (like shape and stuff)
     * 
     * This struct just provides the velocity (blocks/sec 0-8192) and the target and source entity ids. It can
     * be sent by the client when claiming a hit, or from the server reporting a hit. 
     * 
     * Player Claim Sequence:
     * - player hit claim
     * - server verifies, sends back hit as confirmation
     * 
     * Server Update Sequence:
     * - server sends hit
     * - player sends hit back as acknowledgement
     * - server keeps sending hit until acknowledged up to 3 times
     * 
     * This relies on each hit on a target entity having slightly different velocity, so we'll want to build in
     * significant velocity randomization to avoid collisions. Probably like 16 blocks/s variation would be enough
     * to avoid most collisions
     * 
     * HIT_VELOCITY_BITS for velocity, ID_LENGTH_BITS + (one of the tiers) bits for target player id
     * 
     */
    struct Hit {
        RequestType type = RequestType::HIT;
        uint16_t velocity;          // blocks/sec 0-8192
        uint64_t target_entity;     // tiered id

        uint32_t pack(uint8_t* buffer) {
            uint32_t offset = 0;
            BitUtil::pack(buffer, offset, SHORT_TYPE, TYPE_TYPE_LENGTH);
            BitUtil::pack(buffer, offset, (uint8_t)type, SHORT_TYPE_LENGTH);
            BitUtil::pack(buffer, offset, velocity, HIT_VELOCITY_BITS);
            pack_id(buffer, offset, target_entity);
            return offset;
        }

        static Hit unpack(uint8_t* buffer) {
            Hit h;
            uint32_t offset = 0;
            BitUtil::unpack(buffer, offset, TYPE_TYPE_LENGTH);
            BitUtil::unpack(buffer, offset, SHORT_TYPE_LENGTH);
            h.velocity = BitUtil::unpack(buffer, offset, HIT_VELOCITY_BITS);
            h.target_entity = unpack_id(buffer, offset);
            return h;
        }
    };


    /**
     * ID_LENGTH_BITS + (one of the tiers) bits for player id
     * 
     * 1 bit for if the angle of the weapon is being provided
     * 
     * if so then 3*ROTATION_BITS are also included
     */
    struct Attack {
        RequestType type = RequestType::ATTACK;
        uint64_t player_id;          // tiered id
        bool angle_provided;         // 1 bit
        uint8_t rot_x;               // 4 bits, only when angle_provided
        uint8_t rot_y;               // 4 bits
        uint8_t rot_z;               // 4 bits

        uint32_t pack(uint8_t* buffer) {
            uint32_t offset = 0;
            BitUtil::pack(buffer, offset, SHORT_TYPE, TYPE_TYPE_LENGTH);
            BitUtil::pack(buffer, offset, (uint8_t)type, SHORT_TYPE_LENGTH);
            pack_id(buffer, offset, player_id);
            BitUtil::pack(buffer, offset, angle_provided ? 1 : 0, 1);
            if (angle_provided) {
                BitUtil::pack(buffer, offset, rot_x, ROTATION_BITS);
                BitUtil::pack(buffer, offset, rot_y, ROTATION_BITS);
                BitUtil::pack(buffer, offset, rot_z, ROTATION_BITS);
            }
            return offset;
        }

        static Attack unpack(uint8_t* buffer) {
            Attack a;
            uint32_t offset = 0;
            BitUtil::unpack(buffer, offset, TYPE_TYPE_LENGTH);
            BitUtil::unpack(buffer, offset, SHORT_TYPE_LENGTH);
            a.player_id = unpack_id(buffer, offset);
            a.angle_provided = BitUtil::unpack(buffer, offset, 1) != 0;
            if (a.angle_provided) {
                a.rot_x = BitUtil::unpack(buffer, offset, ROTATION_BITS);
                a.rot_y = BitUtil::unpack(buffer, offset, ROTATION_BITS);
                a.rot_z = BitUtil::unpack(buffer, offset, ROTATION_BITS);
            }
            return a;
        }
    };


    /**
     * Server reporting a player died to a client
     * 
     * ID_LENGTH_BITS + (one of the tiers) bits for player id
     */
    struct Dead {
        RequestType type = RequestType::DEAD;
        uint64_t player_id;          // tiered id

        uint32_t pack(uint8_t* buffer) {
            uint32_t offset = 0;
            BitUtil::pack(buffer, offset, LONG_TYPE, TYPE_TYPE_LENGTH);
            BitUtil::pack(buffer, offset, (uint8_t)type, LONG_TYPE_LENGTH);
            pack_id(buffer, offset, player_id);
            return offset;
        }

        static Dead unpack(uint8_t* buffer) {
            Dead d;
            uint32_t offset = 0;
            BitUtil::unpack(buffer, offset, TYPE_TYPE_LENGTH);
            BitUtil::unpack(buffer, offset, LONG_TYPE_LENGTH);
            d.player_id = unpack_id(buffer, offset);
            return d;
        }
    };


    /**
     * Sends a list of all player ids relavent to the player. (if players have left surrounding locals
     * this will catch that)
     * 
     * ID_LENGTH_BITS + (one of the tiers) bits for player id, 1 bit for alive or dead
     * 
     */
    struct Resync {
        struct Entry {
            uint64_t player_id;      // tiered id
            uint8_t alive;           // 1 bit
        };

        RequestType type = RequestType::RESYNC;
        vector<Entry> entries;

        uint32_t pack(uint8_t* buffer) {
            uint32_t offset = 0;
            BitUtil::pack(buffer, offset, LONG_TYPE, TYPE_TYPE_LENGTH);
            BitUtil::pack(buffer, offset, (uint8_t)type, LONG_TYPE_LENGTH);
            BitUtil::pack(buffer, offset, entries.size(), 11);
            for (auto& e : entries) {
                pack_id(buffer, offset, e.player_id);
                BitUtil::pack(buffer, offset, e.alive, 1);
            }
            return offset;
        }

        static Resync unpack(uint8_t* buffer) {
            Resync r;
            uint32_t offset = 0;
            BitUtil::unpack(buffer, offset, TYPE_TYPE_LENGTH);
            BitUtil::unpack(buffer, offset, LONG_TYPE_LENGTH);
            uint16_t count = BitUtil::unpack(buffer, offset, 11);
            r.entries.resize(count);
            for (int i = 0; i < count; i++) {
                r.entries[i].player_id = unpack_id(buffer, offset);
                r.entries[i].alive = BitUtil::unpack(buffer, offset, 1);
            }
            return r;
        }
    };

    /**
     * Sends a schemata with it's id. Client can also send this without block data to
     * request a schemata from the server
     * 
     * ID_LENGTH_BITS + (one of the tiers) bits for schemata id
     * 
     * MAX_SCHEMATA_BLOCKS_BITS for how many blocks
     * 
     * blocks
     * - 1 bit for BIT_PACKED vs RANGE_COMPRESSED (see Block Storage)
     * - variable number of bits from the block storage type
     * 
     */
    struct Schemata {
        RequestType type = RequestType::SCHEMATA;
        uint64_t schemata_id;            // tiered id
        uint32_t num_blocks;             // MAX_SCHEMATA_BLOCKS_BITS (22)
        vector<shared_ptr<Block>> blocks; // positions relative to the schemata's min corner

        uint32_t pack(uint8_t* buffer) {
            uint32_t offset = 0;
            BitUtil::pack(buffer, offset, LONG_TYPE, TYPE_TYPE_LENGTH);
            BitUtil::pack(buffer, offset, (uint8_t)type, LONG_TYPE_LENGTH);
            pack_id(buffer, offset, schemata_id);
            num_blocks = blocks.size();
            BitUtil::pack(buffer, offset, num_blocks, MAX_SCHEMATA_BLOCKS_BITS);
            if (num_blocks == 0) return offset; // blockless request for the schemata
            // origin = min corner so block positions are stored relative to the schemata anchor
            int64_t ox = INT64_MAX, oy = INT64_MAX, oz = INT64_MAX;
            for (auto& b : blocks) {
                if (!b->position_double) b->position_double = b->position->to_position_double();
                ox = min(ox, (int64_t)floor(b->position_double->x));
                oy = min(oy, (int64_t)floor(b->position_double->y));
                oz = min(oz, (int64_t)floor(b->position_double->z));
            }
            pack_block_payload(buffer, offset, blocks, ox, oy, oz);
            return offset;
        }

        static Schemata unpack(uint8_t* buffer) {
            Schemata s;
            uint32_t offset = 0;
            BitUtil::unpack(buffer, offset, TYPE_TYPE_LENGTH);
            BitUtil::unpack(buffer, offset, LONG_TYPE_LENGTH);
            s.schemata_id = unpack_id(buffer, offset);
            s.num_blocks = BitUtil::unpack(buffer, offset, MAX_SCHEMATA_BLOCKS_BITS);
            if (s.num_blocks > 0) {
                s.blocks = unpack_block_payload(buffer, offset, s.num_blocks, 0, 0, 0); // relative to origin
            }
            return s;
        }
    };


    /**
     * A list of blocks/removed blocks (removed block denoted with a 'VOID' material),
     * and schemata placements.
     * 
     * 3*PNEG_CHUNK_1024_OFFSET_BITS for chunk position
     * 
     * 30 bits (1024^3) for how many blocks
     * 
     * blocks (removed block denoted with a 'VOID' material)
     * - 1 bit for BIT_PACKED vs RANGE_COMPRESSED (see Block Storage)
     * - variable number of bits from the block storage type
     * 
     * schemata placements (11 bit count prefix so the message is self-framing)
     * - ID_LENGTH_BITS + (one of the tiers) bits for schemata id
     * - 3*LOCAL_POSITION_BITS + 2 bits for rotation NEWS
     * 
     */
    struct Blocks {
        struct SchemataPlacement {
            uint64_t schemata_id;        // tiered id
            uint16_t x;                  // LOCAL_POSITION_BITS
            uint16_t y;                  // LOCAL_POSITION_BITS
            uint16_t z;                  // LOCAL_POSITION_BITS
            uint8_t rotation;            // 2 bits NEWS
        };

        RequestType type = RequestType::BLOCKS;
        int64_t chunk_x;                 // PNEG 54-bit each
        int64_t chunk_y;
        int64_t chunk_z;
        uint32_t num_blocks;             // 30 bits
        vector<shared_ptr<Block>> blocks; // positions relative to chunk origin
        uint32_t num_placements;         // 11 bits
        vector<SchemataPlacement> placements;

        uint32_t pack(uint8_t* buffer) {
            uint32_t offset = 0;
            BitUtil::pack(buffer, offset, LONG_TYPE, TYPE_TYPE_LENGTH);
            BitUtil::pack(buffer, offset, (uint8_t)type, LONG_TYPE_LENGTH);
            BitUtil::pack(buffer, offset, pack_pneg(chunk_x), PNEG_CHUNK_1024_OFFSET_BITS);
            BitUtil::pack(buffer, offset, pack_pneg(chunk_y), PNEG_CHUNK_1024_OFFSET_BITS);
            BitUtil::pack(buffer, offset, pack_pneg(chunk_z), PNEG_CHUNK_1024_OFFSET_BITS);
            num_blocks = blocks.size();
            BitUtil::pack(buffer, offset, num_blocks, 30);
            pack_block_payload(buffer, offset, blocks, chunk_x, chunk_y, chunk_z);
            num_placements = placements.size();
            BitUtil::pack(buffer, offset, num_placements, 11);
            for (auto& p : placements) {
                pack_id(buffer, offset, p.schemata_id);
                BitUtil::pack(buffer, offset, p.x, LOCAL_POSITION_BITS);
                BitUtil::pack(buffer, offset, p.y, LOCAL_POSITION_BITS);
                BitUtil::pack(buffer, offset, p.z, LOCAL_POSITION_BITS);
                BitUtil::pack(buffer, offset, p.rotation, 2);
            }
            return offset;
        }

        static Blocks unpack(uint8_t* buffer) {
            Blocks b;
            uint32_t offset = 0;
            BitUtil::unpack(buffer, offset, TYPE_TYPE_LENGTH);
            BitUtil::unpack(buffer, offset, LONG_TYPE_LENGTH);
            b.chunk_x = unpack_pneg(BitUtil::unpack(buffer, offset, PNEG_CHUNK_1024_OFFSET_BITS));
            b.chunk_y = unpack_pneg(BitUtil::unpack(buffer, offset, PNEG_CHUNK_1024_OFFSET_BITS));
            b.chunk_z = unpack_pneg(BitUtil::unpack(buffer, offset, PNEG_CHUNK_1024_OFFSET_BITS));
            b.num_blocks = BitUtil::unpack(buffer, offset, 30);
            b.blocks = unpack_block_payload(buffer, offset, b.num_blocks, b.chunk_x, b.chunk_y, b.chunk_z);
            uint32_t count = BitUtil::unpack(buffer, offset, 11);
            b.num_placements = count;
            b.placements.resize(count);
            for (uint32_t i = 0; i < count; i++) {
                b.placements[i].schemata_id = unpack_id(buffer, offset);
                b.placements[i].x = BitUtil::unpack(buffer, offset, LOCAL_POSITION_BITS);
                b.placements[i].y = BitUtil::unpack(buffer, offset, LOCAL_POSITION_BITS);
                b.placements[i].z = BitUtil::unpack(buffer, offset, LOCAL_POSITION_BITS);
                b.placements[i].rotation = BitUtil::unpack(buffer, offset, 2);
            }
            return b;
        }
    };


    /**
     * A vehicle's data sent to the client once. All objects in space are vehicles (even if they're like a city/space station or something)
     * 
     * 3*PNEG_CHUNK_1024_OFFSET_BITS for chunk position
     * 
     * 30 bits (1024^3) for how many blocks
     * 
     * blocks (removed block denoted with a 'VOID' material)
     * - 1 bit for BIT_PACKED vs RANGE_COMPRESSED (see Block Storage)
     * - variable number of bits from the block storage type
     * 
     * schemata placements (11 bit count prefix so the message is self-framing)
     * - ID_LENGTH_BITS + (one of the tiers) bits for schemata id
     * - 3*LOCAL_POSITION_BITS + 2 bits for rotation NEWS
     * 
     * then n bits for vehicle meta data that'll be serialized in VehicleGen.hpp
     * (8 bit length prefix so the message is self-framing)
     * 
     */
    struct Vehicle {
        RequestType type = RequestType::VEHICLE;
        int64_t chunk_x;                 // PNEG 54-bit each
        int64_t chunk_y;
        int64_t chunk_z;
        uint32_t num_blocks;             // 30 bits
        vector<shared_ptr<Block>> blocks; // positions relative to chunk origin
        uint32_t num_placements;         // 11 bits
        vector<Blocks::SchemataPlacement> placements;
        vector<uint8_t> meta_data;       // vehicle meta serialized by VehicleGen.hpp

        uint32_t pack(uint8_t* buffer) {
            uint32_t offset = 0;
            BitUtil::pack(buffer, offset, LONG_TYPE, TYPE_TYPE_LENGTH);
            BitUtil::pack(buffer, offset, (uint8_t)type, LONG_TYPE_LENGTH);
            BitUtil::pack(buffer, offset, pack_pneg(chunk_x), PNEG_CHUNK_1024_OFFSET_BITS);
            BitUtil::pack(buffer, offset, pack_pneg(chunk_y), PNEG_CHUNK_1024_OFFSET_BITS);
            BitUtil::pack(buffer, offset, pack_pneg(chunk_z), PNEG_CHUNK_1024_OFFSET_BITS);
            num_blocks = blocks.size();
            BitUtil::pack(buffer, offset, num_blocks, 30);
            pack_block_payload(buffer, offset, blocks, chunk_x, chunk_y, chunk_z);
            num_placements = placements.size();
            BitUtil::pack(buffer, offset, num_placements, 11);
            for (auto& p : placements) {
                pack_id(buffer, offset, p.schemata_id);
                BitUtil::pack(buffer, offset, p.x, LOCAL_POSITION_BITS);
                BitUtil::pack(buffer, offset, p.y, LOCAL_POSITION_BITS);
                BitUtil::pack(buffer, offset, p.z, LOCAL_POSITION_BITS);
                BitUtil::pack(buffer, offset, p.rotation, 2);
            }
            BitUtil::pack(buffer, offset, meta_data.size(), 8);
            for (uint8_t byte : meta_data) {
                BitUtil::pack(buffer, offset, byte, 8);
            }
            return offset;
        }

        static Vehicle unpack(uint8_t* buffer) {
            Vehicle v;
            uint32_t offset = 0;
            BitUtil::unpack(buffer, offset, TYPE_TYPE_LENGTH);
            BitUtil::unpack(buffer, offset, LONG_TYPE_LENGTH);
            v.chunk_x = unpack_pneg(BitUtil::unpack(buffer, offset, PNEG_CHUNK_1024_OFFSET_BITS));
            v.chunk_y = unpack_pneg(BitUtil::unpack(buffer, offset, PNEG_CHUNK_1024_OFFSET_BITS));
            v.chunk_z = unpack_pneg(BitUtil::unpack(buffer, offset, PNEG_CHUNK_1024_OFFSET_BITS));
            v.num_blocks = BitUtil::unpack(buffer, offset, 30);
            v.blocks = unpack_block_payload(buffer, offset, v.num_blocks, v.chunk_x, v.chunk_y, v.chunk_z);
            uint32_t count = BitUtil::unpack(buffer, offset, 11);
            v.num_placements = count;
            v.placements.resize(count);
            for (uint32_t i = 0; i < count; i++) {
                v.placements[i].schemata_id = unpack_id(buffer, offset);
                v.placements[i].x = BitUtil::unpack(buffer, offset, LOCAL_POSITION_BITS);
                v.placements[i].y = BitUtil::unpack(buffer, offset, LOCAL_POSITION_BITS);
                v.placements[i].z = BitUtil::unpack(buffer, offset, LOCAL_POSITION_BITS);
                v.placements[i].rotation = BitUtil::unpack(buffer, offset, 2);
            }
            uint32_t meta_len = BitUtil::unpack(buffer, offset, 8);
            v.meta_data.resize(meta_len);
            for (uint32_t i = 0; i < meta_len; i++) {
                v.meta_data[i] = BitUtil::unpack(buffer, offset, 8);
            }
            return v;
        }
    };

    /**
     * Placement of a block/removing a block (removal denoted with a 'VOID' material),
     * or placement of a schemata.
     * 
     * 1 bit for terrain/vehicle
     * 
     * 1 bit for block/schemata
     * 
     * if schemata
     * - ID_LENGTH_BITS + (one of the tiers) for id
     * - 2 bits for rotation NEWS
     * 
     * if terrain
     * - 3*PNEG_CHUNK_1024_OFFSET_BITS for the position
     * - MATERIAL_BITS for the material
     * 
     * if vehicle
     * - ID_LENGTH_BITS + (one of the tiers) for id
     * - 3*PNEG_CHUNK_1024_OFFSET_BITS vehicle chunk
     * - 3*LOCAL_POSITION_BITS
     * 
     */
    struct Placement {
        RequestType type = RequestType::PLACEMENT;
        bool vehicle;                   // 1 bit
        bool schemata;                  // 1 bit
        // schemata
        uint64_t schemata_id;           // tiered id
        uint8_t rotation;               // 2 bits NEWS
        // terrain block (block coords, not chunk index)
        int64_t pos_x;                  // PNEG 54-bit each
        int64_t pos_y;
        int64_t pos_z;
        uint16_t material;              // MATERIAL_BITS
        // vehicle
        uint64_t vehicle_id;            // tiered id
        int64_t vchunk_x;               // PNEG 54-bit each
        int64_t vchunk_y;
        int64_t vchunk_z;
        uint16_t vx;                    // LOCAL_POSITION_BITS each
        uint16_t vy;
        uint16_t vz;

        uint32_t pack(uint8_t* buffer) {
            uint32_t offset = 0;
            BitUtil::pack(buffer, offset, LONG_TYPE, TYPE_TYPE_LENGTH);
            BitUtil::pack(buffer, offset, (uint8_t)type, LONG_TYPE_LENGTH);
            BitUtil::pack(buffer, offset, vehicle ? 1 : 0, 1);
            BitUtil::pack(buffer, offset, schemata ? 1 : 0, 1);
            if (schemata) {
                pack_id(buffer, offset, schemata_id);
                BitUtil::pack(buffer, offset, rotation, 2);
                return offset;
            }
            if (vehicle) {
                pack_id(buffer, offset, vehicle_id);
                BitUtil::pack(buffer, offset, pack_pneg(vchunk_x), PNEG_CHUNK_1024_OFFSET_BITS);
                BitUtil::pack(buffer, offset, pack_pneg(vchunk_y), PNEG_CHUNK_1024_OFFSET_BITS);
                BitUtil::pack(buffer, offset, pack_pneg(vchunk_z), PNEG_CHUNK_1024_OFFSET_BITS);
                BitUtil::pack(buffer, offset, vx, LOCAL_POSITION_BITS);
                BitUtil::pack(buffer, offset, vy, LOCAL_POSITION_BITS);
                BitUtil::pack(buffer, offset, vz, LOCAL_POSITION_BITS);
                return offset;
            }
            BitUtil::pack(buffer, offset, pack_pneg(pos_x), PNEG_CHUNK_1024_OFFSET_BITS);
            BitUtil::pack(buffer, offset, pack_pneg(pos_y), PNEG_CHUNK_1024_OFFSET_BITS);
            BitUtil::pack(buffer, offset, pack_pneg(pos_z), PNEG_CHUNK_1024_OFFSET_BITS);
            BitUtil::pack(buffer, offset, material, MATERIAL_BITS);
            return offset;
        }

        static Placement unpack(uint8_t* buffer) {
            Placement p;
            uint32_t offset = 0;
            BitUtil::unpack(buffer, offset, TYPE_TYPE_LENGTH);
            BitUtil::unpack(buffer, offset, LONG_TYPE_LENGTH);
            p.vehicle = BitUtil::unpack(buffer, offset, 1) != 0;
            p.schemata = BitUtil::unpack(buffer, offset, 1) != 0;
            if (p.schemata) {
                p.schemata_id = unpack_id(buffer, offset);
                p.rotation = BitUtil::unpack(buffer, offset, 2);
                return p;
            }
            if (p.vehicle) {
                p.vehicle_id = unpack_id(buffer, offset);
                p.vchunk_x = unpack_pneg(BitUtil::unpack(buffer, offset, PNEG_CHUNK_1024_OFFSET_BITS));
                p.vchunk_y = unpack_pneg(BitUtil::unpack(buffer, offset, PNEG_CHUNK_1024_OFFSET_BITS));
                p.vchunk_z = unpack_pneg(BitUtil::unpack(buffer, offset, PNEG_CHUNK_1024_OFFSET_BITS));
                p.vx = BitUtil::unpack(buffer, offset, LOCAL_POSITION_BITS);
                p.vy = BitUtil::unpack(buffer, offset, LOCAL_POSITION_BITS);
                p.vz = BitUtil::unpack(buffer, offset, LOCAL_POSITION_BITS);
                return p;
            }
            p.pos_x = unpack_pneg(BitUtil::unpack(buffer, offset, PNEG_CHUNK_1024_OFFSET_BITS));
            p.pos_y = unpack_pneg(BitUtil::unpack(buffer, offset, PNEG_CHUNK_1024_OFFSET_BITS));
            p.pos_z = unpack_pneg(BitUtil::unpack(buffer, offset, PNEG_CHUNK_1024_OFFSET_BITS));
            p.material = BitUtil::unpack(buffer, offset, MATERIAL_BITS);
            return p;
        }
    };



    /**
     * A annoucement of a new player, npc, or falling object to the client, or a message detailing the object is now at
     * rest at it's last position.
     * 
     * ID_LENGTH_BITS + (one of the tiers) bits for object id
     * 3 bits entity type
     * 64 bit seed if npc or generated vehicle, this is used if block data is empty
     * n bits of block data if applicable
     * 
     * 
     */
    struct NewEntity {

        enum class EntityType {
            PLAYER,
            NPC,
            VEHICLE,
            FALLING_OBJECT
        };

        RequestType type = RequestType::NEW_ENTITY;
        uint64_t id;
        EntityType entity_type;
        int64_t seed;
        shared_ptr<BlockStorage::StorageChunk> block_data;
    };


    static void get_type(const char* message, RequestType& type) {
        uint32_t bit_offset = 0;
        uint8_t flag = BitUtil::unpack((uint8_t*)message, bit_offset, 1);
        if (flag) {
            type = (RequestType)BitUtil::unpack((uint8_t*)message, bit_offset, 8);
        } else {
            type = (RequestType)BitUtil::unpack((uint8_t*)message, bit_offset, 2);
        }
    }

    /**
     * Returns the byte length of the message at `message`, or 0 if not enough
     * of it is buffered yet to know the length. -1 on a malformed header.
     * Fixed-size messages know their length from type alone (zero framing
     * overhead); variable-size messages embed their own size fields which we
     * parse here. Used to frame TCP messages so connections can be reused
     * even when reads are split or coalesced.
     */
    static int message_length(const char* message, int available) {
        uint32_t offset = 0;
        uint64_t avail_bits = (uint64_t)available * 8;

        // bit reader that fails instead of reading past the buffer
        auto take = [&](uint32_t bits, uint64_t& out) -> bool {
            if (offset + bits > avail_bits) return false;
            out = BitUtil::unpack((uint8_t*)message, offset, bits); // advances offset
            return true;
        };
        auto skip = [&](uint64_t bits) -> bool {
            if ((uint64_t)offset + bits > avail_bits) return false;
            offset += (uint32_t)bits;
            return true;
        };
        auto to_bytes = [](uint64_t bits) -> int {
            return (int)((bits + 7) / 8);
        };
        // skip a tiered id: 2 bit tier + id in that tier's bit width
        auto skip_id = [&]() -> bool {
            uint64_t tier = 0;
            if (!take(ID_LENGTH_BITS, tier)) return false;
            return skip(id_tier_bits((uint8_t)tier));
        };

        uint64_t flag = 0;
        if (!take(1, flag)) return 0;

        if (flag) {
            // long type, 8 bit encoding
            uint64_t t = 0;
            if (!take(8, t)) return 0;
            switch ((RequestType)t) {
                case RequestType::PING: return Ping::BYTES;
                case RequestType::LOGOUT: return Logout::BYTES;
                case RequestType::AUTH: return Auth::BYTES;
                case RequestType::STOP: return Stop::BYTES;
                case RequestType::DEAD: {
                    if (!skip_id()) return 0;
                    return to_bytes(offset);
                }
                case RequestType::REGISTER: {
                    uint64_t pw_len = 0;
                    if (!take(5, pw_len)) return 0;
                    if (!skip(pw_len * 8)) return 0;
                    uint64_t num_captchas = 0;
                    if (!take(4, num_captchas)) return 0;
                    for (uint64_t i = 0; i < num_captchas; i++) {
                        uint64_t pid = 0;
                        if (!take(64, pid)) return 0;
                        uint64_t num_answers = 0;
                        if (!take(4, num_answers)) return 0;
                        if (!skip(num_answers * 4)) return 0;
                    }
                    return to_bytes(offset);
                }
                case RequestType::LOGIN: {
                    uint64_t id_len = 0;
                    if (!take(6, id_len)) return 0;
                    if (!skip(id_len * 8)) return 0;
                    uint64_t pw_len = 0;
                    if (!take(5, pw_len)) return 0;
                    if (!skip(pw_len * 8)) return 0;
                    uint64_t num_captchas = 0;
                    if (!take(4, num_captchas)) return 0;
                    for (uint64_t i = 0; i < num_captchas; i++) {
                        uint64_t pid = 0;
                        if (!take(64, pid)) return 0;
                        uint64_t num_answers = 0;
                        if (!take(4, num_answers)) return 0;
                        if (!skip(num_answers * 4)) return 0;
                    }
                    return to_bytes(offset);
                }
                case RequestType::PUZZLE: {
                    uint64_t puzzle_id = 0;
                    if (!take(64, puzzle_id)) return 0;
                    uint64_t image_size = 0;
                    if (!take(18, image_size)) return 0;
                    if (!skip(image_size * 8)) return 0;
                    return to_bytes(offset);
                }
                case RequestType::PASSWORD_CHANGE: {
                    if (!skip(128)) return 0;
                    uint64_t old_len = 0;
                    if (!take(5, old_len)) return 0;
                    if (!skip(old_len * 8)) return 0;
                    uint64_t new_len = 0;
                    if (!take(5, new_len)) return 0;
                    if (!skip(new_len * 8)) return 0;
                    return to_bytes(offset);
                }
                case RequestType::POSITION_UPDATE: {
                    uint64_t space = 0;
                    if (!take(1, space)) return 0;
                    if (!space) {
                        if (!skip(63 * 3)) return 0;
                        return to_bytes(offset);
                    }
                    uint64_t change = 0;
                    if (!take(2, change)) return 0;
                    if (change == 0) {
                        if (!skip(64 * 3)) return 0;
                        if (!skip(14 * 3)) return 0;
                    } else if (change == 1) {
                        if (!skip(14 * 3)) return 0;
                    }
                    if (!skip(KM_BITS_PER_LY * 3)) return 0;
                    return to_bytes(offset);
                }
                case RequestType::RESYNC: {
                    uint64_t count = 0;
                    if (!take(11, count)) return 0;
                    for (uint64_t i = 0; i < count; i++) {
                        if (!skip_id()) return 0;
                        if (!skip(1)) return 0;
                    }
                    return to_bytes(offset);
                }
                case RequestType::SCHEMATA: {
                    if (!skip_id()) return 0;
                    uint64_t num_blocks = 0;
                    if (!take(MAX_SCHEMATA_BLOCKS_BITS, num_blocks)) return 0;
                    if (!skip_block_payload(message, offset, (uint32_t)avail_bits, num_blocks)) return 0;
                    return to_bytes(offset);
                }
                case RequestType::BLOCKS: {
                    if (!skip(PNEG_CHUNK_1024_OFFSET_BITS * 3)) return 0;
                    uint64_t num_blocks = 0;
                    if (!take(30, num_blocks)) return 0;
                    if (!skip_block_payload(message, offset, (uint32_t)avail_bits, num_blocks)) return 0;
                    uint64_t num_placements = 0;
                    if (!take(11, num_placements)) return 0;
                    for (uint64_t i = 0; i < num_placements; i++) {
                        if (!skip_id()) return 0;
                        if (!skip(3 * LOCAL_POSITION_BITS + 2)) return 0;
                    }
                    return to_bytes(offset);
                }
                case RequestType::VEHICLE: {
                    if (!skip(PNEG_CHUNK_1024_OFFSET_BITS * 3)) return 0;
                    uint64_t num_blocks = 0;
                    if (!take(30, num_blocks)) return 0;
                    if (!skip_block_payload(message, offset, (uint32_t)avail_bits, num_blocks)) return 0;
                    uint64_t num_placements = 0;
                    if (!take(11, num_placements)) return 0;
                    for (uint64_t i = 0; i < num_placements; i++) {
                        if (!skip_id()) return 0;
                        if (!skip(3 * LOCAL_POSITION_BITS + 2)) return 0;
                    }
                    uint64_t meta_len = 0;
                    if (!take(8, meta_len)) return 0;
                    if (!skip(meta_len * 8)) return 0;
                    return to_bytes(offset);
                }
                case RequestType::PLACEMENT: {
                    uint64_t vehicle = 0;
                    if (!take(1, vehicle)) return 0;
                    uint64_t schemata = 0;
                    if (!take(1, schemata)) return 0;
                    if (schemata) {
                        if (!skip_id()) return 0;
                        if (!skip(2)) return 0;
                        return to_bytes(offset);
                    }
                    if (vehicle) {
                        if (!skip_id()) return 0;
                        if (!skip(PNEG_CHUNK_1024_OFFSET_BITS * 3 + 3 * LOCAL_POSITION_BITS)) return 0;
                        return to_bytes(offset);
                    }
                    if (!skip(PNEG_CHUNK_1024_OFFSET_BITS * 3 + MATERIAL_BITS)) return 0;
                    return to_bytes(offset);
                }
                default:
                    return -1;
            }
        }

        // short type, 2 bit encoding
        uint64_t t = 0;
        if (!take(2, t)) return 0;
        switch ((RequestType)t) {
            case RequestType::CLIENT_LOCAL_POSITION_UPDATE: return ClientLocalPositionUpdate::BYTES;
            case RequestType::HIT: {
                if (!skip(HIT_VELOCITY_BITS)) return 0;
                if (!skip_id()) return 0;
                return to_bytes(offset);
            }
            case RequestType::ATTACK: {
                if (!skip_id()) return 0;
                uint64_t angle = 0;
                if (!take(1, angle)) return 0;
                if (angle) {
                    if (!skip(3 * ROTATION_BITS)) return 0;
                }
                return to_bytes(offset);
            }
            case RequestType::BATCH_POSITION_UPDATE: {
                uint64_t count = 0;
                if (!take(11, count)) return 0;
                for (uint64_t i = 0; i < count; i++) {
                    if (!skip_id()) return 0;
                    if (!skip(3 * LOCAL_POSITION_BITS + 3 * ROTATION_BITS)) return 0;
                }
                return to_bytes(offset);
            }
            default:
                return -1;
        }
    }


}
