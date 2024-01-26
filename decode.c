#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

#include "fountain.h"

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Parameters error!\n");
        return 1;
    }

    // 传入参数：待解码文件路径、block 大小、原始文件大小
    char* file_name = argv[1];
    u32 block_size = atoi(argv[2]);
    u32 raw_data_size = atoi(argv[3]);

    FILE* file_encode_ptr = fopen(file_name, "rb");
    if (!file_encode_ptr) {
        printf("Open encode file error!\n");
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

    #ifdef _WIN32
    _mkdir("./output");
    #else
    mkdir("./output", 0777);
    #endif
    FILE* file_decode_ptr = fopen("./output/decode.bin", "wb");
    if (!file_decode_ptr) {
        printf("Open decode file error!\n");
        free(decode_data_ptr);
        return 1;
    }
    fwrite(decode_data_ptr, 1, decode_data_size, file_decode_ptr);
    free(decode_data_ptr);
    fclose(file_decode_ptr);

    return 0;
}
