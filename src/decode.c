#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "fountain.h"

Data read_enc_file(FILE *fp) {
  fseek(fp, 0, SEEK_END);
  uint32_t enc_data_size = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  uint8_t *enc_data_ptr = (uint8_t *)calloc(enc_data_size, sizeof(uint8_t));
  if (!enc_data_ptr) {
    perror("Calloc encode buffer error");
    fclose(fp);
    exit(EXIT_FAILURE);
  }
  fread(enc_data_ptr, 1, enc_data_size, fp);
  Data enc_data = {
      .ptr = enc_data_ptr,
      .size = enc_data_size,
  };
  return enc_data;
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

  FILE *enc_file_ptr = NULL;
  errno_t err = fopen_s(&enc_file_ptr, file_name, "rb");
  if (err != 0) {
    perror("Open encode file error");
    return 1;
  }
  Data enc_data = read_enc_file(enc_file_ptr);
  fclose(enc_file_ptr);

  clock_t start = clock();
  Data dec_data =
      decode(enc_data.ptr, enc_data.size, block_size, raw_data_size);
  clock_t end = clock();
  free(enc_data.ptr);

  FILE *dec_file_ptr = NULL;
  err = fopen_s(&dec_file_ptr, "./data/decode.bin", "wb");
  if (err != 0) {
    perror("Open decode file error");
    free(dec_data.ptr);
    return 1;
  }
  fwrite(dec_data.ptr, 1, dec_data.size, dec_file_ptr);
  free(dec_data.ptr);
  fclose(dec_file_ptr);

  printf("Decode time: %f s\n", (double)(end - start) / CLOCKS_PER_SEC);
  return 0;
}
