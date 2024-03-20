#include "fountain.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <set>
#include <cmath>
#include <numeric>
#include <random>

// 种子长度可为 1 至 4 字节

std::vector<double> calc_probs_robust_soliton(u32 block_cnt, double R, double delta) {
    std::vector<double> probs(block_cnt + 1, 0.0);
    u32 critical_point = static_cast<u32>(std::ceil(block_cnt / R));
    double tau_critical = R * std::log(R / delta) / block_cnt;
    double Z = 0.0; // 归一化常数

    for (u32 d = 1; d <= block_cnt; ++d) {
        // 理想孤子部分
        if (d == 1) {
            probs[d] = 1.0 / block_cnt;
        } else {
            probs[d] = 1.0 / (d * (d - 1));
        }
        // 鲁棒孤子部分
        double tau = 0.0;
        if (d <= critical_point - 1) {
            tau = (R / block_cnt) / d;
        } else if (d == critical_point) {
            tau = tau_critical;
        }
        probs[d] += tau;
        Z += probs[d];
    }
    for (auto& prob : probs) {
        prob /= Z;
    }
    return probs;
}

u32 gen_degree(u32 seed, u32 block_cnt, const std::vector<double>& probs) {
    std::mt19937 gen_rand(seed);
    std::uniform_real_distribution<> uniform(0.0, 1.0);
    double rand_num = uniform(gen_rand);
    u32 degree = 1;
    double prob = probs[1];
    while (rand_num > prob && degree < block_cnt) {
        ++degree;
        prob += probs[degree];
    }
    return degree;
}

std::set<u32> gen_indexes(u32 seed, u32 degree, u32 block_cnt) {
    std::mt19937 gen_rand(seed);
    std::uniform_int_distribution<> uniform(0, block_cnt - 1);
    std::set<u32> indexes;
    while (indexes.size() < degree) {
        indexes.insert(uniform(gen_rand));
    }
    return indexes;
}

std::vector<u32> calc_packet_cnts(u32 packet_cnt) {
    std::vector<u32> packet_cnts(4, 0);
    if (packet_cnt <= 64) {
        // 只使用 Seed 中的 6 位
        packet_cnts[0] = packet_cnt;
    } else if (packet_cnt <= 16384) {
        // 新增 1 字节，6 + 8 = 14 位，以此类推
        packet_cnts[0] = 64;
        packet_cnts[1] = packet_cnt - packet_cnts[0];
    } else if (packet_cnt <= 4194304) {
        packet_cnts[0] = 64;
        packet_cnts[1] = 16384 - packet_cnts[0];
        packet_cnts[2] = packet_cnt - packet_cnts[0] - packet_cnts[1];
    } else {
        packet_cnts[0] = 64;
        packet_cnts[1] = 16384 - packet_cnts[0];
        packet_cnts[2] = 4194304 - packet_cnts[0] - packet_cnts[1];
        packet_cnts[3] = packet_cnt - packet_cnts[0] - packet_cnts[1] - packet_cnts[2];
    }
    return packet_cnts;
}

Data encode(u8* real_data_ptr, u32 real_data_size, u32 block_size, u32 packet_cnt) {
    std::vector<u32> packet_cnts = calc_packet_cnts(packet_cnt);
    u32 write_data_size = (block_size + sizeof(Seed)) * packet_cnts[0] +
                          (block_size + sizeof(Seed) + 1) * packet_cnts[1] +
                          (block_size + sizeof(Seed) + 2) * packet_cnts[2] +
                          (block_size + sizeof(Seed) + 3) * packet_cnts[3];
    u8* write_data_ptr = (u8*)calloc(write_data_size, sizeof(u8));
    if (!write_data_ptr) {
        perror("Calloc write buffer error");
        exit(EXIT_FAILURE);
    }

    u32 block_cnt = real_data_size / block_size;
    std::vector<double> probs = calc_probs_robust_soliton(block_cnt, std::sqrt(block_cnt), 0.05);
    u32 seed = 0;
    u8* write_data_tmp_ptr = write_data_ptr;
    for (u32 i = 0; i < 4; ++i) {
        for (u32 j = 0; j < packet_cnts[i]; ++j) {
            u32 degree = gen_degree(seed, block_cnt, probs);
            std::set<u32> indexes = gen_indexes(seed, degree, block_cnt);
            u8* block_ptr = (u8*)calloc(block_size, sizeof(u8));
            for (auto index : indexes) {
                for (u32 k = 0; k < block_size; ++k) {
                    block_ptr[k] ^= real_data_ptr[index * block_size + k];
                }
            }

            memcpy(write_data_tmp_ptr, block_ptr, block_size);
            free(block_ptr);
            write_data_tmp_ptr += block_size;
            Seed seed_head = {static_cast<u8>(i), static_cast<u8>(seed & 0x3F)};
            memcpy(write_data_tmp_ptr, &seed_head, sizeof(Seed));
            write_data_tmp_ptr += sizeof(Seed);
            for (u32 k = 0; k < i; ++k) {
                u8 seed_ext = static_cast<u8>(seed >> (6 + k * 8));
                memcpy(write_data_tmp_ptr, &seed_ext, sizeof(u8));
                write_data_tmp_ptr += sizeof(u8);
            }
            ++seed;
        }
    }
    Data write_data = {write_data_ptr, write_data_size};
    return write_data;
}

Data decode(u8* encode_data_ptr, u32 encode_data_size, u32 block_size, u32 raw_data_size) {
    u32 block_cnt = raw_data_size / block_size;
    if (raw_data_size % block_size != 0) {
        ++block_cnt;
    }
    u8* decode_data_ptr = (u8*)calloc(block_size * block_cnt, sizeof(u8));
    if (!decode_data_ptr) {
        perror("Calloc decode buffer error");
        exit(EXIT_FAILURE);
    }

    std::vector<double> probs = calc_probs_robust_soliton(block_cnt, std::sqrt(block_cnt), 0.05);
    std::vector<bool> is_decoded(block_cnt, false);
    bool flag = true;
    while (flag) {
        flag = false;
        u8* encode_data_tmp_ptr = encode_data_ptr;
        while (encode_data_tmp_ptr < encode_data_ptr + encode_data_size)  {
            Seed seed_head = *(Seed*)(encode_data_tmp_ptr + block_size);
            u32 seed_type = seed_head.type;
            u32 seed = seed_head.val;
            for (u32 i = 0; i < seed_type; ++i) {
                seed |= static_cast<u32>(*(encode_data_tmp_ptr + block_size + sizeof(Seed) + i))
                        << (6 + i * 8);
            }
            u32 degree = gen_degree(seed, block_cnt, probs);
            std::set<u32> indexes = gen_indexes(seed, degree, block_cnt);

            for (auto index : indexes) {
                if (is_decoded[index]) {
                    --degree;
                }
            }
            if (degree == 1) {
                u8* block_ptr = (u8*)calloc(block_size, sizeof(u8));
                memcpy(block_ptr, encode_data_tmp_ptr, block_size);
                u32 new_decode_index = 0;
                for (auto index : indexes) {
                    if (is_decoded[index]) {
                        for (u32 j = 0; j < block_size; ++j) {
                            block_ptr[j] ^= decode_data_ptr[index * block_size + j];
                        }
                    } else {
                        new_decode_index = index;
                    }
                }
                memcpy(decode_data_ptr + new_decode_index * block_size, block_ptr, block_size);
                is_decoded[new_decode_index] = true;
                free(block_ptr);
                flag = true;
            }
            encode_data_tmp_ptr += block_size + sizeof(Seed) + seed_type;
        }
    }
    Data decode_data = {decode_data_ptr, block_size * block_cnt};
    return decode_data;
}
