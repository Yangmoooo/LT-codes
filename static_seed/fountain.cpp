#include "fountain.h"

// 读取原始文件
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

    u8* real_data_ptr = (u8*)malloc(sizeof(u8) * (real_data_size));
    if (!real_data_ptr) {
        printf("Malloc read buffer error!\n");
        fclose(fp);
        exit(-1);
    }
    memset(real_data_ptr, 0, raw_data_size);
    fread(real_data_ptr, 1, raw_data_size, fp);

    Data real_data;
    real_data.ptr = real_data_ptr;
    real_data.size = real_data_size;

    return real_data;
}

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
    u8* write_data_ptr = (u8*)malloc(sizeof(u8) * ((block_size + 4) * packet_cnt));
    if (!write_data_ptr) {
        printf("Malloc write buffer error!\n");
        exit(-1);
    }
    memset(write_data_ptr, 0, (block_size + 4) * packet_cnt);

    for (u32 i = 0; i < packet_cnt; i++) {
        // mt19937 算法生成的随机数质量足够好，不太需要将 seed 分散
        u32 seed = i;
        u32 degree = gen_degree_ideal_soliton(seed, real_data_size / block_size);
        std::set<u32> indexes = gen_indexes(seed, degree, real_data_size / block_size);

        u8* block_ptr = (u8*)malloc(sizeof(u8) * block_size);
        memset(block_ptr, 0, block_size);
        for (auto index : indexes) {
            for (u32 j = 0; j < block_size; j++)
                block_ptr[j] ^= real_data_ptr[index * block_size + j];
        }

        memcpy(write_data_ptr + i * (block_size + 4), block_ptr, block_size);
        memcpy(write_data_ptr + i * (block_size + 4) + block_size, &seed, 4);
        free(block_ptr);
    }

    Data write_data;
    write_data.ptr = write_data_ptr;
    write_data.size = (block_size + 4) * packet_cnt;

    return write_data;
}

// 读取编码文件
Data read_encode_file(FILE* fp) {
    fseek(fp, 0, SEEK_END);
    u32 encode_data_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    u8* encode_data_ptr = (u8*)malloc(sizeof(u8) * encode_data_size);
    if (!encode_data_ptr) {
        printf("Malloc encode buffer error!\n");
        fclose(fp);
        exit(-1);
    }
    memset(encode_data_ptr, 0, encode_data_size);
    fread(encode_data_ptr, 1, encode_data_size, fp);

    Data encode_data;
    encode_data.ptr = encode_data_ptr;
    encode_data.size = encode_data_size;

    return encode_data;
}

Data decode(u8* encode_data_ptr, u32 encode_data_size, u32 block_size, u32 raw_data_size) {
    u32 packet_cnt = encode_data_size / (block_size + 4);
    // block_cnt 是原始文件对齐后按照 block_size 划分的块数
    u32 block_cnt = raw_data_size / block_size;
    if (raw_data_size % block_size != 0)
        block_cnt++;
    u8* decode_data_ptr = (u8*)malloc(sizeof(u8) * (block_size * block_cnt));
    if (!decode_data_ptr) {
        printf("Malloc decode buffer error!\n");
        exit(-1);
    }
    memset(decode_data_ptr, 0, block_size * block_cnt);

    std::vector<bool> is_decoded(block_cnt, false);

    // 先得到度为 1 的原始数据块
    for (u32 i = 0; i < packet_cnt; i++) {
        u32 seed = *(u32*)(encode_data_ptr + i * (block_size + 4) + block_size);
        u32 degree = gen_degree_ideal_soliton(seed, block_cnt);
        if (degree == 1) {
            std::set<u32> indexes = gen_indexes(seed, degree, block_cnt);
            for (auto index : indexes) {
                memcpy(decode_data_ptr + index * block_size, encode_data_ptr + i * (block_size + 4), block_size);
                is_decoded[index] = true;
            }
        }
    }

    // Belief Propagation 解码
    bool flag = false;
    while (!flag) {
        flag = true;
        for (u32 i = 0; i < packet_cnt; i++) {
            u32 seed = *(u32*)(encode_data_ptr + i * (block_size + 4) + block_size);
            u32 degree = gen_degree_ideal_soliton(seed, block_cnt);
            if (degree == 1)
                continue;

            std::set<u32> indexes = gen_indexes(seed, degree, block_cnt);
            for (auto index : indexes) {
                if (is_decoded[index])
                    degree--;
            }
            if (degree == 1) {
                u8* block_ptr = (u8*)malloc(sizeof(u8) * block_size);
                memcpy(block_ptr, encode_data_ptr + i * (block_size + 4), block_size);
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
                flag = false;
            }
        }
    }

    Data decode_data;
    decode_data.ptr = decode_data_ptr;
    decode_data.size = block_size * block_cnt;

    return decode_data;
}