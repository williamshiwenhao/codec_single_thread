#include <atomic>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "codec.h"
#include "json.h"
#include "logger.h"
#include "udp.h"
#include "utils.h"
const char* kConfigFile = "config.json";

const int kPacketLength = 964;
const int kFramePrePacket = 3;

void EncodeLoop(uint16_t listen_port, const char* ip,
                const uint16_t remote_port, std::atomic<bool>& work) {
  UdpSocket udp_server;
  if (udp_server.Init()) {
    return;
  }
  if (udp_server.Bind(listen_port)) return;
  udp_server.SetSendIp(ip, remote_port);
  uint8_t buff[1600];
  uint8_t send_buff[1600];
  char log[256];
  uint32_t id = 0;
  bool first_pkt = true;
  Coder* encoder = new Codec2EnCoder;
  if (encoder->Init()) return;
  while (work) {
    int length = udp_server.RecvFrom((char*)buff, sizeof(buff));
    /// Check length
    if (length != kPacketLength) {
      PrintLog("[Warning] Packet length error");
      continue;
    }
    uint8_t* p_read = buff + sizeof(uint32_t);
    uint32_t* recv_id = (uint32_t*)buff;
    uint8_t* p_write = send_buff + sizeof(uint32_t);
    uint32_t* write_id = (uint32_t*)send_buff;
    p_read += sizeof(uint32_t);
    if (first_pkt) {
      id = *recv_id;
    } else {
      ++id;
      if (id != *recv_id) {
        sprintf(log, "[Warning] %u packet lost", *recv_id - id - 1);
        for (; id < *recv_id; ++id) {
          *write_id = id;
          int pkt_len = encoder->GetWhite(
              p_write, sizeof(send_buff) - sizeof(uint32_t), kFramePrePacket);
          if (pkt_len == kFramePrePacket * kCodec2FrameLength)
            udp_server.Send((const char*)send_buff, kPacketLength);
        }
      }
    }
    id = *recv_id;
    *write_id = id;
    int pkt_len = encoder->Codec(p_read, length - sizeof(uint32_t), p_write,
                                 sizeof(send_buff) - sizeof(uint32_t));
    if (pkt_len == kFramePrePacket * kCodec2FrameLength) {
      udp_server.Send((char*)send_buff, kPacketLength);
    }
  }
  udp_server.Close();
}

void HandleConsole(std::atomic<bool> work) {
  std::string str;
  while (std::cin >> str) {
    if (str == std::string{"q"} || str == std::string("quit")) {
      work = false;
      return;
    }
  }
}

int main() {
  /************************************
   * Get config
   ************************************/
  Json::Value root;
  ReadConfig(root, kConfigFile);
  unsigned recv_port = root["recv_port"].asUInt();
  std::string ip = root["send_ip"].asString();
  unsigned send_port = root["send_port"].asUInt();
  printf("----------------------------------------\n");
  printf("|Service message\n");
  printf("----------------------------------------\n");
  printf("[Recv port] %u\n", recv_port);
  printf("[Send to ip] %s\n", ip.data());
  printf("[Send to port] %u\n", send_port);
  printf("----------------------------------------\n");
  printf("|Codec2 message\n");
  printf("----------------------------------------\n");
  Codec2PrintCoderMsg();
  /************************************
   * Create work thread
   ************************************/
  std::atomic<bool> work{true};
  std::thread console_hander(HandleConsole, std::ref(work));
  EncodeLoop(recv_port, ip.c_str(), send_port, work);
  console_hander.join();
  return 0;
}