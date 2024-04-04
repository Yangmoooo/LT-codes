#include "utils.h"

#include <math.h>
#include <stdint.h>

#include <random>
#include <unordered_set>
#include <vector>

std::vector<double> gen_probs(uint32_t block_cnt, double R, double delta) {
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
  for (auto& prob : probs) {
    prob /= Z;
  }
  return probs;
}

uint32_t calc_degree(uint32_t seed, uint32_t block_cnt,
                     const std::vector<double>& probs) {
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

std::unordered_set<uint32_t> gen_indexes(uint32_t seed, uint32_t degree,
                                         uint32_t block_cnt) {
  std::mt19937 gen_rand(seed);
  std::uniform_int_distribution<> uniform(0, block_cnt - 1);
  std::unordered_set<uint32_t> indexes;
  while (indexes.size() < degree) {
    indexes.insert(uniform(gen_rand));
  }
  return indexes;
}

uint8_t calc_crc(uint8_t* data, uint32_t len) {
  uint8_t crc = 0;
  while (len--) {
    crc = crc8_table[crc ^ *data++];
  }
  return crc;
}
