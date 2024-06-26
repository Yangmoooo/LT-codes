/* LT codes implementation based on variable-length seeds.
 * Copyright (c) 2024, Zhaoyang Pan <yangmoooo at outlook dot com>
 * All rights reserved.
 */

#ifndef LTCODES_CORE_H_
#define LTCODES_CORE_H_

#include <stdint.h>

typedef struct {
    uint8_t *ptr;
    uint32_t size;
} Data;

#ifdef __cplusplus
extern "C" {
#endif

Data Encode(uint8_t *pad_data_ptr, uint32_t pad_data_size, uint32_t block_size,
            uint32_t packet_cnt);
Data Decode(uint8_t *encode_data_ptr, uint32_t encode_data_size,
            uint32_t block_size, uint32_t raw_data_size);

#ifdef __cplusplus
}
#endif

#endif // LTCODES_CORE_H_
