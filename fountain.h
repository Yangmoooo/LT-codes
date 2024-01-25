#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif
// #include <time.h>
#include <set>
#include <random>

typedef unsigned char u8;
typedef signed char i8;
typedef unsigned short u16;
typedef signed short i16;
typedef unsigned int u32;
typedef signed int i32;
typedef unsigned long long u64;
typedef signed long long i64;

typedef struct {
    u8* ptr;
    u32 size;
} Data;

Data encode(u8* real_data_ptr, u32 real_data_size, u32 block_size, u32 packet_cnt);
Data decode(u8* write_data_ptr, u32 write_data_size, u32 block_size, u32 raw_data_size);
Data read_raw_file(FILE* fp, u32 block_size);
Data read_encode_file(FILE* fp);
