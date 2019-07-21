/*
@ Create on July 7 2019
@ Author SWH
@ Try boost::asio
 */

#ifndef __UDP_H__
#define __UDP_H__
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <cstdint>

class UdpSocket {
 public:
  UdpSocket(const UdpSocket&) = delete;
  UdpSocket() = default;
  ~UdpSocket();
  int Init();
  int Bind(const uint32_t port);  // Bind port

  int SendTo(const char* ip, uint32_t port, const char* buff, const int length);
  int RecvFrom(char* buff, const int length, sockaddr_in* addr = nullptr);
  void Close();

 private:
  int fd_ = -1;
};

#endif