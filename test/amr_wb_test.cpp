/**
 * @file amr_wb.cpp
 * @author SWH
 * @brief Test coder->amr_wb model using file
 * @version 0.1
 * @date 2019-08-12
 *
 * @copyright Copyright (c) 2019
 *
 */
#include <array>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>

#include "codec.h"
#include "logger.h"

const char kInput[] = "8k.raw";
const char kOutputAmr[] = "8k.amr";
const char kOutputPcm[] = "amr_output.pcm";

const int kCodecUnit = 3;

void NormalTest() {
  std::unique_ptr<Coder> encoder{new AmrWbEnCoder};
  std::unique_ptr<Coder> decoder{new AmrWbDeCoder};
  if (encoder->Init()) {
    fprintf(stderr, "[Error] Encoder init error\n");
    return;
  }
  if (decoder->Init()) {
    fprintf(stderr, "[Error] Decoder init error\n");
    return;
  }
  // Read file
  std::ifstream in_fd;
  in_fd.open(kInput, std::ifstream::binary);
  if (!in_fd) {
    fprintf(stderr, "[Error] Cannot open input pcm file\n");
    return;
  }
  // Get length of input file
  in_fd.seekg(0, in_fd.end);
  int input_length = in_fd.tellg();
  in_fd.seekg(in_fd.beg);
  char* in_buff = new char[input_length];
  int buff_size = input_length;
  in_fd.read(in_buff, input_length);
  if (!in_fd) {
    fprintf(stderr, "[Error] Cannot read data\n");
    return;
  }
  in_fd.close();
  char* p_frame = in_buff;
  char amr_buff[1600];
  int codec_uint_length = kPcmFrameLength * kCodecUnit;
  while (input_length >= codec_uint_length) {
    int len = encoder->Codec((uint8_t*)p_frame, codec_uint_length,
                             (uint8_t*)amr_buff, sizeof(amr_buff));
    if (len < 0) {
      fprintf(stderr, "[Error] Encoder error\n");
      delete[] in_buff;
      return;
    }
    len = decoder->Codec((uint8_t*)amr_buff, len, (uint8_t*)p_frame,
                         input_length);
    if (len != codec_uint_length) {
      fprintf(stderr, "[Error] Decoder output length error\n");
      delete[] in_buff;
      return;
    }
    if (len < 0) {
      fprintf(stderr, "[Error] Decoder error\n");
      delete[] in_buff;
      return;
    }
    p_frame += codec_uint_length;
    input_length -= codec_uint_length;
  }
  std::ofstream out_fd;
  out_fd.open(kOutputPcm, std::ofstream::binary);
  out_fd.write(in_buff, buff_size);
  delete[] in_buff;
  if (!out_fd) {
    fprintf(stderr, "[Error] Write error\n");
    out_fd.close();
    return;
  }
  out_fd.close();
}

void EncodeTest() {
  std::unique_ptr<Coder> encoder{new AmrWbEnCoder};
  if (encoder->Init()) {
    fprintf(stderr, "[Error] Encoder init error\n");
    return;
  }
  // Read file
  std::ifstream in_fd;
  in_fd.open(kInput, std::ifstream::binary);
  if (!in_fd) {
    fprintf(stderr, "[Error] Cannot open input pcm file\n");
    return;
  }
  // Get length of input file
  in_fd.seekg(0, in_fd.end);
  int input_length = in_fd.tellg();
  in_fd.seekg(in_fd.beg);
  std::vector<char> in_buff(input_length);
  char amr_buff[1600];
  std::vector<char> out_buff(input_length / 320 * 61);
  out_buff.clear();
  in_fd.read(in_buff.data(), input_length);
  if (!in_fd) {
    fprintf(stderr, "[Error] Cannot read data\n");
    return;
  }
  in_fd.close();
  char* p_frame = in_buff.data();
  int codec_uint_length = kPcmFrameLength * kCodecUnit;
  while (input_length >= codec_uint_length) {
    int len = encoder->Codec((uint8_t*)p_frame, codec_uint_length,
                             (uint8_t*)amr_buff, sizeof(amr_buff));
    if (len < 0) {
      fprintf(stderr, "[Warning] Encode error\n");
      continue;
    }
    out_buff.insert(out_buff.end(), amr_buff, amr_buff + len);
    input_length -= codec_uint_length;
    p_frame += codec_uint_length;
  }
  char amr_magic_number[] = "#!AMR-WB\n";
  std::ofstream out_fd;
  out_fd.open(kOutputAmr, std::ofstream::binary);
  if (!out_fd) {
    fprintf(stderr, "[Error] Cannot open amr output file\n");
    return;
  }
  out_fd.write(amr_magic_number, strlen(amr_magic_number));
  out_fd.write(out_buff.data(), out_buff.size());
  if (!out_fd) {
    fprintf(stderr, "[Error] Write file error\n");
  }
  out_fd.close();
}

int main() {
  printf("[Test] Start\n");
  // NormalTest();
  EncodeTest();
  printf("[Test] Finished\n");
  return 0;
}