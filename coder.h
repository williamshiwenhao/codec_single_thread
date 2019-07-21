#ifndef __CODER_H__
#define __CODER_H__

#include <cstdint>
#include "codec2.h"
#include "udp.h"

class Coder {
 public:
  static void EncodeLoop(const uint32_t recv_port, const char *ip,
                         const uint32_t remote_port);
  static void DecodeLoop(const uint32_t recv_port, const char *ip,
                         const uint32_t remote_port);
  static void PrintCoderMsg();
  Coder() = default;
  Coder(const Coder &) = delete;             // copy are not supported
  Coder &operator=(const Coder &) = delete;  // assignment not supported;
  int Init(const uint32_t recv_port, const char *ip,
           const uint32_t remote_port);
  void Encode();
  void Decode();
  ~Coder();

 private:
  int Recv(unsigned char *, const int);
  int Send(unsigned char *, const int);
  enum { kCodecMode = CODEC2_MODE_2400 };
  struct CODEC2 *coder_base_ = nullptr;
  UdpSocket socket_;
  unsigned char coded_[16];
  unsigned char pcm_[1600];
};

#endif