#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "fountain.h"
#include "utils.h"

/* packet = block + seed + CRC-8
    * block: block_size bytes
    * seed: seed_size bytes
    * CRC-8: 1 byte
    */

static uint32_t seed_size = 4;

Data encode(uint8_t* real_data_ptr, uint32_t real_data_size, uint32_t block_size, 
            uint32_t packet_cnt) {
    uint32_t block_cnt = real_data_size / block_size;
    // 写缓冲区大小为 (编码块大小 + 种子大小 + CRC-8) * 编码包数量
    uint32_t packet_size = block_size + seed_size + 1;
    uint8_t* write_data_ptr = (uint8_t*)calloc(packet_size * packet_cnt, sizeof(uint8_t));
    if (!write_data_ptr) {
        perror("Calloc write buffer error");
        exit(EXIT_FAILURE);
    }

    std::vector<double> probs = gen_probs(block_cnt, sqrt(block_cnt), 0.05);
    for (uint32_t i = 0; i < packet_cnt; ++i) {
        // mt19937 算法生成的随机数质量足够好，不太需要将 seed 分散
        uint32_t seed = i;
        uint32_t degree = calc_degree(seed, block_cnt, probs);
        std::set<uint32_t> indexes = gen_indexes(seed, degree, block_cnt);

        uint8_t* block_ptr = (uint8_t*)calloc(block_size, sizeof(uint8_t));
        for (auto index : indexes) {
            for (uint32_t j = 0; j < block_size; ++j) {
                block_ptr[j] ^= real_data_ptr[index * block_size + j];
            }
        }

        uint8_t* write_data_tmp_ptr = write_data_ptr + i * packet_size;
        memcpy(write_data_tmp_ptr, block_ptr, block_size);
        free(block_ptr);
        memcpy(write_data_tmp_ptr + block_size, &seed, seed_size);
        write_data_tmp_ptr[block_size + seed_size] = calc_crc(write_data_tmp_ptr, 
                                                              block_size + seed_size);
    }
    Data write_data = {write_data_ptr, packet_size * packet_cnt};
    return write_data;
}

Data decode(uint8_t* encode_data_ptr, uint32_t encode_data_size, uint32_t block_size, 
            uint32_t raw_data_size) {
    uint32_t packet_size = block_size + seed_size + 1;
    uint32_t packet_cnt = encode_data_size / packet_size;
    // block_cnt 是原始文件对齐后按照 block_size 划分的块数
    uint32_t block_cnt = raw_data_size / block_size;
    if (raw_data_size % block_size != 0) {
        block_cnt++;
    }
    uint8_t* decode_data_ptr = (uint8_t*)calloc(block_size * block_cnt, sizeof(uint8_t));
    if (!decode_data_ptr) {
        perror("Calloc decode buffer error");
        exit(EXIT_FAILURE);
    }

    // Belief Propagation 解码
    std::vector<double> probs = gen_probs(block_cnt, sqrt(block_cnt), 0.05);
    std::vector<bool> is_decoded(block_cnt, false);
    bool flag = true;
    while (flag) {
        flag = false;
        for (uint32_t i = 0; i < packet_cnt; ++i) {
            uint8_t crc = calc_crc(encode_data_ptr + i * packet_size, block_size + seed_size);
            if (crc != encode_data_ptr[i * packet_size + block_size + seed_size]) {
                continue;
            }

            uint32_t seed = *(uint32_t*)(encode_data_ptr + i * packet_size + block_size);
            uint32_t degree = calc_degree(seed, block_cnt, probs);
            std::set<uint32_t> indexes = gen_indexes(seed, degree, block_cnt);
            for (auto index : indexes) {
                if (is_decoded[index]) {
                    degree--;
                }
            }
            if (degree == 1) {
                uint8_t* block_ptr = (uint8_t*)calloc(block_size, sizeof(uint8_t));
                memcpy(block_ptr, encode_data_ptr + i * packet_size, block_size);
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
        }
    }
    Data decode_data = {decode_data_ptr, block_size * block_cnt};
    return decode_data;
}
