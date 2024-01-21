#include "fountain.h"

int main(int argc, char* argv[]) {
    if (argc < 3) {
        cerr << "Parameters error!" << endl;
        exit(-1);
    }

    // 传入参数：待解码文件名、block 大小、原始文件大小
    char* file_name = argv[1];
    u32 block_size = atoi(argv[2]);
    u32 raw_data_size = atoi(argv[3]);

    FILE* file_encode_ptr = fopen(file_name, "rb");
    if (!file_encode_ptr) {
        cerr << "Open encode file error!" << endl;
        exit(-1);
    }

    pair<u8*, u32> encode_data = read_encode_file(file_encode_ptr);
    u8* encode_data_ptr = encode_data.first;
    u32 encode_data_size = encode_data.second;
    fclose(file_encode_ptr);

    pair<u8*, u32> decode_data = decode(encode_data_ptr, encode_data_size, block_size, raw_data_size);
    u8* decode_data_ptr = decode_data.first;
    u32 decode_data_size = decode_data.second;

    FILE* file_decode_ptr = fopen("decode.txt", "wb");
    if (!file_decode_ptr) {
        cerr << "Open decode file error!" << endl;
        exit(-1);
    }
    fwrite(decode_data_ptr, 1, decode_data_size, file_decode_ptr);
    fclose(file_decode_ptr);

    free(encode_data_ptr);
    free(decode_data_ptr);

    return 0;
}