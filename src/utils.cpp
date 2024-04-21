#include "utils.hpp"

#include <math.h>
#include <stdint.h>
#include <string.h>

#include <random>
#include <unordered_set>
#include <vector>

uint8_t *AllocMem(uint32_t size) {
  uint8_t *ptr = (uint8_t *)calloc(size, sizeof(uint8_t));
  if (!ptr) {
    perror("Calloc memory error");
    exit(EXIT_FAILURE);
  }
  return ptr;
}

std::vector<double> GenProbs(uint32_t block_cnt, double R, double delta) {
  std::vector<double> probs(block_cnt + 1, 0.0);
  uint32_t critical_point = static_cast<uint32_t>(ceil(block_cnt / R));
  double tau_critical = R * log(R / delta) / block_cnt;
  double Z = 0.0;  // 归一化常数

  for (uint32_t d = 1; d <= block_cnt; ++d) {
    // 理想孤子部分
    if (d == 1) {
      probs[d] = 1.0 / block_cnt;
    } else {
      probs[d] = 1.0 / (d * (d - 1));
    }
    // 鲁棒孤子部分
    double tau = 0.0;
    if (d <= critical_point - 1) {
      tau = (R / block_cnt) / d;
    } else if (d == critical_point) {
      tau = tau_critical;
    }
    probs[d] += tau;
    Z += probs[d];
  }
  for (auto &prob : probs) {
    prob /= Z;
  }
  return probs;
}

uint32_t CalcDegree(uint32_t seed, uint32_t block_cnt,
                    const std::vector<double> &probs) {
  std::mt19937 gen_rand(seed);
  std::uniform_real_distribution<> uniform(0.0, 1.0);
  double rand_num = uniform(gen_rand);
  uint32_t degree = 1;
  double prob = probs[1];
  while (rand_num > prob && degree < block_cnt) {
    ++degree;
    prob += probs[degree];
  }
  return degree;
}

std::unordered_set<uint32_t> GenIndexes(uint32_t seed, uint32_t degree,
                                        uint32_t block_cnt) {
  std::mt19937 gen_rand(seed);
  std::uniform_int_distribution<> uniform(0, block_cnt - 1);
  std::unordered_set<uint32_t> indexes;
  while (indexes.size() < degree) {
    indexes.insert(uniform(gen_rand));
  }
  return indexes;
}

void FillBlock(uint8_t *dst, uint8_t *src, uint32_t block_size,
               const std::unordered_set<uint32_t> &indexes) {
  uint8_t *block_ptr = AllocMem(block_size);
  for (auto index : indexes) {
    for (uint32_t i = 0; i < block_size; ++i) {
      block_ptr[i] ^= src[index * block_size + i];
    }
  }
  memcpy(dst, block_ptr, block_size);
  free(block_ptr);
}

uint8_t CalcCrc(uint8_t *data, uint32_t len) {
  uint8_t crc = 0;
  while (len--) {
    crc = crc8_table[crc ^ *data++];
  }
  return crc;
}

void DecodeBlock(uint8_t *decode_data_ptr, uint8_t *block_ptr,
                 uint32_t block_size, std::vector<bool> &is_decoded,
                 const std::unordered_set<uint32_t> &indexes) {
  uint32_t new_decode_index = 0;
  for (uint32_t index : indexes) {
    if (is_decoded[index]) {
      for (uint32_t i = 0; i < block_size; ++i) {
        block_ptr[i] ^= decode_data_ptr[index * block_size + i];
      }
    } else {
      new_decode_index = index;
    }
  }
  memcpy(decode_data_ptr + new_decode_index * block_size, block_ptr,
         block_size);
  is_decoded[new_decode_index] = true;
}
