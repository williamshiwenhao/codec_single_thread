/*
@ Create on July 7 2019
@ Author SWH
 */

#ifndef __UDP_H__
#define __UDP_H__
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <cstdint>

class UdpSocket {
 public:
  UdpSocket(const UdpSocket &) = delete;
  UdpSocket() = default;
  ~UdpSocket();
  int Init();
  int Bind(const uint16_t port);  // Bind port
  int SendTo(const char *ip, uint16_t port, const char *buff, const int length);
  int RecvFrom(char *buff, const int length, sockaddr_in *addr = nullptr);
  int Send(const char *buff, const int length);
  void SetSendIp(const char *ip, const uint16_t port);
  void Close();

 private:
  int fd_ = -1;
  bool remote_set_ = false;
  sockaddr_in remote_addr_;
};

#endif