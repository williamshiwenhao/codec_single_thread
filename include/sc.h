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
const size_t kSCHeadSize = 12;

#define SC_BIGENDIAN

#pragma pack(push)
#pragma pack(2)
struct SCHeader {
  uint8_t forward;           // 0 upward, 1 downward
  uint8_t id[5];             // 5 bytes id
  uint16_t sequence_number;  // increase one every pack
  uint16_t rev;              // reversed
  uint16_t length;           // packet length, dose not contain header length
};
#pragma pack(pop)

class SC {
 public:
  int Init(const uint8_t id[5]);
  int ResetSend(const uint8_t id[5]);
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

  int Recv(uint8_t* data, int data_length, int& lost_pack);

 private:
  bool CheckHeadSize();
  SCHeader send_header_, recv_header_;
  bool first_pack_;
};
#endif  //__SC_H__