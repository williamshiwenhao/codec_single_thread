#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

#include "json.h"
#include "udp_packer.h"
#include "utils.h"

const char kConfigfile[] = "test.json";

using namespace std;

int main() {
  UdpPacker packer;
  Json::Value root;
  if (ReadConfig(root, kConfigfile)) {
    return -1;
  }
  Json::Value config = root["udp_packer_test"];
  string src_ip = config["src_ip"].asString();
  string dst_ip = config["dst_ip"].asString();
  uint16_t src_port = config["src_port"].asUInt();
  uint16_t dst_port = config["dst_port"].asUInt();
  int test_num = config["test_num"].asInt();
  if (test_num <= 0) test_num = 1;
  printf("-----------------------------------\n");
  printf("UDP PACKER TEST\n");
  printf("-----------------------------------\n");
  printf("[SRC IP] %s\n", src_ip.c_str());
  printf("[DST IP] %s\n", dst_ip.c_str());
  printf("[SRC PORT] %u\n", src_port);
  printf("[DST PORT] %u\n", dst_port);
  printf("TEST NUM] %d\n", test_num);
  packer.SetIpPort(src_ip.c_str(), src_port, dst_ip.c_str(), dst_port);
  uint8_t buff[1000];
  memset(buff, 'a', sizeof(buff));

  int sd = 0;
  // Submit request for a raw socket descriptor.
  if ((sd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
    perror("socket() failed ");
    exit(EXIT_FAILURE);
  }
  const int on = 1;
  // Set flag so socket expects us to provide IPv4 header.
  if (setsockopt(sd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0) {
    perror("setsockopt() failed to set IP_HDRINCL ");
    exit(EXIT_FAILURE);
  }

  // Bind socket to interface index.
  /*
  if (setsockopt(sd, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr)) < 0) {
    perror("setsockopt() failed to bind to interface ");
    exit(EXIT_FAILURE);
  }
   */
  sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_network(dst_ip.c_str());
  for (int i = 0; i < test_num; ++i) {
    packer.GenerateHeader(buff, sizeof(buff) - UdpPacker::GetUdpIpLen());
    TransInfo info;
    int len = UdpParser(buff, sizeof(buff), &info);
    printf("[Header Length] %d\n", len);
    uint8_t* sip_int = (uint8_t*)&info.src_ip;
    uint8_t* dip_int = (uint8_t*)&info.dst_ip;
    printf("[Souce IP] %u.%u.%u.%u\n", sip_int[3], sip_int[2], sip_int[1],
           sip_int[1]);
    printf("[Destition IP] %u.%u.%u.%u\n", dip_int[3], dip_int[2], dip_int[1],
           dip_int[1]);
    printf("[Source port] %u\n", info.src_port);
    printf("[Destition port] %u\n", info.dst_port);

    // Send packet.
    if (sendto(sd, buff, sizeof(buff), 0, (sockaddr*)&addr, sizeof(addr)) < 0) {
      perror("sendto() failed ");
      exit(EXIT_FAILURE);
    }
  }

  // Close socket descriptor.
  close(sd);
  return 0;
}