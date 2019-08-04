/*
@ Create on Aug 2 2019
@ Author SWH
@ Send and receive rtp packet
 */
#ifndef __RTP_H__
#define __RTP_H__
#include <cstdint>

#include "logger.h"
#include "udp.h"
struct RtpHeader {
  uint16_t v : 2;
  uint16_t p : 1;
  uint16_t x : 1;
  uint16_t cc : 4;
  uint16_t m : 1;
  uint16_t pt : 7;
  uint16_t sequence_num;
  uint32_t timestamp;
  uint32_t ssrc;
};

class Rtp {
 public:
  static bool HeaderCheck() {
    auto size = sizeof(RtpHeader);
    if (size != 12) {
      PrintLog("[Error] RTP header size should be 8 byte!");
      return false;
    }
    return true;
  }
  Rtp() = default;
  Rtp(const Rtp&) = delete;
  int Init(const uint16_t local_port);
  int SetRemote(const char* ip, const uint16_t remote_port);

 private:
  UdpSocket udp_;
  RtpHeader send_header_, recv_header_;
};

#endif  //__RTP_H__