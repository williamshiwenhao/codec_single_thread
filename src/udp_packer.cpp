#include <arpa/inet.h>  // inet_pton() and inet_ntop()
#include <cstring>
#include <memory>
#include <sstream>
#include <string>

#include "logger.h"
#include "udp_packer.h"

uint16_t CheckSum(uint16_t* addr, int len);

int UdpPacker::SetIpPort(const char* src_ip, uint16_t src_port,
                         const char* dst_ip, uint16_t dst_port) {
  int status;
  memset(&udphdr_, 0, sizeof(udphdr_));
  memset(&iphdr_, 0, sizeof(iphdr_));
  memset(&udp_feak_header_, 0, sizeof(udp_feak_header_));
  iphdr_.ip_hl = kIpHeaderLen >> 2;  // iphder len: number of 32-bit words
  iphdr_.ip_v = 4;                   // IPV4
  iphdr_.ip_tos = 0;                 // Type of service
  iphdr_.ip_id = htons(0);
  iphdr_.ip_off = 0;          // Fregment, no fregment
  iphdr_.ip_ttl = 255;        // Time to live: defautl max value
  iphdr_.ip_p = IPPROTO_UDP;  // Transport layer protocol, UDP
  if ((status = inet_pton(AF_INET, src_ip, &(iphdr_.ip_src))) != 1) {
    std::stringstream ss;
    ss << "[Error] UdpPacker cannot parse src_ip: " << strerror(status);
    PrintLog(ss.str().c_str());
    return -1;
  }
  if ((status = inet_pton(AF_INET, dst_ip, &(iphdr_.ip_dst))) != 1) {
    std::stringstream ss;
    ss << "[Error] UdpPacker cannot parse dst_ip: " << strerror(status);
    PrintLog(ss.str().c_str());
    return -1;
  }
  // UDP
  udphdr_.uh_sport = htons(src_port);
  udphdr_.uh_dport = htons(dst_port);
  // UDP fake head
  udp_feak_header_.src_ip = *(uint32_t*)&iphdr_.ip_src;
  udp_feak_header_.dst_ip = *(uint32_t*)&iphdr_.ip_dst;
  udp_feak_header_.zero = 0;
  udp_feak_header_.proto = IPPROTO_UDP;
  udp_feak_header_.src_port = udphdr_.uh_sport;
  udp_feak_header_.dst_port = udphdr_.uh_dport;

  return 0;
}

int UdpPacker::GenerateHeader(uint8_t* buff, int pack_len) {
#if __APPLE__
  iphdr_.ip_len = (kIpHeaderLen + kUdpHeaderLen + pack_len);
#else
  iphdr_.ip_len = htons(kIpHeaderLen + kUdpHeaderLen + pack_len);
#endif  //__APPLE__
  // file checksum
  iphdr_.ip_sum = 0;
  iphdr_.ip_sum = CheckSum((uint16_t*)&iphdr_, sizeof(iphdr_));
  udphdr_.uh_ulen = htons(kUdpHeaderLen + pack_len);
  udp_feak_header_.udp_len = udp_feak_header_.len = udphdr_.uh_ulen;
  udphdr_.uh_sum =
      CheckSum((uint16_t*)&udp_feak_header_, sizeof(udp_feak_header_));
  memcpy(buff, &iphdr_, sizeof(iphdr_));
  memcpy(buff + sizeof(iphdr_), &udphdr_, sizeof(udphdr_));
  return UdpPacker::GetUdpIpLen();
}

uint16_t CheckSum(uint16_t* addr, int len) {
  int nleft = len;
  int sum = 0;
  uint16_t* w = addr;
  uint16_t answer = 0;

  while (nleft > 1) {
    sum += *w++;
    nleft -= sizeof(uint16_t);
  }

  if (nleft == 1) {
    *(uint8_t*)(&answer) = *(uint8_t*)w;
    sum += answer;
  }

  sum = (sum >> 16) + (sum & 0xFFFF);
  sum += (sum >> 16);
  answer = ~sum;
  return (answer);
}

int UdpParser(uint8_t* buff, int len, TransInfo* info) {
  if (len < kUdpHeaderLen + kIpHeaderLen) {
    PrintLog("[Warning] Wrong UDP/IP header length, < len(iphdr)+len(udphdr)");
    return -1;
  }
  ip* iphdr = (ip*)buff;
  int iphdr_len = iphdr->ip_hl;
  iphdr_len <<= 2;
  if (iphdr_len > 60 || iphdr_len + kUdpHeaderLen > len) {
    PrintLog("Warning] UDP/IP length error");
    return -1;
  }
  udphdr* udp = (udphdr*)(buff + iphdr_len);
  if (info != nullptr) {
    info->src_ip = ntohl(*(uint32_t*)&iphdr->ip_src);
    info->dst_ip = ntohl(*(uint32_t*)&iphdr->ip_dst);
    info->src_port = ntohs(udp->uh_sport);
    info->dst_port = ntohs(udp->uh_dport);
  }
  return iphdr_len + kUdpHeaderLen;
}