#include <stdio.h>
#include <stdlib.h>
#include "fountain.h"

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Parameters error!\n");
        return 1;
    }

    // 传入参数：待编码文件路径、block 大小、packet 数量
    char* file_name = argv[1];
    u32 block_size = atoi(argv[2]);
    u32 packet_cnt = atoi(argv[3]);

    FILE* file_open_ptr = fopen(file_name, "rb");
    if (!file_open_ptr) {
        printf("Open read file error!\n");
        return 1;
    }
    Data real_data = read_raw_file(file_open_ptr, block_size);
    fclose(file_open_ptr);
    u8* real_data_ptr = real_data.ptr;
    u32 real_data_size = real_data.size;

    Data write_data = encode(real_data_ptr, real_data_size, block_size, packet_cnt);
    free(real_data_ptr);
    u8* write_data_ptr = write_data.ptr;
    u32 write_data_size = write_data.size;

    FILE* file_write_ptr = fopen("./data/encode.bin", "wb");
    if (!file_write_ptr) {
        printf("Open write file error!\n");
        free(write_data_ptr);
        return 1;
    }
    fwrite(write_data_ptr, 1, write_data_size, file_write_ptr);
    free(write_data_ptr);
    fclose(file_write_ptr);

    return 0;
}
