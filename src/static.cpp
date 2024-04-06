#include "core.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <unordered_set>
#include <vector>

#include "utils.h"

/* packet = block + seed + CRC-8
 * block: block_size bytes
 * seed: kSeedSize bytes
 * CRC-8: 1 byte
 */

static const uint32_t kSeedSize = 4;

Data Encode(uint8_t *pad_data_ptr, uint32_t pad_data_size, uint32_t block_size,
            uint32_t packet_cnt) {
  uint32_t packet_size = block_size + kSeedSize + 1;
  uint8_t *encode_data_ptr = AllocMem(packet_size * packet_cnt);
  uint32_t block_cnt = pad_data_size / block_size;
  std::vector<double> probs = GenProbs(block_cnt, sqrt(block_cnt), 0.05);
  for (uint32_t i = 0; i < packet_cnt; ++i) {
    // mt19937 算法生成的随机数质量足够好，不太需要将 seed 分散
    uint32_t seed = i;
    uint32_t degree = CalcDegree(seed, block_cnt, probs);
    std::unordered_set<uint32_t> indexes = GenIndexes(seed, degree, block_cnt);
    uint8_t *encode_data_tmp_ptr = encode_data_ptr + i * packet_size;
    // 构造 block
    FillBlock(encode_data_tmp_ptr, pad_data_ptr, block_size, indexes);
    // 构造 seed
    memcpy(encode_data_tmp_ptr + block_size, &seed, kSeedSize);
    // 构造 CRC-8
    encode_data_tmp_ptr[block_size + kSeedSize] =
        CalcCrc(encode_data_tmp_ptr, block_size + kSeedSize);
  }
  Data encode_data = {
      .ptr = encode_data_ptr,
      .size = packet_size * packet_cnt,
  };
  return encode_data;
}

Data Decode(uint8_t *encode_data_ptr, uint32_t encode_data_size,
            uint32_t block_size, uint32_t raw_data_size) {
  uint32_t packet_size = block_size + kSeedSize + 1;
  uint32_t packet_cnt = encode_data_size / packet_size;
  // block_cnt 是原始文件对齐后按照 block_size 划分的块数
  uint32_t block_cnt = raw_data_size / block_size;
  if (raw_data_size % block_size != 0) {
    ++block_cnt;
  }
  uint8_t *decode_data_ptr = AllocMem(block_size * block_cnt);
  std::vector<double> probs = GenProbs(block_cnt, sqrt(block_cnt), 0.05);
  std::vector<bool> is_decoded(block_cnt, false);
  // Belief Propagation 解码
  bool flag = true;
  while (flag) {
    flag = false;
    for (uint32_t i = 0; i < packet_cnt; ++i) {
      uint8_t crc =
          CalcCrc(encode_data_ptr + i * packet_size, block_size + kSeedSize);
      if (crc != encode_data_ptr[i * packet_size + block_size + kSeedSize]) {
        continue;
      }
      uint32_t seed =
          *(uint32_t *)(encode_data_ptr + i * packet_size + block_size);
      uint32_t degree = CalcDegree(seed, block_cnt, probs);
      std::unordered_set<uint32_t> indexes =
          GenIndexes(seed, degree, block_cnt);
      // 根据度计算出未解码的块数
      for (uint32_t index : indexes) {
        if (is_decoded[index]) {
          --degree;
        }
      }
      if (degree == 1) {
        uint8_t *block_ptr = AllocMem(block_size);
        memcpy(block_ptr, encode_data_ptr + i * packet_size, block_size);
        DecodeBlock(decode_data_ptr, block_ptr, block_size, is_decoded,
                    indexes);
        free(block_ptr);
        flag = true;
      }
    }
  }
  Data decode_data = {
      .ptr = decode_data_ptr,
      .size = block_size * block_cnt,
  };
  return decode_data;
}
