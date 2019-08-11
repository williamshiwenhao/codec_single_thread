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

#include <samplerate.h>
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
 * Change audio sample rate
 *
 */
class RateConverter {
 public:
  RateConverter() = default;
  RateConverter(RateConverter &) = delete;
  RateConverter &operator=(RateConverter &) = delete;
  ~RateConverter();
  /**
   * Init the RateConverter
   *
   * @param rate output_sample_rate / input_sample_rate
   * @param quality The quality of converter
   * @return int 0 if success
   */
  int Init(double rate, int quality = SRC_SINC_BEST_QUALITY);

  /**
   * Conver frames' sample rate. Input buff and output buff can be the same one
   *
   * @param input Input buff
   * @param intput_frames  Input frames, not bytes
   * @param output Output buff
   * @param output_frames Max output frames
   * @param is_end Is the end of frames
   * @return int Number of output frames
   */
  int Convert(int16_t *input, int intput_frames, int16_t *output,
              int output_frames, bool is_end = false);

 private:
  static const int kBuffSize = 1600;  // byte
  SRC_STATE *p_state_ = nullptr;
  int rate_;
  float *in_buff = nullptr, *out_buff = nullptr;
  SRC_DATA data_;
};

/**
 * @brief  Basic Class for codec
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

class AmrWbEnCoder : public Coder {
 public:
  AmrWbEnCoder() = default;
  virtual ~AmrWbEnCoder();
  virtual int Init();
  virtual int Codec(const uint8_t *input, const int length, uint8_t *output,
                    int output_length);
  virtual int GetWhite(uint8_t *output, const int length, const int frames);

 private:
  void *coder_base_ = nullptr;
  RateConverter converter;
};

class AmrWbDeCoder : public Coder {
 public:
  AmrWbDeCoder() = default;
  virtual ~AmrWbDeCoder();
  virtual int Init();
  virtual int Codec(const uint8_t *input, const int length, uint8_t *output,
                    int output_length);
  virtual int GetWhite(uint8_t *output, const int length, const int frames);

 private:
  void *coder_base_ = nullptr;
  RateConverter converter;
};

#endif  //__CODEC_H__