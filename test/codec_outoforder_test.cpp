/**
 * @file codec_outoforder_test.cpp
 * @author SWH
 * @brief
 * @version 0.1
 * @date 2019-10-04
 *
 * @copyright Copyright (c) 2019
 * 用来确定Codec2编码确实不能乱序
 */
#include <signal.h>
#include <sys/time.h>
#include <array>
#include <atomic>
#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <random>
#include <thread>
#include <vector>

#include "codec.h"
#include "json.h"
#include "logger.h"
#include "rtp.h"
#include "utils.h"

const char kInput[] = "audio/8k.raw";
const char kConfigFile[] = "test.json";

const int kCodecUnit = 3;

class Timer {
 public:
  void Tick();
  void Wait();
  static Timer* GetTimer() {
    static Timer* timer = nullptr;
    if (timer == nullptr) {
      timer = new Timer();
    }
    return timer;
  }

 private:
  std::condition_variable cv_;
};

void ClockClick(int signo) {
  Timer* timer = Timer::GetTimer();
  timer->Tick();
}

void Timer::Wait() {
  std::mutex mutex_;
  std::unique_lock<std::mutex> locker(mutex_);
  cv_.wait(locker);
}

void Timer::Tick() { cv_.notify_one(); }

/**
 * @发送RTP流，使用VLC接收。测试编码模块和rtp模块
 *
 */
void SendTest() {
  Json::Value config_root;
  if (ReadConfig(config_root, kConfigFile)) {
    fprintf(stderr, "[Error] Cannot read config file\n");
    return;
  }
  config_root = config_root["rtp_test"];
  // Print config
  std::string str_ip = config_root["ip"].asString();
  unsigned port = config_root["port"].asUInt();
  unsigned payload_type = config_root["payload type"].asUInt();
  std::string codec_type = config_root["codec type"].asString();
  printf("[IP] %s\n", str_ip.c_str());
  printf("[Port] %u\n", port);
  printf("[Payload type] %u\n", payload_type);
  printf("[Codec type] %s\n", codec_type.c_str());
  Coder *p_encoder, *p_decoder;
  if (codec_type == "pcmu") {
    p_encoder = new PcmUEnCoder;
  } else if (codec_type == "pcma") {
    p_encoder = new PcmAEnCoder;
  } else if (codec_type == "amr-wb") {
    p_encoder = new AmrWbEnCoder;
  } else if (codec_type == "codec2") {
    p_encoder = new Codec2EnCoder;
  } else {
    fprintf(stderr, "[Error] Codec not found\n");
    return;
  }
  // Init coder
  std::unique_ptr<Coder> encoder{p_encoder};
  std::unique_ptr<Coder> codec_encoder{new Codec2EnCoder()};
  std::unique_ptr<Coder> codec_decoder{new Codec2DeCoder()};
  if (encoder->Init()) {
    fprintf(stderr, "[Error] Encoder init error\n");
    return;
  }
  if (codec_encoder->Init() || codec_decoder->Init()) {
    fprintf(stderr, "[Error] Codec2 init error\n");
    return;
  }
  uint8_t codec2_buff[1600];
  uint8_t codec2_buff2[1600];
  // Init RTP
  RtpSession rtp;
  MediaParam media_param;
  media_param.payload_type = payload_type;
  media_param.clock_rate = 8000;
  media_param.samples_pre_frames = 160;
  if (rtp.Init(media_param)) {
    fprintf(stderr, "[Error] Rtp session init error\n");
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
  in_fd.read(in_buff, input_length);
  if (!in_fd) {
    fprintf(stderr, "[Error] Cannot read data\n");
    return;
  }
  in_fd.close();
  char codec_buff[1600];
  int codec_uint_length = kPcmFrameLength * kCodecUnit;
  // Init random engine
  std::default_random_engine random_engine;
  std::uniform_int_distribution<int> random_idx(
      0, input_length - 320 * kCodecUnit - 1);
  auto GetRandom = [&]() { return random_idx(random_engine); };
  // Init Socket
  UdpSocket udp;
  if (udp.Init()) {
    fprintf(stderr, "[Error] Udp Socket error\n");
    return;
  }
  udp.SetSendIp(str_ip.c_str(), port);
  // Init timer
  signal(SIGALRM, ClockClick);
  Timer* timer = Timer::GetTimer();
  struct itimerval tick;
  memset(&tick, 0, sizeof(tick));
  tick.it_value.tv_usec = 20 * kCodecUnit * 1000;
  tick.it_interval.tv_usec = 20 * kCodecUnit * 1000;
  if (setitimer(ITIMER_REAL, &tick, NULL) < 0) {
    fprintf(stderr, "[Error] Set timer error\n");
    return;
  }
  while (true) {
    char* p_frame = in_buff;
    int now_length = input_length;
    while (now_length >= codec_uint_length) {
      int rtp_len = rtp.GenerateRtpHeader((uint8_t*)codec_buff,
                                          sizeof(codec_buff), kCodecUnit);
      if (rtp_len < 0) {
        fprintf(stderr, "[Error] Rtp error\n");
        delete[] in_buff;
        return;
      }
      char* codec_ptr = codec_buff + rtp_len;
      int codec2_len;
      // codec2_len = codec_encoder->Codec((uint8_t*)in_buff + GetRandom(),
      //                                   codec_uint_length, codec2_buff,
      //                                   sizeof(codec2_buff));
      // if (codec2_len <= 0) {
      //   fprintf(stderr, "[Error] Codec2 encode error\n");
      //   return;
      // }
      codec2_len = codec_encoder->Codec((uint8_t*)p_frame, codec_uint_length,
                                        codec2_buff, sizeof(codec2_buff));
      if (codec2_len <= 0) {
        fprintf(stderr, "[Error] Codec2 encode error\n");
        return;
      }
      codec2_len = codec_decoder->Codec(codec2_buff, codec2_len, codec2_buff2,
                                        sizeof(codec2_buff2));
      if (codec2_len <= 0 || codec2_len != codec_uint_length) {
        fprintf(stderr, "[Error] Codec2 decode error\n");
        return;
      }
      int codec_len =
          encoder->Codec(codec2_buff2, codec_uint_length, (uint8_t*)codec_ptr,
                         sizeof(codec_buff) - rtp_len);
      if (codec_len < 0) {
        fprintf(stderr, "[Error] Encoder error\n");
        delete[] in_buff;
        return;
      }
      timer->Wait();
      udp.Send(codec_buff, rtp_len + codec_len);
      p_frame += codec_uint_length;
      now_length -= codec_uint_length;
    }
  }
}

int main() {
  printf("[Test] Start\n");
  SendTest();
  printf("[Test] Finished\n");
  return 0;
}