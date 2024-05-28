#include "core.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <unordered_set>
#include <vector>

#include "utils.hpp"

/* packet = block + seed + CRC-8
 * block: block_size bytes
 * seed: LEB128 integer(1~4 bytes)
 * CRC-8: 1 byte
 */

std::vector<uint32_t> GenPacketCnts(uint32_t packet_cnt) {
    std::vector<uint32_t> packet_cnts(4, 0);
    if (packet_cnt <= 128) {
        packet_cnts[0] = packet_cnt;
    } else if (packet_cnt <= 16384) {
        packet_cnts[0] = 128;
        packet_cnts[1] = packet_cnt - 128;
    } else if (packet_cnt <= 2097152) {
        packet_cnts[0] = 128;
        packet_cnts[1] = 16384 - 128;
        packet_cnts[2] = packet_cnt - 16384;
    } else {
        packet_cnts[0] = 128;
        packet_cnts[1] = 16384 - 128;
        packet_cnts[2] = 2097152 - 16384;
        packet_cnts[3] = packet_cnt - 2097152;
    }
    return packet_cnts;
}

uint32_t EncodeLeb128(uint8_t *ptr, uint32_t val) {
    uint32_t size = 0;
    do {
        uint8_t byte = val & 0x7F;
        val >>= 7;
        if (val != 0) {
            byte |= 0x80;
        }
        ptr[size++] = byte;
    } while (val != 0);
    return size;
}

uint32_t DecodeLeb128(uint8_t *ptr, uint32_t &val) {
    uint32_t size = 0, shift = 0;
    do {
        val |= (ptr[size] & 0x7F) << shift;
        shift += 7;
    } while (ptr[size++] & 0x80);
    return size;
}

Data Encode(uint8_t *pad_data_ptr, uint32_t pad_data_size, uint32_t block_size,
            uint32_t packet_cnt) {
    std::vector<uint32_t> packet_cnts = GenPacketCnts(packet_cnt);
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
            encode_data_tmp_ptr += EncodeLeb128(encode_data_tmp_ptr, seed);
            // 构造 CRC-8
            *encode_data_tmp_ptr = CalcCrc(
                encode_data_tmp_ptr - block_size - 1 - i, block_size + 1 + i);
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
        // 若靠后的某个包长度字段出现 1 位以上错误，则最后一个包可能越界访问
        uint8_t *encode_data_tmp_ptr = encode_data_ptr;
        while (encode_data_tmp_ptr < encode_data_ptr + encode_data_size) {
            uint32_t seed = 0;
            uint32_t seed_size =
                DecodeLeb128(encode_data_tmp_ptr + block_size, seed);
            uint32_t payload_size = block_size + seed_size;
            uint8_t crc = CalcCrc(encode_data_tmp_ptr, payload_size);
            if (crc != *(encode_data_tmp_ptr + payload_size)) {
                encode_data_tmp_ptr += payload_size + 1;
                continue;
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
