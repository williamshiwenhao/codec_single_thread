/**
 * @file codec.h
 * @author SWH
 * @brief
 * @version 0.1
 * @date 2019-08-05
 *
 * @copyright Copyright (c) 2019
 *
 */
#ifndef __CODEC_H__
#define __CODEC_H__

#include <cstdint>
#include "codec2.h"
#include "udp.h"

const int kPcmFrameLength = 320;  // Byte
const int kPcmFrameSample = 160;
const int kCodec2FrameLength = 6;  // Byte

/**
 * @brief Print message of Codec2
 *
 */
void Codec2PrintCoderMsg();

/**
 * @brief  Baseic Class for codec
 *
 */
class Coder {
 public:
  Coder() = default;
  Coder(const Coder &) = delete;
  Coder &operator=(const Coder &) = delete;
  virtual ~Coder(){};
  /**
   * @brief Init the coder
   *
   * @return int 0 if succeed
   */
  virtual int Init() = 0;
  /**
   * @brief code
   *
   * @param input input code
   * @param length input code length
   * @param output output code
   * @param output_length Input the output buff length
   * @return int The length of packet when > 0 or error when < 0
   */
  virtual int Codec(const uint8_t *input, const int length, uint8_t *output,
                    int output_length) = 0;
  /**
   * @brief Get white frame, in cases some frames are lost
   *
   * @param output White frame
   * @param length output buff length
   * @param frames need frames, 1 frame have 160 samples
   * @return int The length of packet when > 0 or error when < 0
   */
  virtual int GetWhite(uint8_t *output, const int length, const int frames) = 0;
};

class Codec2EnCoder : public Coder {
 public:
  virtual ~Codec2EnCoder();
  virtual int Init();
  virtual int Codec(const uint8_t *input, const int length, uint8_t *output,
                    int output_length);
  virtual int GetWhite(uint8_t *output, const int length, const int frames);

 private:
  struct CODEC2 *coder_base_ = nullptr;  /// The codec mode
};

class Codec2DeCoder : public Coder {
 public:
  virtual ~Codec2DeCoder();
  virtual int Init();
  virtual int Codec(const uint8_t *input, const int length, uint8_t *output,
                    int output_length);
  virtual int GetWhite(uint8_t *output, const int length, const int frames);

 private:
  struct CODEC2 *coder_base_ = nullptr;  /// The codec mode
};

#endif  //__CODEC_H__