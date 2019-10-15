/**
 * @file udp_packer.h
 * @author shiwenhao
 * @brief
 * @version 0.1
 * @date 2019-10-13
 *
 * @copyright Copyright (c) 2019
 * Generage and parms UDP/IP packeg, for sc2, can test by raw socket
 */
#ifndef __UDP_PACKET__
#define __UDP_PACKET__

#include <netinet/ip.h>   // struct ip and IP_MAXPACKET (which is 65535)
#include <netinet/udp.h>  // struct udphdr
#include <cstdint>

const int kIpHeaderLen = 20;
const int kUdpHeaderLen = 8;
const int kUdpIpLen = kIpHeaderLen + kUdpHeaderLen;

struct TransInfo {
  uint32_t src_ip;
  uint32_t dst_ip;
  uint16_t src_port;
  uint16_t dst_port;
};

class UdpPacker {
 public:
  /**
   * @brief Set ip and port
   *
   * @param src_ip Source ip adress
   * @param src_port Souce UDP port
   * @param dst_ip Destintion ip adree
   * @param dst_port Destintion UDP port
   * @return int 0 if success
   * Set ip and udp port, fill basic information of header
   */
  int SetIpPort(const char* src_ip, uint16_t src_port, const char* dst_ip,
                uint16_t dst_port);

  int GenerateHeader(uint8_t* buff, int pack_len);
  static int GetUdpIpLen() { return kIpHeaderLen + kUdpHeaderLen; }

 private:
  struct ip iphdr_;
  struct udphdr udphdr_;
  struct {
    uint32_t src_ip;
    uint32_t dst_ip;
#if __BYTE_ORDER == __BIG_ENDIAN
    uint8_t zero;
    uint8_t proto;
#else
    uint8_t proto;
    uint8_t zero;
#endif
    uint16_t udp_len;
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t len;
    uint16_t sum;
  } udp_feak_header_;
};

/**
 * @brief Parse udp pack and info(if need it) in host order
 *
 * @param buff Pointer to data
 * @param len Data length
 * @param info Pack info, or null if don't need it. Info are in host order
 * @return int The length of header or -1 if err
 * Segment is not supported, checksum are not used
 */
int UdpParser(uint8_t* buff, int len, TransInfo* info = nullptr);

#endif  //__UDP_PACKET__