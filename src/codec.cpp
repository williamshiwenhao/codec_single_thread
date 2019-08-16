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

#include "amr.h"
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

/******************************************************************* */
/*Codec2EnCoder                                                     */
/***************************************************************** */

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

int Codec2EnCoder::Codec(uint8_t* input, const int length, uint8_t* output,
                         int output_length) {
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

/******************************************************************* */
/*Codec2Decoder                                                     */
/***************************************************************** */
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

int Codec2DeCoder::Codec(uint8_t* input, const int length, uint8_t* output,
                         int output_length) {
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

/******************************************************************* */
/*RateConverter                                                     */
/***************************************************************** */
int RateConverter::Init(double rate, int quality) {
  int error = 0;
  rate_ = rate;
  // only support 1 channel
  p_state_ = src_new(quality, 1, &error);
  if (p_state_ == nullptr) {
    PrintLog("[Error] RateConverter init error");
    return -1;
  }
  if (src_set_ratio(p_state_, rate)) {
    PrintLog("[Error] RateConverter init rate error");
    return -1;
  }
  src_set_ratio(p_state_, rate);
  in_buff = new float[kBuffSize];
  out_buff = new float[kBuffSize];
  data_.data_in = in_buff;
  data_.data_out = out_buff;
  data_.output_frames = kBuffSize;
  data_.src_ratio = rate;
  data_.end_of_input = 0;
  int16_t buff[640];
  int frames = 640;
  memset(buff, 0, sizeof(buff));
  Convert(buff, frames / 2, buff, frames);
  return 0;
}

RateConverter::~RateConverter() {
  if (in_buff) delete[] in_buff;
  if (out_buff) delete[] out_buff;
  src_delete(p_state_);
}

int RateConverter::Convert(int16_t* input, int input_frames, int16_t* output,
                           int output_frames, bool is_end) {
  if (input_frames > kBuffSize || input_frames * rate_ > kBuffSize) {
    PrintLog("[Error] Convert sample rate error, too much input frames");
    return -1;
  }
  // Convert int16_t to float
  src_short_to_float_array(input, in_buff, input_frames);
  // Process
  if (is_end)
    data_.end_of_input = 1;
  else
    data_.end_of_input = 0;
  data_.input_frames = input_frames;
  if (src_process(p_state_, &data_)) {
    PrintLog("[Error] Convert error");
    return -1;
  }
  if (data_.input_frames_used != input_frames) {
    PrintLog("[Warning] Convert warning, not all frames are used");
  }
  if (data_.output_frames_gen > output_frames) {
    PrintLog("[Error] Convert sample rate error, too much input frames");
    return -1;
  }
  src_float_to_short_array(out_buff, output, data_.output_frames_gen);
  return data_.output_frames_gen;
}

/******************************************************************* */
/*AmrWbEncoder                                                      */
/***************************************************************** */
#ifdef __USE_AMRWB__

#ifdef __AMR_IF2__
static const int kAmrBlockSize[16] = {18, 23, 33, 37, 41, 47, 51, 59,
                                      61, 6,  6,  0,  0,  0,  1,  1};
#else
static const int kAmrBlockSize[16] = {18, 24, 33, 37, 41, 47, 51, 59,
                                      61, 6,  6,  0,  0,  0,  1,  1};
#endif
int AmrWbEnCoder::Init() {
  if (coder_base_ != nullptr) {
    PrintLog("[Error] AMR WB Encoder error, try to double init");
    return -1;
  }
  coder_base_ = E_IF_init();
  if (coder_base_ == nullptr) {
    PrintLog("[Error] AmrEnCoder init error, cannot alloc coder_base");
    return -1;
  }
  // EnCoder receive PCM of 8k sample rate, AMR-WB 16k pcm
  const double rate = 16000.0 / 8000.0;
  if (converter.Init(rate)) {
    PrintLog("[Error] AmrEncoder converter init error");
    return -1;
  }
  return 0;
}

AmrWbEnCoder::~AmrWbEnCoder() {
  if (coder_base_) {
    E_IF_exit(coder_base_);
  }
}

int AmrWbEnCoder::Codec(uint8_t* input, const int length, uint8_t* output,
                        int output_length) {
  if (length <= 0) {
    PrintLog("[Error] Amr Encoder error, input length < 0");
    return -1;
  }
  if (length % 320 != 0) {
    PrintLog("[Warning] Amr encoder: not a complate frame");
  }
  int16_t* buff = new int16_t[length];
  int convert_size =
      converter.Convert((int16_t*)input, length >> 1, buff, length);
  if (convert_size < 0) {
    PrintLog("[Error] Amr Encoder : converter error");
    delete[] buff;
    return -1;
  }
  if (convert_size != length) {
    PrintLog("[Warning] Amr Encoder: converter dose not convert all frames");
  }
  int codec_size = 0;
  int total_size = 0;
  int last_frames = length;
  int16_t* in_frame = buff;
  while (last_frames >= kAmrWbFrameSample) {
    codec_size = E_IF_encode(coder_base_, kAmrEncodeMode, in_frame, amr_buff_,
                             kAmrAllowDtx);
    if (output_length < codec_size) {
      PrintLog("[Warning] Amr encoder: no enough memory for output");
      break;
    }
    memcpy(output, amr_buff_, codec_size);
    output += codec_size;
    output_length -= codec_size;
    total_size += codec_size;
    in_frame += kAmrWbFrameSample;
    last_frames -= kAmrWbFrameSample;
  }
  delete[] buff;
  return total_size;
}

int AmrWbEnCoder::GetWhite(uint8_t* output, const int length,
                           const int frames) {
  uint16_t white_buff[640];
  int total_length = 0;
  memset(white_buff, 0, sizeof(white_buff));
  for (int i = 0; i < length; ++i) {
    int coded_size = E_IF_encode(coder_base_, kAmrEncodeMode,
                                 (int16_t*)white_buff, amr_buff_, kAmrAllowDtx);
    if (coded_size < 0) {
      PrintLog("[Error] Amr encoder: get white error");
      return -1;
    }
    total_length += coded_size;
    if (total_length > length) {
      PrintLog("[Error] Amr Encoder: no enough memory for white frames");
      return -1;
    }
    memcpy(output, amr_buff_, coded_size);
    output += coded_size;
  }
  return total_length;
}

/******************************************************************* */
/*AmrWBDeCoder                                                      */
/***************************************************************** */
int AmrWbDeCoder::Init() {
  if (coder_base_ != nullptr) {
    PrintLog("[Error] AMR WB Encoder error, try to double init");
    return -1;
  }
  coder_base_ = D_IF_init();
  if (coder_base_ == nullptr) {
    PrintLog("[Error] AmrDeCoder init error, cannot alloc coder_base");
    return -1;
  }
  // DeCoder: AMR decoder output sample rate 16KHz PCM, change it to 8KHz
  const double rate = 8000.0 / 16000.0;
  if (converter.Init(rate)) {
    PrintLog("[Error] AmrDecoder converter init error");
    return -1;
  }
  return 0;
}

AmrWbDeCoder::~AmrWbDeCoder() {
  if (coder_base_) D_IF_exit(coder_base_);
}

int AmrWbDeCoder::Codec(uint8_t* input, const int length, uint8_t* output,
                        int output_length) {
  if (length < 0) {
    PrintLog("[Error] Amr Wb Decoder error: length < 0");
    return -1;
  }
  int remain_length = length;
  int output_size = 0;
  int16_t speech_[320];
  while (remain_length > 0) {
#ifdef __AMR_IF2__
    int mode = *input >> 4;
#else
    int mode = *input >> 3 & 0x0f;
#endif  //__AMR_IF2__
    if (remain_length < kAmrBlockSize[mode]) {
      PrintLog("[Warning] Amr wb decoder: not a complete packet");
      break;
    }
    D_IF_decode(coder_base_, input, speech_, kAmrGoodFrame);
    remain_length -= kAmrBlockSize[mode];
    input += kAmrBlockSize[mode];
    if (mode > 8) {
      // Confort noise, 2.4kbps codec cannot handle comfort noise, just output 0
      memset(speech_, 0, sizeof(speech_));
    }
    if (output_length < kPcmFrameLength) {
      PrintLog("[Error] Amr wb decoder: no enough memory");
      return -1;
    }
    int convert_size = converter.Convert(speech_, kAmrWbFrameSample,
                                         (short*)output, output_length >> 1);
    if (convert_size < 0) {
      PrintLog("[Warning] Amr wb decoder error, convert error");
      memset(output, 0, convert_size << 1);
    }
    if (convert_size != kPcmFrameSample) {
      PrintLog(
          "[Warning] Amr wb decoder convert warning, dose not convert all "
          "frames");
    }
    output += convert_size << 1;
    output_size += convert_size << 1;
  }
  return output_size;
}

int AmrWbDeCoder::GetWhite(uint8_t* output, const int length,
                           const int frames) {
  if (frames <= 0) return 0;
  int output_length = frames * kPcmFrameLength;
  if (length < frames * kPcmFrameLength) {
    PrintLog("[Error] AmrWbDecoder: get white error, no enough memory");
    return -1;
  }
  memset(amr_buff_, 0, 64);

  // tell amr coder some frames are lost
  D_IF_decode(coder_base_, amr_buff_, (short*)output, kAmrLostFrame);
  // TODO: Reset convert
  memset(output, 0, output_length);
  return output_length;
}

#endif  //__USE_AMEWB__

/******************************************************************* */
/*Pcm U law Codec                                                   */
/***************************************************************** */

int PcmUEnCoder::Codec(uint8_t* input, const int length, uint8_t* output,
                       int output_length) {
  if (output_length<length>> 1) {
    PrintLog("[Warning] Pcm u encoder: no enough memory");
    return -1;
  }
  static const int16_t seg_end[8] = {0xFF,  0x1FF,  0x3FF,  0x7FF,
                                     0xFFF, 0x1FFF, 0x3FFF, 0x7FFF};
  const int16_t kBIAS{0x84};
  int16_t* speech = (int16_t*)input;
  int frames = length >> 1;
  for (int i = 0; i < frames; ++i) {
    int mask;
    int seg;
    unsigned char uval;
    int16_t& pcm_val = speech[i];

    /* Get the sign and the magnitude of the value. */
    if (pcm_val < 0) {
      pcm_val = kBIAS - pcm_val;
      mask = 0x7F;
    } else {
      pcm_val += kBIAS;
      mask = 0xFF;
    }

    /* Convert the scaled magnitude to segment number. */
    for (; seg < 8; ++seg) {
      if (pcm_val < seg_end[seg]) {
        break;
      }
    }

    /*
     * Combine the sign, segment, quantization bits;
     * and complement the code word.
     */
    if (seg >= 8) /* out of range, return maximum value. */
      output[i] = 0x7F ^ mask;
    else {
      uval = (seg << 4) | ((pcm_val >> (seg + 3)) & 0xF);
      output[i] = uval ^ mask;
    }
  }
  return frames;
}

int PcmUEnCoder::GetWhite(uint8_t* output, const int length, const int frames) {
  if (length < frames) {
    PrintLog("[Error] Pcm u law encoder GetWhite: no enough memory");
    return -1;
  }
  memset(output, 0, frames);
  return frames;
}

int PcmUDeCoder::Codec(uint8_t* input, const int length, uint8_t* output,
                       int output_length) {
  if (output_length < length << 1) {
    PrintLog("[Warning] Pcm u decoder: no enough memory");
    return -1;
  }
  int16_t* speech = (int16_t*)output;
  static const int16_t ulaw_table[] = {
      -32124, -31100, -30076, -29052, -28028, -27004, -25980, -24956, -23932,
      -22908, -21884, -20860, -19836, -18812, -17788, -16764, -15996, -15484,
      -14972, -14460, -13948, -13436, -12924, -12412, -11900, -11388, -10876,
      -10364, -9852,  -9340,  -8828,  -8316,  -7932,  -7676,  -7420,  -7164,
      -6908,  -6652,  -6396,  -6140,  -5884,  -5628,  -5372,  -5116,  -4860,
      -4604,  -4348,  -4092,  -3900,  -3772,  -3644,  -3516,  -3388,  -3260,
      -3132,  -3004,  -2876,  -2748,  -2620,  -2492,  -2364,  -2236,  -2108,
      -1980,  -1884,  -1820,  -1756,  -1692,  -1628,  -1564,  -1500,  -1436,
      -1372,  -1308,  -1244,  -1180,  -1116,  -1052,  -988,   -924,   -876,
      -844,   -812,   -780,   -748,   -716,   -684,   -652,   -620,   -588,
      -556,   -524,   -492,   -460,   -428,   -396,   -372,   -356,   -340,
      -324,   -308,   -292,   -276,   -260,   -244,   -228,   -212,   -196,
      -180,   -164,   -148,   -132,   -120,   -112,   -104,   -96,    -88,
      -80,    -72,    -64,    -56,    -48,    -40,    -32,    -24,    -16,
      -8,     0,      32124,  31100,  30076,  29052,  28028,  27004,  25980,
      24956,  23932,  22908,  21884,  20860,  19836,  18812,  17788,  16764,
      15996,  15484,  14972,  14460,  13948,  13436,  12924,  12412,  11900,
      11388,  10876,  10364,  9852,   9340,   8828,   8316,   7932,   7676,
      7420,   7164,   6908,   6652,   6396,   6140,   5884,   5628,   5372,
      5116,   4860,   4604,   4348,   4092,   3900,   3772,   3644,   3516,
      3388,   3260,   3132,   3004,   2876,   2748,   2620,   2492,   2364,
      2236,   2108,   1980,   1884,   1820,   1756,   1692,   1628,   1564,
      1500,   1436,   1372,   1308,   1244,   1180,   1116,   1052,   988,
      924,    876,    844,    812,    780,    748,    716,    684,    652,
      620,    588,    556,    524,    492,    460,    428,    396,    372,
      356,    340,    324,    308,    292,    276,    260,    244,    228,
      212,    196,    180,    164,    148,    132,    120,    112,    104,
      96,     88,     80,     72,     64,     56,     48,     40,     32,
      24,     16,     8,      0};
  for (int i = 0; i < length; ++i) {
    speech[i] = ulaw_table[input[i]];
  }
  return length << 1;
}

int PcmUDeCoder::GetWhite(uint8_t* output, const int length, const int frames) {
  if (length < frames << 1) {
    PrintLog("[Error] Pcm u law decoder GetWhite: no enough memory");
    return -1;
  }
  memset(output, 0, frames << 1);
  return frames << 1;
}

/******************************************************************* */
/*Pcm A law Codec                                                   */
/***************************************************************** */

int PcmAEnCoder::Codec(uint8_t* input, const int length, uint8_t* output,
                       int output_length) {
  if (output_length<length>> 1) {
    PrintLog("[Warning] Pcm a law encoder: no enough memory");
    return -1;
  }
  static const int16_t seg_end[8] = {0xFF,  0x1FF,  0x3FF,  0x7FF,
                                     0xFFF, 0x1FFF, 0x3FFF, 0x7FFF};
  int16_t* speech = (int16_t*)input;
  int frames = length >> 1;
  for (int i = 0; i < frames; ++i) {
    int mask;
    int seg;
    uint8_t aval;
    int16_t& pcm_val = speech[i];
    static const int kSegShift = 4;
    static const uint8_t kQuantMask = 0x0f;

    if (pcm_val >= 0) {
      mask = 0xD5; /* sign (7th) bit = 1 */
    } else {
      mask = 0x55; /* sign bit = 0 */
      pcm_val = -pcm_val - 8;

      if (pcm_val < 0) pcm_val = 0;
    }

    /* Convert the scaled magnitude to segment number. */
    for (; seg < 8; ++seg) {
      if (pcm_val < seg_end[seg]) {
        break;
      }
    }

    /* Combine the sign, segment, and quantization bits. */

    if (seg >= 8) /* out of range, return maximum value. */
      return (0x7F ^ mask);
    else {
      aval = seg << kSegShift;
      if (seg < 2)
        aval |= (pcm_val >> 4) & kQuantMask;
      else
        aval |= (pcm_val >> (seg + 3)) & kQuantMask;
      output[i] = aval ^ mask;
    }
  }
  return frames;
}

int PcmAEnCoder::GetWhite(uint8_t* output, const int length, const int frames) {
  if (length < frames) {
    PrintLog("[Error] Pcm a law encoder GetWhite: no enough memory");
    return -1;
  }
  memset(output, 0, frames);
  return frames;
}

int PcmADeCoder::Codec(uint8_t* input, const int length, uint8_t* output,
                       int output_length) {
  if (output_length < length << 1) {
    PrintLog("[Warning] Pcm a law decoder: no enough memory");
    return -1;
  }
  static const int16_t a_law_table[] = {
      -5504,  -5248,  -6016,  -5760,  -4480,  -4224,  -4992,  -4736,  -7552,
      -7296,  -8064,  -7808,  -6528,  -6272,  -7040,  -6784,  -2752,  -2624,
      -3008,  -2880,  -2240,  -2112,  -2496,  -2368,  -3776,  -3648,  -4032,
      -3904,  -3264,  -3136,  -3520,  -3392,  -22016, -20992, -24064, -23040,
      -17920, -16896, -19968, -18944, -30208, -29184, -32256, -31232, -26112,
      -25088, -28160, -27136, -11008, -10496, -12032, -11520, -8960,  -8448,
      -9984,  -9472,  -15104, -14592, -16128, -15616, -13056, -12544, -14080,
      -13568, -344,   -328,   -376,   -360,   -280,   -264,   -312,   -296,
      -472,   -456,   -504,   -488,   -408,   -392,   -440,   -424,   -88,
      -72,    -120,   -104,   -24,    -8,     -56,    -40,    -216,   -200,
      -248,   -232,   -152,   -136,   -184,   -168,   -1376,  -1312,  -1504,
      -1440,  -1120,  -1056,  -1248,  -1184,  -1888,  -1824,  -2016,  -1952,
      -1632,  -1568,  -1760,  -1696,  -688,   -656,   -752,   -720,   -560,
      -528,   -624,   -592,   -944,   -912,   -1008,  -976,   -816,   -784,
      -880,   -848,   5504,   5248,   6016,   5760,   4480,   4224,   4992,
      4736,   7552,   7296,   8064,   7808,   6528,   6272,   7040,   6784,
      2752,   2624,   3008,   2880,   2240,   2112,   2496,   2368,   3776,
      3648,   4032,   3904,   3264,   3136,   3520,   3392,   22016,  20992,
      24064,  23040,  17920,  16896,  19968,  18944,  30208,  29184,  32256,
      31232,  26112,  25088,  28160,  27136,  11008,  10496,  12032,  11520,
      8960,   8448,   9984,   9472,   15104,  14592,  16128,  15616,  13056,
      12544,  14080,  13568,  344,    328,    376,    360,    280,    264,
      312,    296,    472,    456,    504,    488,    408,    392,    440,
      424,    88,     72,     120,    104,    24,     8,      56,     40,
      216,    200,    248,    232,    152,    136,    184,    168,    1376,
      1312,   1504,   1440,   1120,   1056,   1248,   1184,   1888,   1824,
      2016,   1952,   1632,   1568,   1760,   1696,   688,    656,    752,
      720,    560,    528,    624,    592,    944,    912,    1008,   976,
      816,    784,    880,    848};
  int16_t* speech = (int16_t*)output;
  for (int i = 0; i < length; ++i) {
    speech[i] = a_law_table[input[i]];
  }
  return length << 1;
}

int PcmADeCoder::GetWhite(uint8_t* output, const int length, const int frames) {
  if (length < frames << 1) {
    PrintLog("[Error] Pcm a law decoder GetWhite: no enough memory");
    return -1;
  }
  memset(output, 0, frames << 1);
  return frames << 1;
}