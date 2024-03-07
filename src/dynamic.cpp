#include "fountain.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <set>
#include <cmath>
#include <numeric>
#include <random>

// 种子长度可以为 1 字节、2 字节、4 字节

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

template <typename T>
u32 gen_degree(T seed, u32 block_cnt, const std::vector<double>& probs) {
    std::mt19937 gen_rand(static_cast<u32>(seed));
    std::uniform_real_distribution<> uniform(0.0, 1.0);
    double rand_num = uniform(gen_rand);
    u32 degree = 1;
    double prob = probs[1];
    while (rand_num > prob && degree < block_cnt) {
        degree++;
        prob += probs[degree];
    }
    return degree;
}

template <typename T>
std::set<u32> gen_indexes(T seed, u32 degree, u32 block_cnt) {
    std::mt19937 gen_rand(static_cast<u32>(seed));
    std::uniform_int_distribution<> uniform(0, block_cnt - 1);
    std::set<u32> indexes;
    while (indexes.size() < degree) {
        indexes.insert(uniform(gen_rand));
    }
    return indexes;
}

template <typename T>
void gen_packet(T seed, const std::vector<double>& probs, 
                u32 packet_pos, u32 packet_size, u8* packet_ptr, 
                u8* real_data_ptr, u32 real_data_size, u32 block_size) {
    u32 degree = gen_degree(seed, real_data_size / block_size, probs);
    std::set<u32> indexes = gen_indexes(seed, degree, real_data_size / block_size);

    u8* block_ptr = (u8*)calloc(block_size, sizeof(u8));
    for (auto index : indexes) {
        for (u32 j = 0; j < block_size; j++) {
            block_ptr[j] ^= real_data_ptr[index * block_size + j];
        }
    }
    u8 seed_size = sizeof(T);
    memcpy(packet_ptr + packet_pos * packet_size, block_ptr, block_size);
    memcpy(packet_ptr + packet_pos * packet_size + block_size, &seed_size, sizeof(u8));
    memcpy(packet_ptr + packet_pos * packet_size + block_size + sizeof(u8), &seed, seed_size);
    free(block_ptr);
}

Data encode(u8* real_data_ptr, u32 real_data_size, u32 block_size, u32 packet_cnt) {
    u32 packet_u8_cnt = 0, packet_u16_cnt = 0, packet_u32_cnt = 0;
    if (packet_cnt <= 256) {
        packet_u8_cnt = packet_cnt;
    } else if (packet_cnt <= 65536) {
        packet_u8_cnt = 256;
        packet_u16_cnt = packet_cnt - packet_u8_cnt;
    } else {
        packet_u8_cnt = 256;
        packet_u16_cnt = 65536 - packet_u8_cnt;
        packet_u32_cnt = packet_cnt - packet_u8_cnt - packet_u16_cnt;
    }

    u32 write_data_size = (block_size + sizeof(u8) + sizeof(u8)) * packet_u8_cnt + 
                          (block_size + sizeof(u8) + sizeof(u16)) * packet_u16_cnt + 
                          (block_size + sizeof(u8) + sizeof(u32)) * packet_u32_cnt;
    u8* write_data_ptr = (u8*)calloc(write_data_size, sizeof(u8));
    if (!write_data_ptr) {
        perror("Calloc write buffer error");
        exit(EXIT_FAILURE);
    }

    u32 block_cnt = real_data_size / block_size;
    std::vector<double> probs = calc_probs_robust_soliton(block_cnt, std::sqrt(block_cnt), 0.05);
    u8* write_data_tmp_ptr = write_data_ptr;
    for (u32 i = 0; i < packet_u8_cnt; i++) {
        u8 seed = i;
        gen_packet(seed, probs, i, block_size + sizeof(u8) + sizeof(u8), 
                   write_data_tmp_ptr, real_data_ptr, real_data_size, block_size);
    }
    write_data_tmp_ptr += packet_u8_cnt * (block_size + sizeof(u8) + sizeof(u8));
    for (u32 i = 0; i < packet_u16_cnt; i++) {
        u16 seed = i + packet_u8_cnt;
        gen_packet(seed, probs, i, block_size + sizeof(u8) + sizeof(u16), 
                   write_data_tmp_ptr, real_data_ptr, real_data_size, block_size);
    }
    write_data_tmp_ptr += packet_u16_cnt * (block_size + sizeof(u8) + sizeof(u16));
    for (u32 i = 0; i < packet_u32_cnt; i++) {
        u32 seed = i + packet_u8_cnt + packet_u16_cnt;
        gen_packet(seed, probs, i, block_size + sizeof(u8) + sizeof(u32), 
                   write_data_tmp_ptr, real_data_ptr, real_data_size, block_size);
    }
    Data write_data = {write_data_ptr, write_data_size};

    return write_data;
}

Data decode(u8* encode_data_ptr, u32 encode_data_size, u32 block_size, u32 raw_data_size) {
    u32 block_cnt = raw_data_size / block_size;
    if (raw_data_size % block_size != 0) {
        block_cnt++;
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
        while (encode_data_tmp_ptr < encode_data_ptr + encode_data_size) {
            u8 seed_size = *(encode_data_tmp_ptr + block_size);
            u32 seed = 0;
            if (seed_size == 1) {
                seed = *(encode_data_tmp_ptr + block_size + sizeof(u8));
            } else if (seed_size == 2) {
                seed = *(u16*)(encode_data_tmp_ptr + block_size + sizeof(u8));
            } else {
                seed = *(u32*)(encode_data_tmp_ptr + block_size + sizeof(u8));
            }
            u32 degree = gen_degree(seed, block_cnt, probs);
            std::set<u32> indexes = gen_indexes(seed, degree, block_cnt);

            for (auto index : indexes) {
                if (is_decoded[index]) {
                    degree--;
                }
            }
            if (degree == 1) {
                u8* block_ptr = (u8*)calloc(block_size, sizeof(u8));
                memcpy(block_ptr, encode_data_tmp_ptr, block_size);
                u32 new_decode_index = 0;
                for (auto index : indexes) {
                    if (is_decoded[index]) {
                        for (u32 j = 0; j < block_size; j++) {
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
            encode_data_tmp_ptr += block_size + sizeof(u8) + seed_size;
        }
    }
    Data decode_data = {decode_data_ptr, block_size * block_cnt};

    return decode_data;
}