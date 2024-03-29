/* Fountain Code implementation based on variable-length seeds.
 * Copyright (c) 2024, Zhaoyang Pan <yangmoooo at outlook dot com>
 * All rights reserved.
 */
#pragma once

#include <stdint.h>

typedef struct {
    uint8_t* ptr;
    uint32_t size;
} Data;

typedef struct {
    uint8_t p1 : 1;
    uint8_t p2 : 1;
    uint8_t d1 : 1;
    uint8_t p3 : 1;
    uint8_t d2 : 1;
    uint8_t val : 3;
} SeedHeader;

#ifdef __cplusplus
extern "C" {
#endif

Data encode(uint8_t* real_data_ptr, uint32_t real_data_size, uint32_t block_size, 
            uint32_t packet_cnt);
Data decode(uint8_t* write_data_ptr, uint32_t write_data_size, uint32_t block_size, 
            uint32_t raw_data_size);

#ifdef __cplusplus
}
#endif
