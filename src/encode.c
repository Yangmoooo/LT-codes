#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "fountain.h"

Data read_src_file(FILE *fp, uint32_t block_size) {
  fseek(fp, 0, SEEK_END);
  // 读入的文件内容字节长度
  uint32_t raw_data_size = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  uint32_t padding = 0;
  if (raw_data_size % block_size != 0) {
    padding = block_size - raw_data_size % block_size;
  }
  // 对齐后的文件内容字节长度
  uint32_t pad_data_size = raw_data_size + padding;

  uint8_t *pad_data_ptr = (uint8_t *)calloc(pad_data_size, sizeof(uint8_t));
  if (!pad_data_ptr) {
    perror("Calloc read buffer error");
    fclose(fp);
    exit(EXIT_FAILURE);
  }
  fread(pad_data_ptr, 1, raw_data_size, fp);
  Data pad_data = {
      .ptr = pad_data_ptr,
      .size = pad_data_size,
  };
  return pad_data;
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    perror("Parameters error");
    return 1;
  }

  // 传入参数：待编码文件路径、block 大小、packet 数量
  char *file_name = argv[1];
  uint32_t block_size = atoi(argv[2]);
  uint32_t packet_cnt = atoi(argv[3]);

  FILE *src_file_ptr = NULL;
  errno_t err = fopen_s(&src_file_ptr, file_name, "rb");
  if (err != 0) {
    perror("Open source file error");
    return 1;
  }
  Data pad_data = read_src_file(src_file_ptr, block_size);
  fclose(src_file_ptr);

  clock_t start = clock();
  Data enc_data = encode(pad_data.ptr, pad_data.size, block_size, packet_cnt);
  clock_t end = clock();
  free(pad_data.ptr);

  FILE *enc_file_ptr = NULL;
  err = fopen_s(&enc_file_ptr, "./data/encode.bin", "wb");
  if (err != 0) {
    perror("Open encode file error");
    free(enc_data.ptr);
    return 1;
  }
  fwrite(enc_data.ptr, 1, enc_data.size, enc_file_ptr);
  free(enc_data.ptr);
  fclose(enc_file_ptr);

  printf("Encode time: %f s\n", (double)(end - start) / CLOCKS_PER_SEC);
  return 0;
}
