/**
 * @file codec.cpp
 * @author SWH
 * @brief
 * @version 0.1
 * @date 2019-08-05
 *
 * @copyright Copyright (c) 2019
 *
 */
#include <cstdio>
#include <cstring>

#include "codec.h"
#include "logger.h"

void Codec2PrintCoderMsg() {
  CODEC2* test_coder = codec2_create(CODEC2_MODE_2400);
  if (test_coder == nullptr) {
    printf("[Test] Failed\n");
    return;
  }
  int val;
  val = codec2_samples_per_frame(test_coder);
  printf("[Test] Samples pre frame = %d\n", val);
  val = codec2_bits_per_frame(test_coder);
  printf("[Test] Bits pre frame = %d\n", val);

  codec2_destroy(test_coder);
}

int Codec2EnCoder::Init() {
  if (coder_base_ != nullptr) {
    PrintLog("Reinit codec2encoder!");
    return -1;
  }
  coder_base_ = codec2_create(CODEC2_MODE_2400);
  if (coder_base_ == nullptr) {
    PrintLog("[Error] Cannot create a new coder struct");
    return -1;
  }
  return 0;
}

Codec2EnCoder::~Codec2EnCoder() {
  if (coder_base_ != nullptr) {
    codec2_destroy(coder_base_);
    coder_base_ = nullptr;
  }
}

int Codec2EnCoder::Codec(const uint8_t* input, const int length,
                         uint8_t* output, int output_length) {
  if (input == nullptr || output == nullptr) {
    PrintLog("[Error] Coder2Encoder codec error, empty pointer get");
    return -1;
  }
  int frames = 0;
  if (length % kPcmFrameLength != 0) {
    PrintLog("[Error] Not a frame!");
    return -1;
  } else {
    frames = length / kPcmFrameLength;
  }
  if (output_length < frames * kCodec2FrameLength) {
    PrintLog("[Error] Codec error, no enough space for output");
    return -1;
  }
  for (int i = 0; i < frames; ++i) {
    codec2_encode(coder_base_, output + i * kCodec2FrameLength,
                  (short*)(input + i * kPcmFrameLength));
  }
  return frames * kCodec2FrameLength;
}

int Codec2EnCoder::GetWhite(uint8_t* output, const int length,
                            const int frames) {
  short empty_buff[160];
  memset(empty_buff, 0, sizeof(empty_buff));
  if (length < kCodec2FrameLength * frames) {
    PrintLog("[Error] No enough memory for white frames");
    return -1;
  }
  for (int i = 0; i < frames; ++i) {
    codec2_encode(coder_base_, output + i * kCodec2FrameLength, empty_buff);
  }
  return frames * kCodec2FrameLength;
}

int Codec2DeCoder::Init() {
  if (coder_base_ != nullptr) {
    PrintLog("Reinit codec2encoder!");
    return -1;
  }
  coder_base_ = codec2_create(CODEC2_MODE_2400);
  if (coder_base_ == nullptr) {
    PrintLog("[Error] Cannot create a new coder struct");
    return -1;
  }
  return 0;
}

Codec2DeCoder::~Codec2DeCoder() {
  if (coder_base_ != nullptr) {
    codec2_destroy(coder_base_);
    coder_base_ = nullptr;
  }
}

int Codec2DeCoder::Codec(const uint8_t* input, const int length,
                         uint8_t* output, int output_length) {
  if (input == nullptr || output == nullptr) {
    PrintLog("[Error] Coder2Encoder codec error, empty pointer get");
    return -1;
  }
  int frames = 0;
  if (length % kCodec2FrameLength != 0) {
    PrintLog("[Error] Not a frame!");
    return -1;
  } else {
    frames = length / kCodec2FrameLength;
  }
  if (output_length < frames * kPcmFrameLength) {
    PrintLog("[Error] Codec error, no enough space for output");
    return -1;
  }
  for (int i = 0; i < frames; ++i) {
    codec2_decode(coder_base_, (short*)(output + i * kPcmFrameLength),
                  input + i * kCodec2FrameLength);
  }
  return frames * kPcmFrameLength;
}

int Codec2DeCoder::GetWhite(uint8_t* output, const int length,
                            const int frames) {
  if (length < frames * kPcmFrameLength) {
    PrintLog("[Error] No enough memory for codec2 decoder white frames");
    return 0;
  }
  memset(output, 0, kPcmFrameLength * frames);
  return kPcmFrameLength * frames;
}