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
  CodecType encode_type, decode_type;
  bool if_add_udpip;
  char udpip_sip[16], udpip_dip[16];
  uint16_t udpip_sport, udpip_dport;
};

class UploadSession {
 public:
  int Init(const SessionParam param);
  int Recv(uint8_t data, int len);
  int Send(uint8_t data, int len);
  void ProcessLoop();

 private:
  bool first_pack_ = true;
  SC sc_;
  RtpSession rtp_;
  SessionParam param_;
  UdpSocket recv_socket_, send_socket_;
  Coder *encoder, *decoder;
};

class DownloadSession {
 public:
  int Init(const SessionParam param);
  int Recv(uint8_t data, int len);
  int Send(uint8_t data, int len);
  void ProcessLoop();

 private:
};

class FrameBuffer {}

#endif  // __SESSION_H__