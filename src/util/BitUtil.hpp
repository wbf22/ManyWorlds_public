#pragma once


using namespace std;


struct BitUtil {

    /**
     * packs the provided bits into the array and the specified offset, updating the offset
     * 
     * the array can even be a cast uint*_t of any size like so:
     * 
     * 
        ```
        uint64_t buf = 0;
        uint32_t offset = 0;
        BitUtil::pack((uint8_t*)&buf, offset, bits, num_bits);
        ```
     */
    static void pack(uint8_t* array, uint32_t& bit_offset, uint64_t bits, uint8_t num_bits) {
        uint32_t byte_idx = bit_offset >> 3;
        uint8_t bit_idx = bit_offset & 7;
        bit_offset += num_bits;

        while (num_bits > 0) {
            uint8_t bits_this_byte = 8 - bit_idx;
            if (bits_this_byte > num_bits) bits_this_byte = num_bits;

            uint8_t mask = (1 << bits_this_byte) - 1;
            array[byte_idx] = (array[byte_idx] & ~(mask << bit_idx)) | ((uint8_t)(bits & mask) << bit_idx);

            bits >>= bits_this_byte;
            num_bits -= bits_this_byte;
            byte_idx++;
            bit_idx = 0;
        }
    }

    /**
     * Unpacks num bits off the end of the array and updates the bit offset
     * 
     * the array can even be a cast uint*_t of any size like so:
     * 
     * 
        ```
        uint64_t buf = 0;
        uint32_t offset = 0;
        uint64_t val = BitUtil::unpack((uint8_t*)&buf, offset, num_bits);
        ```
     */
    static uint64_t unpack(uint8_t* array, uint32_t& bit_offset, uint8_t num_bits) {
        uint32_t byte_idx = bit_offset >> 3;
        uint8_t bit_idx = bit_offset & 7;
        bit_offset += num_bits;
        uint64_t result = 0;
        uint8_t shift = 0;

        while (num_bits > 0) {
            uint8_t bits_this_byte = 8 - bit_idx;
            if (bits_this_byte > num_bits) bits_this_byte = num_bits;

            uint8_t mask = (1 << bits_this_byte) - 1;
            result |= (uint64_t)((array[byte_idx] >> bit_idx) & mask) << shift;

            shift += bits_this_byte;
            num_bits -= bits_this_byte;
            byte_idx++;
            bit_idx = 0;
        }

        return result;
    }


    /**
     * Prints the binary of an array with labels underneath.
     * Blue coloring at byte boundaries, gaps between labeled fields.
     * 
     * Each label is {name, bit_width} — start positions are sequential.
     * Call: print(bits, len, {{"name1", width1}, {"name2", width2}});
     */
    static void print(uint8_t* array, uint32_t array_length, initializer_list<pair<string, uint16_t>> labels) {
        size_t N = labels.size();
        uint32_t total_bits = array_length * 8;

        vector<uint16_t> starts(N), widths(N);
        vector<string> names(N);
        uint16_t running = 0;
        size_t idx = 0;
        for (auto& label : labels) {
            starts[idx] = running;
            widths[idx] = label.second;
            names[idx] = label.first;
            running += widths[idx];
            idx++;
        }

        vector<uint16_t> ends(N);
        for (size_t i = 0; i < N; i++) {
            uint16_t end = starts[i] + widths[i];
            ends[i] = end < total_bits ? end : total_bits;
        }

        // First pass: visible character positions for field boundaries
        vector<int> field_start_pos(N), field_end_pos(N);
        int vis_pos = 0;
        for (size_t f = 0; f < N; f++) {
            if (f > 0) vis_pos += 2;
            field_start_pos[f] = vis_pos;
            for (uint16_t b = starts[f]; b < ends[f]; b++) {
                vis_pos++;
                if (b % 8 == 7 && b + 1 < ends[f])
                    vis_pos++;
            }
            field_end_pos[f] = vis_pos;
        }

        // Binary line: blue at byte boundaries, gaps between fields
        for (size_t f = 0; f < N; f++) {
            if (f > 0) printf("  ");
            for (uint16_t b = starts[f]; b < ends[f]; b++) {
                uint32_t byte_idx = b / 8;
                uint8_t bit = (array[byte_idx] >> (b % 8)) & 1;
                if (b % 8 == 0) printf("\033[34m");
                printf("%d", bit);
                if (b % 8 == 0) printf("\033[0m");
                if (b % 8 == 7 && b + 1 < ends[f]) printf(" ");
            }
        }
        printf("\n");

        // Pipe separators at field boundaries
        int cur = 0;
        for (size_t f = 0; f < N; f++) {
            while (cur < field_start_pos[f]) { printf(" "); cur++; }
            printf("|");
            cur++;
        }
        printf("\n");

        // Field names centered in their spans
        cur = 0;
        for (size_t f = 0; f < N; f++) {
            int field_w = field_end_pos[f] - field_start_pos[f];
            int name_len = names[f].size();
            int pad_left = max(0, field_w - name_len) / 2;
            while (cur < field_start_pos[f] + pad_left) { printf(" "); cur++; }
            printf("%s", names[f].c_str());
            cur += names[f].size();
        }
        printf("\n");
    }


    template<typename T>
    static void convert_to_array(uint8_t* array, T bits) {
        memcpy(array, &bits, sizeof(T));
    }
};