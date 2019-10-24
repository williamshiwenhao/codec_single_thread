/**
 * @file session.h
 * @author SWH
 * @brief A codec session
 * @version 0.1
 * @date 2019-08-05
 *
 * @copyright Copyright (c) 2019
 *
 */
#ifndef __SESSION_H__
#define __SESSION_H__
#include <cstdint>
#include <deque>
#include <vector>

#include "codec.h"
#include "logger.h"
#include "rtp.h"
#include "sc.h"
#include "udp.h"
#include "udp_packer.h"
#include "utils.h"

#define SENT_RTCP

const int kBuffSize = 1600;

struct SessionParam {
  uint16_t recv_port;
  uint16_t remote_port;
  char ip[16];
  uint32_t ueid;
  CodecType encoder_type, decoder_type;
  bool if_add_udpip;
  bool if_send_sc;
  char udpip_sip[16], udpip_dip[16];
  uint16_t udpip_sport, udpip_dport;
};

class UploadSession {
 public:
  ~UploadSession();
  int Init(const SessionParam param);
  int Recv(uint8_t *data, int len, uint8_t *output, int output_len);
  int Send(uint8_t *data, int len, uint8_t *output, int output_len);
  void ProcessLoop(int send_frame = 160);

 private:
  bool first_pack_ = true;
  UdpPacker udp_;
  SC sc_recv_, sc_send_;
  RtpSession rtp_;
  SessionParam param_;
  UdpSocket recv_socket_, send_socket_;
  Coder *encoder_ = nullptr, *decoder_ = nullptr;
};

class DownloadSession {
 public:
  ~DownloadSession() {
    if (encoder_) delete encoder_;
    if (decoder_) delete decoder_;
  }
  int Init(const SessionParam param);
  int Recv(uint8_t *data, int len, uint8_t *output, int output_len);
  int Send(uint8_t *data, int len, uint8_t *output, int output_len);
  void ProcessLoop(int send_frame = kScFrames * kPcmFrameSample);

 private:
  bool first_pack_ = true;
  UdpPacker udp_;
  SC sc_recv_, sc_send_;
  RtpSession rtp_;
  SessionParam param_;
  UdpSocket recv_socket_, send_socket_;
  Coder *encoder_ = nullptr, *decoder_ = nullptr;
};

#endif  // __SESSION_H__