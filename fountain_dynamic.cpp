#include "fountain.h"

// 种子大小可能为 1 字节、2 字节、4 字节
#define MAX_U8 255
#define MAX_U16 65535

template <typename T>
u32 gen_degree_ideal_soliton(T seed, u32 block_cnt) {
    std::mt19937 gen_rand(static_cast<u32>(seed));
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

template <typename T>
std::set<u32> gen_indexes(T seed, u32 degree, u32 block_cnt) {
    std::mt19937 gen_rand(static_cast<u32>(seed));
    std::uniform_int_distribution<> uniform(0, block_cnt - 1);
    std::set<u32> indexes;
    while (indexes.size() < degree)
        indexes.insert(uniform(gen_rand));
    return indexes;
}

//TODO: 修改该函数，用于简化 encode 函数
template <typename T>
void process_packet(u8* write_data_tmp_ptr, u8* real_data_ptr, u32 block_size, 
                    u32 packet_size, T seed, u32 real_data_size, 
                    u32 packet_index) {
    u32 degree = gen_degree_ideal_soliton(seed, real_data_size / block_size);
    std::set<u32> indexes = gen_indexes(seed, degree, real_data_size / block_size);

    u8* block_ptr = (u8*)calloc(block_size, sizeof(u8));
    for (auto index : indexes) {
        for (u32 j = 0; j < block_size; j++)
            block_ptr[j] ^= real_data_ptr[index * block_size + j];
    }

    u8 seed_size = sizeof(T);
    memcpy(write_data_tmp_ptr + packet_index * packet_size, block_ptr, block_size);
    memcpy(write_data_tmp_ptr + packet_index * packet_size + block_size, &seed_size, sizeof(u8));
    memcpy(write_data_tmp_ptr + packet_index * packet_size + block_size + sizeof(u8), &seed, seed_size);
    free(block_ptr);
}

Data encode(u8* real_data_ptr, u32 real_data_size, u32 block_size, u32 packet_cnt) {
    u32 packet_u8_cnt = 0, packet_u16_cnt = 0, packet_u32_cnt = 0;
    if (packet_cnt <= MAX_U8 + 1) {
        packet_u8_cnt = packet_cnt;
    } else if (packet_cnt <= MAX_U16 + 1) {
        packet_u8_cnt = MAX_U8 + 1;
        packet_u16_cnt = packet_cnt - packet_u8_cnt;
    } else {
        packet_u8_cnt = MAX_U8 + 1;
        packet_u16_cnt = MAX_U16 - MAX_U8;
        packet_u32_cnt = packet_cnt - packet_u8_cnt - packet_u16_cnt;
    }

    u32 write_data_size = (block_size + 1 + 1) * packet_u8_cnt + 
                          (block_size + 1 + 2) * packet_u16_cnt + 
                          (block_size + 1 + 4) * packet_u32_cnt;
    u8* write_data_ptr = (u8*)calloc(write_data_size, sizeof(u8));
    if (!write_data_ptr) {
        printf("Calloc write buffer error!\n");
        exit(-1);
    }

    u8* write_data_tmp_ptr = write_data_ptr;
    u8 seed_size = 1;
    u32 packet_size = block_size + sizeof(u8) + seed_size;
    for (u32 i = 0; i < packet_u8_cnt; i++) {
        u8 seed = i;
        u32 degree = gen_degree_ideal_soliton(seed, real_data_size / block_size);
        std::set<u32> indexes = gen_indexes(seed, degree, real_data_size / block_size);

        u8* block_ptr = (u8*)calloc(block_size, sizeof(u8));
        for (auto index : indexes) {
            for (u32 j = 0; j < block_size; j++)
                block_ptr[j] ^= real_data_ptr[index * block_size + j];
        }

        memcpy(write_data_tmp_ptr + i * packet_size, block_ptr, block_size);
        memcpy(write_data_tmp_ptr + i * packet_size + block_size, &seed_size, sizeof(u8));
        memcpy(write_data_tmp_ptr + i * packet_size + block_size + sizeof(u8), &seed, seed_size);
        free(block_ptr);
    }

    write_data_tmp_ptr += packet_u8_cnt * (block_size + sizeof(u8) + 1);
    seed_size = 2;
    packet_size = block_size + sizeof(u8) + seed_size;
    for (u32 i = 0; i < packet_u16_cnt; i++) {
        u16 seed = i + packet_u8_cnt;
        u32 degree = gen_degree_ideal_soliton(seed, real_data_size / block_size);
        std::set<u32> indexes = gen_indexes(seed, degree, real_data_size / block_size);

        u8* block_ptr = (u8*)calloc(block_size, sizeof(u8));
        for (auto index : indexes) {
            for (u32 j = 0; j < block_size; j++)
                block_ptr[j] ^= real_data_ptr[index * block_size + j];
        }

        memcpy(write_data_tmp_ptr + i * packet_size, block_ptr, block_size);
        memcpy(write_data_tmp_ptr + i * packet_size + block_size, &seed_size, sizeof(u8));
        memcpy(write_data_tmp_ptr + i * packet_size + block_size + sizeof(u8), &seed, seed_size);
        free(block_ptr);
    }

    write_data_tmp_ptr += packet_u16_cnt * (block_size + sizeof(u8) + 2);
    seed_size = 4;
    packet_size = block_size + sizeof(u8) + seed_size;
    for (u32 i = 0; i < packet_u32_cnt; i++) {
        u32 seed = i + packet_u8_cnt + packet_u16_cnt;
        u32 degree = gen_degree_ideal_soliton(seed, real_data_size / block_size);
        std::set<u32> indexes = gen_indexes(seed, degree, real_data_size / block_size);

        u8* block_ptr = (u8*)calloc(block_size, sizeof(u8));
        for (auto index : indexes) {
            for (u32 j = 0; j < block_size; j++)
                block_ptr[j] ^= real_data_ptr[index * block_size + j];
        }

        memcpy(write_data_tmp_ptr + i * packet_size, block_ptr, block_size);
        memcpy(write_data_tmp_ptr + i * packet_size + block_size, &seed_size, sizeof(u8));
        memcpy(write_data_tmp_ptr + i * packet_size + block_size + sizeof(u8), &seed, seed_size);
        free(block_ptr);
    }

    Data write_data;
    write_data.ptr = write_data_ptr;
    write_data.size = write_data_size;

    return write_data;
}

Data decode(u8* encode_data_ptr, u32 encode_data_size, u32 block_size, u32 raw_data_size) {
    u32 block_cnt = raw_data_size / block_size;
    if (raw_data_size % block_size != 0)
        block_cnt++;
    u8* decode_data_ptr = (u8*)calloc(block_size * block_cnt, sizeof(u8));
    if (!decode_data_ptr) {
        printf("Calloc decode buffer error!\n");
        exit(-1);
    }

    std::vector<bool> is_decoded(block_cnt, false);
    u32 packet_u8_size = block_size + sizeof(u8) + 1;
    u32 packet_u16_size = block_size + sizeof(u8) + 2;
    u32 packet_u32_size = block_size + sizeof(u8) + 4;
    //TODO: 将原本的内层 for 循环改为用指针的 while 循环，手动控制指针移动，进行边界检查
    bool flag = true;
    while (flag) {
        flag = false;
        u8* encode_data_tmp_ptr = encode_data_ptr;
        while (encode_data_tmp_ptr < encode_data_ptr + encode_data_size) {
            u8 seed_size = *(encode_data_tmp_ptr + block_size);
            u32 packet_size = block_size + sizeof(u8) + seed_size;
            u32 seed = 0;
            if (seed_size == 1) {
                seed = *(encode_data_tmp_ptr + block_size + sizeof(u8));
                encode_data_tmp_ptr += packet_u8_size;
            } else if (seed_size == 2) {
                seed = *(u16*)(encode_data_tmp_ptr + block_size + sizeof(u8));
                encode_data_tmp_ptr += packet_u16_size;
            } else {
                seed = *(u32*)(encode_data_tmp_ptr + block_size + sizeof(u8));
                encode_data_tmp_ptr += packet_u32_size;
            }

            u32 degree = gen_degree_ideal_soliton(seed, block_cnt);
            std::set<u32> indexes = gen_indexes(seed, degree, block_cnt);
            for (auto index : indexes) {
                if (is_decoded[index])
                    degree--;
            }
            if (degree == 1) {
                u8* block_ptr = (u8*)calloc(block_size, sizeof(u8));
                memcpy(block_ptr, encode_data_tmp_ptr - packet_size, block_size);
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
                free(block_ptr);
                is_decoded[new_decode_index] = true;
                flag = true;
            }
        }
    }

    Data decode_data;
    decode_data.ptr = decode_data_ptr;
    decode_data.size = block_size * block_cnt;

    return decode_data;
}

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

    u8* real_data_ptr = (u8*)calloc(real_data_size, sizeof(u8));
    if (!real_data_ptr) {
        printf("Calloc read buffer error!\n");
        fclose(fp);
        exit(-1);
    }
    fread(real_data_ptr, 1, raw_data_size, fp);

    Data real_data;
    real_data.ptr = real_data_ptr;
    real_data.size = real_data_size;

    return real_data;
}

// 读取编码文件
Data read_encode_file(FILE* fp) {
    fseek(fp, 0, SEEK_END);
    u32 encode_data_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    u8* encode_data_ptr = (u8*)calloc(encode_data_size, sizeof(u8));
    if (!encode_data_ptr) {
        printf("Calloc encode buffer error!\n");
        fclose(fp);
        exit(-1);
    }
    fread(encode_data_ptr, 1, encode_data_size, fp);

    Data encode_data;
    encode_data.ptr = encode_data_ptr;
    encode_data.size = encode_data_size;

    return encode_data;
}