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
#include "udp.h"

#define SENT_RTCP

const int kBuffSize = 1600;

struct SessionParam {
  uint16_t low_rate_port;         /// port for 2.4k audio
  uint16_t rtp_port_base;         /// port for rtp port, must be even
  enum { To4G, From4G } forward;  /// determin the codec changer forward
  char ip[16];                    /// IP of IMS
  uint16_t remote_port;           /// Port to send data
  MediaParam media_param;
};

class Session {
 public:
  Session() = default;                /// Use default construct function
  Session(const Session &) = delete;  /// Disable copy construct function
  Session &operator=(const Session &) = delete;  /// Disable assignment
  int Init(const SessionParam &);                /// Init a session
  ~Session();
  std::vector<UdpSocket *>
  GetSocket();  /// Get sockets for something like epoll()
  int SendFrame(const uint8_t data, int length);

 private:
  uint8_t *buff = nullptr;
  UdpSocket low_rate_socket_, rtp_socket_, rtcp_socket_;
  Coder *decoder_, *encoder_;
  RtpSession rtp_session_;
};

#endif  // __SESSION_H__