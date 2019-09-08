/**
 * @file rtp_test.cpp
 * @author SWH
 * @brief
 * @version 0.1
 * @date 2019-09-06
 *
 * @copyright Copyright (c) 2019
 *
 */
#include <array>
#include <atomic>
#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "codec.h"
#include "json.h"
#include "logger.h"
#include "rtp.h"
#include "utils.h"

const char kInput[] = "audio/8k.raw";
const char kConfigFile[] = "test.json";

const int kCodecUnit = 1;

class Timer {
 public:
  void Start();
  void Stop() {
    work_ = false;
    while (!work_)
      ;
  }
  void SetClock(int millisecond) { millisecond_ = millisecond; }
  void Wait();

 private:
  void Tick();
  std::atomic<bool> work_;
  std::condition_variable cv_;
  int millisecond_ = 20;
};

void Timer::Start() {
  work_ = true;
  std::thread th(&Timer::Tick, this);
  th.detach();
}

void Timer::Wait() {
  std::mutex mutex_;
  std::unique_lock<std::mutex> locker(mutex_);
  cv_.wait(locker);
}

void Timer::Tick() {
  while (work_) {
    std::this_thread::sleep_for(std::chrono::milliseconds(millisecond_));
    cv_.notify_one();
  }
  work_ = true;
}

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
  if (encoder->Init()) {
    fprintf(stderr, "[Error] Encoder init error\n");
    return;
  }
  // Init RTP
  RtpSession rtp;
  MediaParam media_param;
  media_param.payload_type = payload_type;
  media_param.clock_rate = 8000;
  media_param.samples_pre_frames = 160;
  media_param.byte_pre_frame = 320;
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
  // Init Socket
  UdpSocket udp;
  if (udp.Init()) {
    fprintf(stderr, "[Error] Udp Socket error\n");
    return;
  }
  udp.SetSendIp(str_ip.c_str(), port);
  // Init timer
  std::unique_ptr<Timer> timer(new Timer);
  timer->SetClock(kCodecUnit * 20);
  timer->Start();
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
      int codec_len =
          encoder->Codec((uint8_t*)p_frame, codec_uint_length,
                         (uint8_t*)codec_ptr, sizeof(codec_buff) - rtp_len);
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