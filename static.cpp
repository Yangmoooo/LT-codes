#include "fountain.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <set>
#include <cmath>
#include <numeric>
#include <random>

// 种子长度固定为 4 字节
static u32 seed_size = 4;

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
        degree++;
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

Data encode(u8* real_data_ptr, u32 real_data_size, u32 block_size, u32 packet_cnt) {
    u32 block_cnt = real_data_size / block_size;
    // 写缓冲区大小为 (编码块大小 + 种子大小) * 编码包数量
    u8* write_data_ptr = (u8*)calloc((block_size + seed_size) * packet_cnt, sizeof(u8));
    if (!write_data_ptr) {
        perror("Calloc write buffer error");
        exit(EXIT_FAILURE);
    }

    std::vector<double> probs = calc_probs_robust_soliton(block_cnt, std::sqrt(block_cnt), 0.05);
    for (u32 i = 0; i < packet_cnt; ++i) {
        // mt19937 算法生成的随机数质量足够好，不太需要将 seed 分散
        u32 seed = i;
        u32 degree = gen_degree(seed, block_cnt, probs);
        std::set<u32> indexes = gen_indexes(seed, degree, block_cnt);

        u8* block_ptr = (u8*)calloc(block_size, sizeof(u8));
        for (auto index : indexes) {
            for (u32 j = 0; j < block_size; ++j) {
                block_ptr[j] ^= real_data_ptr[index * block_size + j];
            }
        }

        memcpy(write_data_ptr + i * (block_size + seed_size), block_ptr, block_size);
        memcpy(write_data_ptr + i * (block_size + seed_size) + block_size, &seed, seed_size);
        free(block_ptr);
    }
    Data write_data = {write_data_ptr, (block_size + seed_size) * packet_cnt};

    return write_data;
}

Data decode(u8* encode_data_ptr, u32 encode_data_size, u32 block_size, u32 raw_data_size) {
    u32 packet_cnt = encode_data_size / (block_size + seed_size);
    // block_cnt 是原始文件对齐后按照 block_size 划分的块数
    u32 block_cnt = raw_data_size / block_size;
    if (raw_data_size % block_size != 0) {
        block_cnt++;
    }
    u8* decode_data_ptr = (u8*)calloc(block_size * block_cnt, sizeof(u8));
    if (!decode_data_ptr) {
        perror("Calloc decode buffer error");
        exit(EXIT_FAILURE);
    }

    // Belief Propagation 解码
    std::vector<double> probs = calc_probs_robust_soliton(block_cnt, std::sqrt(block_cnt), 0.05);
    std::vector<bool> is_decoded(block_cnt, false);
    bool flag = true;
    while (flag) {
        flag = false;
        for (u32 i = 0; i < packet_cnt; ++i) {
            u32 seed = *(u32*)(encode_data_ptr + i * (block_size + seed_size) + block_size);
            u32 degree = gen_degree(seed, block_cnt, probs);
            std::set<u32> indexes = gen_indexes(seed, degree, block_cnt);

            for (auto index : indexes) {
                if (is_decoded[index]) {
                    degree--;
                }
            }
            if (degree == 1) {
                u8* block_ptr = (u8*)calloc(block_size, sizeof(u8));
                memcpy(block_ptr, encode_data_ptr + i * (block_size + seed_size), block_size);
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
        }
    }
    Data decode_data = {decode_data_ptr, block_size * block_cnt};

    return decode_data;
}
