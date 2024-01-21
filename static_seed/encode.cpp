#include "fountain.h"

int main(int argc, char* argv[]) {
    if (argc < 3) {
        cerr << "Parameters error!" << endl;
        exit(-1);
    }

    //! packet = block + seed
    //! packet 就是 droplet，可以无限生成
    //! block_cnt 则是对齐后文件按照 block_size 划分的块数
    char* file_name = argv[1];
    u32 block_size = atoi(argv[2]);
    u32 packet_cnt = atoi(argv[3]);

    FILE* file_open_ptr = fopen(file_name, "rb");
    if (!file_open_ptr) {
        cerr << "Open read file error!" << endl;
        exit(-1);
    }

    pair<u8*, u32> real_data = read_raw_file(file_open_ptr, block_size);
    u8* real_data_ptr = real_data.first;
    u32 real_data_size = real_data.second;
    fclose(file_open_ptr);

    pair<u8*, u32> write_data = encode(real_data_ptr, real_data_size, block_size, packet_cnt);
    u8* write_data_ptr = write_data.first;
    u32 write_data_size = write_data.second;

    FILE* file_write_ptr = fopen("encode.txt", "wb");
    if (!file_write_ptr) {
        cerr << "Open write file error!" << endl;
        exit(-1);
    }
    fwrite(write_data_ptr, 1, write_data_size, file_write_ptr);
    fclose(file_write_ptr);

    free(real_data_ptr);
    free(write_data_ptr);

    return 0;
}