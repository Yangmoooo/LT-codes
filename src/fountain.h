/* Fountain Code implementation based on variable-length seeds.
 * Copyright (c) 2024, Zhaoyang Pan <yangmoooo at outlook dot com>
 * All rights reserved.
 */
#pragma once

#include <stdio.h>

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

typedef struct {
    u8 type : 2;
    u8 val : 6;
} Seed;

#ifdef __cplusplus
extern "C" {
#endif

Data encode(u8* real_data_ptr, u32 real_data_size, u32 block_size, u32 packet_cnt);
Data decode(u8* write_data_ptr, u32 write_data_size, u32 block_size, u32 raw_data_size);

#ifdef __cplusplus
}
#endif
