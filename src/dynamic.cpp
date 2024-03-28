#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "fountain.h"
#include "utils.h"

/* packet = block + seed + CRC-8
    * block: block_size bytes
    * seed: SeedHeader(1 byte) + seed_ext(0~3 bytes)
    * CRC-8: 1 byte
    */

void gen_packet_cnts(uint32_t packet_cnt, uint32_t packet_cnts[4]) {
    if (packet_cnt <= 64) {
        // 只使用 Seed 中的 6 位
        packet_cnts[0] = packet_cnt;
    } else if (packet_cnt <= 16384) {
        // 新增 1 字节，6 + 8 = 14 位，以此类推
        packet_cnts[0] = 64;
        packet_cnts[1] = packet_cnt - 64;
    } else if (packet_cnt <= 4194304) {
        packet_cnts[0] = 64;
        packet_cnts[1] = 16384 - 64;
        packet_cnts[2] = packet_cnt - 16384;
    } else {
        packet_cnts[0] = 64;
        packet_cnts[1] = 16384 - 64;
        packet_cnts[2] = 4194304 - 16384;
        packet_cnts[3] = packet_cnt - 4194304;
    }
}

Data encode(uint8_t* real_data_ptr, uint32_t real_data_size, uint32_t block_size, 
            uint32_t packet_cnt) {
    uint32_t packet_cnts[4] = {0};
    gen_packet_cnts(packet_cnt, packet_cnts);
    uint32_t write_data_size = 0;
    for (uint32_t i = 0; i < 4; ++i) {
        write_data_size += (block_size + i + 2) * packet_cnts[i];
    }
    uint8_t* write_data_ptr = (uint8_t*)calloc(write_data_size, sizeof(uint8_t));
    if (!write_data_ptr) {
        perror("Calloc write buffer error");
        exit(EXIT_FAILURE);
    }

    uint32_t block_cnt = real_data_size / block_size;
    std::vector<double> probs = calc_probs(block_cnt, sqrt(block_cnt), 0.05);
    uint32_t seed = 0;
    uint8_t* write_data_tmp_ptr = write_data_ptr;
    for (uint32_t i = 0; i < 4; ++i) {
        for (uint32_t j = 0; j < packet_cnts[i]; ++j) {
            uint32_t degree = gen_degree(seed, block_cnt, probs);
            std::set<uint32_t> indexes = gen_indexes(seed, degree, block_cnt);
            // 构造 block
            uint8_t* block_ptr = (uint8_t*)calloc(block_size, sizeof(uint8_t));
            for (auto index : indexes) {
                for (uint32_t k = 0; k < block_size; ++k) {
                    block_ptr[k] ^= real_data_ptr[index * block_size + k];
                }
            }
            memcpy(write_data_tmp_ptr, block_ptr, block_size);
            free(block_ptr);
            write_data_tmp_ptr += block_size;
            // 构造 seed
            SeedHeader seed_header = {static_cast<uint8_t>(i), 
                                      static_cast<uint8_t>(seed & 0x3F)};
            memcpy(write_data_tmp_ptr, &seed_header, sizeof(SeedHeader));
            write_data_tmp_ptr += sizeof(SeedHeader);
            for (uint32_t k = 0; k < i; ++k) {
                uint8_t seed_ext = static_cast<uint8_t>(seed >> (6 + k * 8));
                // memcpy(write_data_tmp_ptr, &seed_ext, sizeof(uint8_t));
                // write_data_tmp_ptr += sizeof(uint8_t);
                *write_data_tmp_ptr++ = seed_ext;
            }
            // 构造 CRC-8
            *write_data_tmp_ptr++ = calc_crc(write_data_tmp_ptr - block_size - 1 - i, 
                                             block_size + 1 + i);
            ++seed;
        }
    }
    Data write_data = {write_data_ptr, write_data_size};
    return write_data;
}

Data decode(uint8_t* encode_data_ptr, uint32_t encode_data_size, uint32_t block_size, 
            uint32_t raw_data_size) {
    uint32_t block_cnt = raw_data_size / block_size;
    if (raw_data_size % block_size != 0) {
        ++block_cnt;
    }
    uint8_t* decode_data_ptr = (uint8_t*)calloc(block_size * block_cnt, sizeof(uint8_t));
    if (!decode_data_ptr) {
        perror("Calloc decode buffer error");
        exit(EXIT_FAILURE);
    }

    std::vector<double> probs = calc_probs(block_cnt, sqrt(block_cnt), 0.05);
    std::vector<bool> is_decoded(block_cnt, false);
    bool flag = true;
    while (flag) {
        flag = false;
        uint8_t* encode_data_tmp_ptr = encode_data_ptr;
        while (encode_data_tmp_ptr < encode_data_ptr + encode_data_size)  {
            SeedHeader seed_header = *(SeedHeader*)(encode_data_tmp_ptr + block_size);
            uint32_t seed_type = seed_header.type;
            uint32_t payload_size = block_size + sizeof(SeedHeader) + seed_type;
            uint8_t crc = calc_crc(encode_data_tmp_ptr, payload_size);
            if (crc != *(encode_data_tmp_ptr + payload_size)) {
                encode_data_tmp_ptr += payload_size + 1;
                continue;
            }

            uint32_t seed = seed_header.val;
            for (uint32_t i = 0; i < seed_type; ++i) {
                seed |= static_cast<uint32_t>(*(encode_data_tmp_ptr + block_size + 1 + i))
                        << (6 + i * 8);
            }
            uint32_t degree = gen_degree(seed, block_cnt, probs);
            std::set<uint32_t> indexes = gen_indexes(seed, degree, block_cnt);
            for (auto index : indexes) {
                if (is_decoded[index]) {
                    --degree;
                }
            }
            if (degree == 1) {
                uint8_t* block_ptr = (uint8_t*)calloc(block_size, sizeof(uint8_t));
                memcpy(block_ptr, encode_data_tmp_ptr, block_size);
                uint32_t new_decode_index = 0;
                for (auto index : indexes) {
                    if (is_decoded[index]) {
                        for (uint32_t j = 0; j < block_size; ++j) {
                            block_ptr[j] ^= decode_data_ptr[index * block_size + j];
                        }
                    } else {
                        new_decode_index = index;
                    }
                }
                memcpy(decode_data_ptr + new_decode_index * block_size, block_ptr, 
                       block_size);
                is_decoded[new_decode_index] = true;
                free(block_ptr);
                flag = true;
            }
            encode_data_tmp_ptr += payload_size + 1;
        }
    }
    Data decode_data = {decode_data_ptr, block_size * block_cnt};
    return decode_data;
}
