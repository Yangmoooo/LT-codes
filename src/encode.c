#include <stdio.h>
#include <stdlib.h>

#include "fountain.h"

Data read_raw_file(FILE* fp, uint32_t block_size) {
    fseek(fp, 0, SEEK_END);
    // 读入的文件内容字节长度
    uint32_t raw_data_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    uint32_t padding = 0;
    if (raw_data_size % block_size != 0)
        padding = block_size - raw_data_size % block_size;
    // 对齐后的文件内容字节长度
    uint32_t real_data_size = raw_data_size + padding;

    uint8_t* real_data_ptr = (uint8_t*)calloc(real_data_size, sizeof(uint8_t));
    if (!real_data_ptr) {
        perror("Calloc read buffer error");
        fclose(fp);
        exit(EXIT_FAILURE);
    }
    fread(real_data_ptr, 1, raw_data_size, fp);
    Data real_data = {real_data_ptr, real_data_size};

    return real_data;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        perror("Parameters error");
        return 1;
    }

    // 传入参数：待编码文件路径、block 大小、packet 数量
    char* file_name = argv[1];
    uint32_t block_size = atoi(argv[2]);
    uint32_t packet_cnt = atoi(argv[3]);

    FILE* file_open_ptr = fopen(file_name, "rb");
    if (!file_open_ptr) {
        perror("Open read file error");
        return 1;
    }
    Data real_data = read_raw_file(file_open_ptr, block_size);
    fclose(file_open_ptr);
    uint8_t* real_data_ptr = real_data.ptr;
    uint32_t real_data_size = real_data.size;

    Data write_data = encode(real_data_ptr, real_data_size, 
                             block_size, packet_cnt);
    free(real_data_ptr);
    uint8_t* write_data_ptr = write_data.ptr;
    uint32_t write_data_size = write_data.size;

    FILE* file_write_ptr = fopen("./data/encode.bin", "wb");
    if (!file_write_ptr) {
        perror("Open write file error");
        free(write_data_ptr);
        return 1;
    }
    fwrite(write_data_ptr, 1, write_data_size, file_write_ptr);
    free(write_data_ptr);
    fclose(file_write_ptr);

    return 0;
}
