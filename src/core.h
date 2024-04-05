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

typedef struct {
  uint8_t p1 : 1;
  uint8_t p2 : 1;
  uint8_t d1 : 1;
  uint8_t p3 : 1;
  uint8_t d2 : 1;
  uint8_t val : 3;
} SeedHdr;

typedef struct {
  uint8_t type : 2;
  uint8_t val : 6;
} SeedHdrNoHamming;

#ifdef __cplusplus
extern "C" {
#endif

Data Encode(uint8_t *pad_data_ptr, uint32_t pad_data_size,
            uint32_t block_size, uint32_t packet_cnt);
Data Decode(uint8_t *encode_data_ptr, uint32_t encode_data_size,
            uint32_t block_size, uint32_t raw_data_size);

#ifdef __cplusplus
}
#endif

#endif  // LTCODES_CORE_H_
