/**
 * @file session.cpp
 * @author SWH
 * @brief
 * @version 0.1
 * @date 2019-08-06
 *
 * @copyright Copyright (c) 2019
 *
 */
#include "session.h"

int Session::Init(const SessionParam& param) {
  if (buff != nullptr) {
    PrintLog("[Error] Cannot recreate session");
    return -1;
  }
  if (param.rtp_port_base & 1 != 0) {
    PrintLog("[Error] Rtp port should be even");
    return -1;
  }
  buff = new uint8_t[kBuffSize];
  int status = 0;
  /// Init socket and bind port
  if (low_rate_socket_.Init()) {
    return -1;
  }
  if (rtp_socket_.Init()) {
    return -1;
  }
  if (rtcp_socket_.Init()) {
    return -1;
  }
  if (low_rate_socket_.Bind(param.low_rate_port)) {
    return -1;
  }
  if (rtp_socket_.Bind(param.rtp_port_base)) {
    return -1;
  }
  if (rtcp_socket_.Bind(param.rtp_port_base + 1)) {
    return -1;
  }
  if (param.forward == param.From4G) {
    low_rate_socket_.SetSendIp(param.ip, param.remote_port);
  } else {
    rtp_socket_.SetSendIp(param.ip, param.remote_port);
    rtcp_socket_.SetSendIp(param.ip, param.remote_port + 1);
  }
  ///
  return 0;
}

Session::~Session() {
  delete[] buff;
  low_rate_socket_.Close();
  rtp_socket_.Close();
  rtcp_socket_.Close();
}