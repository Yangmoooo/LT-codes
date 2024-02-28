#include "fountain.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <set>
#include <random>

// 种子长度固定为 4 字节
static u32 seed_size = 4;

u32 gen_degree_ideal_soliton(u32 seed, u32 block_cnt) {
    std::mt19937 gen_rand(seed);
    std::uniform_real_distribution<> uniform(0.0, 1.0);
    double rand_num = uniform(gen_rand);
    u32 degree = 1;
    double prob = 1.0 / block_cnt;
    while (rand_num > prob && degree < block_cnt) {
        degree++;
        prob += 1.0 / (degree * (degree - 1));
    }
    return degree;
}

std::set<u32> gen_indexes(u32 seed, u32 degree, u32 block_cnt) {
    std::mt19937 gen_rand(seed);
    std::uniform_int_distribution<> uniform(0, block_cnt - 1);
    std::set<u32> indexes;
    while (indexes.size() < degree)
        indexes.insert(uniform(gen_rand));
    return indexes;
}

Data encode(u8* real_data_ptr, u32 real_data_size, u32 block_size, u32 packet_cnt) {
    // 写缓冲区大小为 (编码块大小 + 种子大小) * 编码包数量
    u8* write_data_ptr = (u8*)calloc((block_size + seed_size) * packet_cnt, sizeof(u8));
    if (!write_data_ptr) {
        perror("Calloc write buffer error");
        exit(EXIT_FAILURE);
    }

    for (u32 i = 0; i < packet_cnt; i++) {
        // mt19937 算法生成的随机数质量足够好，不太需要将 seed 分散
        u32 seed = i;
        u32 degree = gen_degree_ideal_soliton(seed, real_data_size / block_size);
        std::set<u32> indexes = gen_indexes(seed, degree, real_data_size / block_size);

        u8* block_ptr = (u8*)calloc(block_size, sizeof(u8));
        for (auto index : indexes) {
            for (u32 j = 0; j < block_size; j++)
                block_ptr[j] ^= real_data_ptr[index * block_size + j];
        }

        memcpy(write_data_ptr + i * (block_size + seed_size), block_ptr, block_size);
        memcpy(write_data_ptr + i * (block_size + seed_size) + block_size, &seed, seed_size);
        free(block_ptr);
    }

    Data write_data;
    write_data.ptr = write_data_ptr;
    write_data.size = (block_size + seed_size) * packet_cnt;

    return write_data;
}

Data decode(u8* encode_data_ptr, u32 encode_data_size, u32 block_size, u32 raw_data_size) {
    u32 packet_cnt = encode_data_size / (block_size + seed_size);
    // block_cnt 是原始文件对齐后按照 block_size 划分的块数
    u32 block_cnt = raw_data_size / block_size;
    if (raw_data_size % block_size != 0)
        block_cnt++;
    u8* decode_data_ptr = (u8*)calloc(block_size * block_cnt, sizeof(u8));
    if (!decode_data_ptr) {
        perror("Calloc decode buffer error");
        exit(EXIT_FAILURE);
    }

    // Belief Propagation 解码
    std::vector<bool> is_decoded(block_cnt, false);
    bool flag = true;
    while (flag) {
        flag = false;
        for (u32 i = 0; i < packet_cnt; i++) {
            u32 seed = *(u32*)(encode_data_ptr + i * (block_size + seed_size) + block_size);
            u32 degree = gen_degree_ideal_soliton(seed, block_cnt);
            std::set<u32> indexes = gen_indexes(seed, degree, block_cnt);

            for (auto index : indexes) {
                if (is_decoded[index])
                    degree--;
            }
            if (degree == 1) {
                u8* block_ptr = (u8*)calloc(block_size, sizeof(u8));
                memcpy(block_ptr, encode_data_ptr + i * (block_size + seed_size), block_size);
                u32 new_decode_index = 0;
                for (auto index : indexes) {
                    if (is_decoded[index]) {
                        for (u32 j = 0; j < block_size; j++)
                            block_ptr[j] ^= decode_data_ptr[index * block_size + j];
                    }
                    else
                        new_decode_index = index;
                }
                memcpy(decode_data_ptr + new_decode_index * block_size, block_ptr, block_size);
                is_decoded[new_decode_index] = true;
                free(block_ptr);
                flag = true;
            }
        }
    }

    Data decode_data;
    decode_data.ptr = decode_data_ptr;
    decode_data.size = block_size * block_cnt;

    return decode_data;
}

Data read_raw_file(FILE* fp, u32 block_size) {
    fseek(fp, 0, SEEK_END);
    // 读入的文件内容字节长度
    u32 raw_data_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    u32 padding = 0;
    if (raw_data_size % block_size != 0)
        padding = block_size - raw_data_size % block_size;
    // 对齐后的文件内容字节长度
    u32 real_data_size = raw_data_size + padding;

    u8* real_data_ptr = (u8*)calloc(real_data_size, sizeof(u8));
    if (!real_data_ptr) {
        perror("Calloc read buffer error");
        fclose(fp);
        exit(EXIT_FAILURE);
    }
    fread(real_data_ptr, 1, raw_data_size, fp);

    Data real_data;
    real_data.ptr = real_data_ptr;
    real_data.size = real_data_size;

    return real_data;
}

Data read_encode_file(FILE* fp) {
    fseek(fp, 0, SEEK_END);
    u32 encode_data_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    u8* encode_data_ptr = (u8*)calloc(encode_data_size, sizeof(u8));
    if (!encode_data_ptr) {
        perror("Calloc encode buffer error");
        fclose(fp);
        exit(EXIT_FAILURE);
    }
    fread(encode_data_ptr, 1, encode_data_size, fp);

    Data encode_data;
    encode_data.ptr = encode_data_ptr;
    encode_data.size = encode_data_size;

    return encode_data;
}
