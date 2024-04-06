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
 * seed: SeedHdrNoHamming(1 byte) + seed_ext(0~3 bytes)
 * CRC-8: 1 byte
 */

std::vector<uint32_t> GenPacketCntsNoHamming(uint32_t packet_cnt) {
  std::vector<uint32_t> packet_cnts(4, 0);
  if (packet_cnt <= 64) {
    // 只使用 Seed 中的 6 位
    packet_cnts[0] = packet_cnt;
  } else if (packet_cnt <= 16384) {
    // 新增 1 字节，6 + 8 = 14 位，以此类推
    packet_cnts[0] = 64;
    packet_cnts[1] = packet_cnt - 64;
  } else if (packet_cnt <= 4194304) {
    packet_cnts[0] = 64;
    packet_cnts[1] = 16384 - 64;
    packet_cnts[2] = packet_cnt - 16384;
  } else {
    packet_cnts[0] = 64;
    packet_cnts[1] = 16384 - 64;
    packet_cnts[2] = 4194304 - 16384;
    packet_cnts[3] = packet_cnt - 4194304;
  }
  return packet_cnts;
}

Data Encode(uint8_t *pad_data_ptr, uint32_t pad_data_size, uint32_t block_size,
            uint32_t packet_cnt) {
  std::vector<uint32_t> packet_cnts = GenPacketCntsNoHamming(packet_cnt);
  uint32_t encode_data_size = 0;
  for (uint32_t i = 0; i < 4; ++i) {
    encode_data_size += (block_size + i + 2) * packet_cnts[i];
  }
  uint8_t *encode_data_ptr = AllocMem(encode_data_size);

  uint32_t block_cnt = pad_data_size / block_size;
  std::vector<double> probs = GenProbs(block_cnt, sqrt(block_cnt), 0.05);
  uint32_t seed = 0;
  uint8_t *encode_data_tmp_ptr = encode_data_ptr;
  for (uint32_t i = 0; i < 4; ++i) {
    for (uint32_t j = 0; j < packet_cnts[i]; ++j) {
      uint32_t degree = CalcDegree(seed, block_cnt, probs);
      std::unordered_set<uint32_t> indexes =
          GenIndexes(seed, degree, block_cnt);
      // 构造 block
      FillBlock(encode_data_tmp_ptr, pad_data_ptr, block_size, indexes);
      encode_data_tmp_ptr += block_size;
      // 构造 seed
      SeedHdrNoHamming seed_header = {
          .type = static_cast<uint8_t>(i),
          .val = static_cast<uint8_t>(seed & 0x3F),
      };
      memcpy(encode_data_tmp_ptr, &seed_header, sizeof(seed_header));
      encode_data_tmp_ptr += sizeof(seed_header);
      for (uint32_t k = 0; k < i; ++k) {
        uint8_t seed_ext = static_cast<uint8_t>(seed >> (6 + k * 8));
        *encode_data_tmp_ptr++ = seed_ext;
      }
      // 构造 CRC-8
      *encode_data_tmp_ptr =
          CalcCrc(encode_data_tmp_ptr - block_size - 1 - i, block_size + 1 + i);
      ++encode_data_tmp_ptr;
      ++seed;
    }
  }
  Data encode_data = {
      .ptr = encode_data_ptr,
      .size = encode_data_size,
  };
  return encode_data;
}

Data Decode(uint8_t *encode_data_ptr, uint32_t encode_data_size,
            uint32_t block_size, uint32_t raw_data_size) {
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
    uint8_t *encode_data_tmp_ptr = encode_data_ptr;
    while (encode_data_tmp_ptr < encode_data_ptr + encode_data_size) {
      SeedHdrNoHamming seed_header =
          *(SeedHdrNoHamming *)(encode_data_tmp_ptr + block_size);
      uint32_t seed_type = seed_header.type;
      uint32_t payload_size = block_size + sizeof(seed_header) + seed_type;
      uint8_t crc = CalcCrc(encode_data_tmp_ptr, payload_size);
      if (crc != *(encode_data_tmp_ptr + payload_size)) {
        encode_data_tmp_ptr += payload_size + 1;
        continue;
      }
      uint32_t seed = seed_header.val;
      for (uint32_t i = 0; i < seed_type; ++i) {
        seed |=
            static_cast<uint32_t>(*(encode_data_tmp_ptr + block_size + 1 + i))
            << (6 + i * 8);
      }
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
        memcpy(block_ptr, encode_data_tmp_ptr, block_size);
        DecodeBlock(decode_data_ptr, block_ptr, block_size, is_decoded,
                    indexes);
        free(block_ptr);
        flag = true;
      }
      encode_data_tmp_ptr += payload_size + sizeof(crc);
    }
  }
  Data decode_data = {
      .ptr = decode_data_ptr,
      .size = block_size * block_cnt,
  };
  return decode_data;
}
