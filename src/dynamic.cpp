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
 * seed: SeedHdr(1 byte) + seed_ext(0~3 bytes)
 * CRC-8: 1 byte
 */

typedef struct {
    uint8_t p1 : 1;
    uint8_t p2 : 1;
    uint8_t d1 : 1;
    uint8_t p3 : 1;
    uint8_t d2 : 1;
    uint8_t val : 3;
} SeedHdr;

std::vector<uint32_t> GenPacketCnts(uint32_t packet_cnt) {
    std::vector<uint32_t> packet_cnts(4, 0);
    if (packet_cnt <= 8) {
        // 只使用 seed 中的 3 位
        packet_cnts[0] = packet_cnt;
    } else if (packet_cnt <= 2048) {
        // 添加 1 字节，3 + 8 = 11 位，以此类推
        packet_cnts[0] = 8;
        packet_cnts[1] = packet_cnt - 8;
    } else if (packet_cnt <= 524288) {
        packet_cnts[0] = 8;
        packet_cnts[1] = 2048 - 8;
        packet_cnts[2] = packet_cnt - 2048;
    } else {
        packet_cnts[0] = 8;
        packet_cnts[1] = 2048 - 8;
        packet_cnts[2] = 524288 - 2048;
        packet_cnts[3] = packet_cnt - 524288;
    }
    return packet_cnts;
}

uint32_t CalcSeedType(SeedHdr *header) {
    uint8_t s1 = header->p1 ^ header->d1 ^ header->d2;
    uint8_t s2 = header->p2 ^ header->d1;
    uint8_t s3 = header->p3 ^ header->d2;
    uint8_t error_pos = s1 | s2 << 1 | s3 << 2;
    if (error_pos) {
        switch (error_pos) {
        case 1:
            header->p1 = !header->p1;
            break;
        case 2:
            header->p2 = !header->p2;
            break;
        case 3:
            header->d1 = !header->d1;
            break;
        case 4:
            header->p3 = !header->p3;
            break;
        case 5:
            header->d2 = !header->d2;
            break;
        default:
            break;
        }
    }
    return header->d1 << 1 | header->d2;
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
            SeedHdr seed_header = {
                .d1 = static_cast<uint8_t>(i >> 1),
                .d2 = static_cast<uint8_t>(i),
                .val = static_cast<uint8_t>(seed & 0x07),
            };
            seed_header.p1 = seed_header.d1 ^ seed_header.d2;
            seed_header.p2 = seed_header.d1;
            seed_header.p3 = seed_header.d2;
            memcpy(encode_data_tmp_ptr, &seed_header, sizeof(seed_header));
            encode_data_tmp_ptr += sizeof(seed_header);
            for (uint32_t k = 0; k < i; ++k) {
                uint8_t seed_ext = static_cast<uint8_t>(seed >> (3 + k * 8));
                *encode_data_tmp_ptr++ = seed_ext;
            }
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
            SeedHdr *seed_header =
                (SeedHdr *)(encode_data_tmp_ptr + block_size);
            uint32_t seed_type = CalcSeedType(seed_header);
            uint32_t payload_size = block_size + sizeof(SeedHdr) + seed_type;
            uint8_t crc = CalcCrc(encode_data_tmp_ptr, payload_size);
            if (crc != *(encode_data_tmp_ptr + payload_size)) {
                encode_data_tmp_ptr += payload_size + 1;
                continue;
            }
            uint32_t seed = seed_header->val;
            for (uint32_t i = 0; i < seed_type; ++i) {
                seed |= static_cast<uint32_t>(
                            *(encode_data_tmp_ptr + block_size + 1 + i))
                        << (3 + i * 8);
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
