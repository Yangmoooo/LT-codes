#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core.h"

Data ReadSourceFile(FILE *fp, uint32_t block_size) {
    fseek(fp, 0, SEEK_END);
    // 读入的文件内容字节长度
    uint32_t raw_data_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    uint32_t padding = 0;
    if (raw_data_size % block_size != 0) {
        padding = block_size - raw_data_size % block_size;
    }
    // 对齐后的文件内容字节长度
    uint32_t pad_data_size = raw_data_size + padding;
    uint8_t *pad_data_ptr = (uint8_t *)calloc(pad_data_size, sizeof(uint8_t));
    if (!pad_data_ptr) {
        perror("Calloc read buffer error");
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    fread(pad_data_ptr, 1, raw_data_size, fp);
    Data pad_data = {
        .ptr = pad_data_ptr,
        .size = pad_data_size,
    };
    return pad_data;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        perror("Parameters error");
        return 1;
    }

    // 传入参数：待编码文件路径、block 大小、packet 数量
    char *file_name = argv[1];
    uint32_t block_size = atoi(argv[2]);
    uint32_t packet_cnt = atoi(argv[3]);

    FILE *source_file_ptr = fopen(file_name, "rb");
    if (!source_file_ptr) {
        perror("Open source file error");
        return 1;
    }
    Data pad_data = ReadSourceFile(source_file_ptr, block_size);
    fclose(source_file_ptr);

    Data encode_data =
        Encode(pad_data.ptr, pad_data.size, block_size, packet_cnt);
    free(pad_data.ptr);

    FILE *encode_file_ptr = fopen(strcat(file_name, ".enc"), "wb");
    if (!encode_file_ptr) {
        perror("Open encode file error");
        free(encode_data.ptr);
        return 1;
    }
    fwrite(encode_data.ptr, 1, encode_data.size, encode_file_ptr);
    free(encode_data.ptr);
    fclose(encode_file_ptr);

    return 0;
}
