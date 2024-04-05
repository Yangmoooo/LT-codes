#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "core.h"

Data ReadEncodeFile(FILE *fp) {
  fseek(fp, 0, SEEK_END);
  uint32_t encode_data_size = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  uint8_t *encode_data_ptr = (uint8_t *)calloc(encode_data_size, sizeof(uint8_t));
  if (!encode_data_ptr) {
    perror("Calloc encode buffer error");
    fclose(fp);
    exit(EXIT_FAILURE);
  }
  fread(encode_data_ptr, 1, encode_data_size, fp);
  Data encode_data = {
      .ptr = encode_data_ptr,
      .size = encode_data_size,
  };
  return encode_data;
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    perror("Parameters error");
    return 1;
  }

  // 传入参数：待解码文件路径、block 大小、原始文件大小
  char *file_name = argv[1];
  uint32_t block_size = atoi(argv[2]);
  uint32_t raw_data_size = atoi(argv[3]);

  FILE *encode_file_ptr = fopen(file_name, "rb");
  if (!encode_file_ptr) {
    perror("Open encode file error");
    return 1;
  }
  Data encode_data = ReadEncodeFile(encode_file_ptr);
  fclose(encode_file_ptr);

  clock_t start = clock();
  Data decode_data =
      Decode(encode_data.ptr, encode_data.size, block_size, raw_data_size);
  clock_t end = clock();
  free(encode_data.ptr);

  FILE *decode_file_ptr = fopen("./data/decode.bin", "wb");
  if (!decode_file_ptr) {
    perror("Open decode file error");
    free(decode_data.ptr);
    return 1;
  }
  fwrite(decode_data.ptr, 1, decode_data.size, decode_file_ptr);
  free(decode_data.ptr);
  fclose(decode_file_ptr);

  printf("Decode time: %f s\n", (double)(end - start) / CLOCKS_PER_SEC);
  return 0;
}
