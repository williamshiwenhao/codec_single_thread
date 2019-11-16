/**
 * @file sc2.h
 * @author SWH
 * @brief SC2 support
 * @version 0.1
 * @date 2019-08-14
 *
 * @copyright Copyright (c) 2019
 *
 */
#ifndef __SC_H__
#define __SC_H__

#include <cstdint>
#include "utils.h"
const size_t kSCHeadSize = 12;
const int kScFrames = 3;

#define SC_BIGENDIAN

#pragma pack(push)
#pragma pack(2)
struct SCHeader {
  uint32_t ueid;  // 4 byte ueid

  uint32_t sn_forward;  // sequence number and forward

  uint16_t length;  // packet length, dose not contain header length
  uint16_t rev;     // reversed
};

#pragma pack(pop)

inline uint8_t GetScForward(SCHeader& header) {
  uint8_t* forward_ptr = (uint8_t*)&header.sn_forward;
  return forward_ptr[3];
}

inline void SetScForward(SCHeader& header, uint8_t forward) {
  uint8_t* forward_ptr = (uint8_t*)&header.sn_forward;
  forward_ptr[3] = forward;
}

inline uint32_t GetScSn(SCHeader& header) {
  return header.sn_forward & 0x00ffffff;
}

inline void SetScSn(SCHeader& header, uint32_t sn) {
  header.sn_forward = (header.sn_forward & 0xff000000) | (sn & 0x00ffffff);
}

inline void ScCopyToNet(SCHeader* to, SCHeader* from) {
  to->ueid = htonl(from->ueid);
  to->sn_forward = htonl(from->sn_forward);
  to->rev = htons(from->rev);
  to->length = htons(from->length);
}

inline void ScCopyFromNet(SCHeader* to, SCHeader* from) {
  to->ueid = ntohl(from->ueid);
  to->sn_forward = ntohl(from->sn_forward);
  to->rev = ntohs(from->rev);
  to->length = ntohs(from->length);
}

class SC {
 public:
  int Init(uint32_t ueid, Transforward forward = Transforward::To4G,
           uint32_t sn = 0);
  int ResetRecv();

  /**
   * @Build a SC header for sender
   *
   * @param payload_length
   * @param buff Pointer to packet head
   * @param buff_length Length of sender buff
   * @return int The header length or negative on error
   */
  int Send(uint16_t payload_length, uint8_t* buff, int buff_length);

  /**
   * @brief Unmarshal SC packet
   *
   * @param data input data
   * @param data_length input, length of data
   * @param sn output, sequence number
   * @return int The length of SC header or error when < 0
   */
  int Recv(uint8_t* data, int data_length, uint32_t& sn);

  uint32_t GetUEID() { return recv_header_.ueid; }
  uint32_t GetSn() { return GetScSn(recv_header_); }

 private:
  bool CheckHeadSize();
  SCHeader send_header_, recv_header_;
  bool first_pack_;
};
#endif  //__SC_H__