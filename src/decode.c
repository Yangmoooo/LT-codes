#include "fountain.h"
#include <stdio.h>
#include <stdlib.h>

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
    Data encode_data = {encode_data_ptr, encode_data_size};

    return encode_data;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        perror("Parameters error");
        return 1;
    }

    // 传入参数：待解码文件路径、block 大小、原始文件大小
    char* file_name = argv[1];
    u32 block_size = atoi(argv[2]);
    u32 raw_data_size = atoi(argv[3]);

    FILE* file_encode_ptr = fopen(file_name, "rb");
    if (!file_encode_ptr) {
        perror("Open encode file error");
        return 1;
    }
    Data encode_data = read_encode_file(file_encode_ptr);
    fclose(file_encode_ptr);
    u8* encode_data_ptr = encode_data.ptr;
    u32 encode_data_size = encode_data.size;

    Data decode_data = decode(encode_data_ptr, encode_data_size, block_size, raw_data_size);
    free(encode_data_ptr);
    u8* decode_data_ptr = decode_data.ptr;
    u32 decode_data_size = decode_data.size;

    FILE* file_decode_ptr = fopen("./data/decode.bin", "wb");
    if (!file_decode_ptr) {
        perror("Open decode file error");
        free(decode_data_ptr);
        return 1;
    }
    fwrite(decode_data_ptr, 1, decode_data_size, file_decode_ptr);
    free(decode_data_ptr);
    fclose(file_decode_ptr);

    return 0;
}
