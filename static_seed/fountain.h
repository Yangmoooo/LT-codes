#include <iostream>
#include <string.h>
#include <time.h>
#include <set>
#include <random>

using namespace std;

typedef unsigned char u8;
typedef signed char i8;
typedef unsigned short u16;
typedef signed short i16;
typedef unsigned int u32;
typedef signed int i32;
typedef unsigned long long u64;
typedef signed long long i64;

pair<u8*, u32> read_raw_file(FILE* fp, u32 blockSize);
pair<u8*, u32> encode(u8* real_data_ptr, u32 real_data_size, u32 block_size, u32 packet_cnt);
pair<u8*, u32> read_encode_file(FILE* fp);
pair<u8*, u32> decode(u8* write_data_ptr, u32 write_data_size, u32 block_size, u32 raw_data_size);