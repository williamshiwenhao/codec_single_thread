#include <arpa/inet.h>
#include <cstring>
#include <limits>
#include <memory>

#include "logger.h"
#include "sc.h"
#include "utils.h"

int SC::Init(const uint8_t id[5]) {
  if (ResetSend(id)) return -1;
  return ResetRecv();
}

int SC::ResetSend(const uint8_t id[5]) {
  if (!CheckHeadSize()) return -1;
  memset(&send_header_, 0, sizeof(send_header_));
  memcpy(&send_header_.id, id, sizeof(send_header_.id));
  send_header_.forward = Transforward::From4G;
  return 0;
}

int SC::ResetRecv() {
  if (!CheckHeadSize()) return -1;
  memset(&recv_header_, 0, sizeof(recv_header_));
  first_pack_ = true;
  return 0;
}

int SC::Send(uint16_t payload_length, uint8_t* buff, int buff_length) {
  if (buff_length < (int)payload_length + (int)sizeof(SCHeader)) {
    PrintLog("[Warning] No enough memory for SC packet");
    return -1;
  }
  send_header_.length = payload_length;
  memcpy(buff, &send_header_, sizeof(send_header_));
#ifdef SC_BIGENDIAN
  SCHeader* now_header = (SCHeader*)buff;
  now_header->sequence_number = htons(now_header->sequence_number);
  now_header->rev = htons(now_header->rev);
  now_header->length = htons(now_header->length);
#endif
  return sizeof(send_header_);
}

int SC::Recv(uint8_t* data, int data_length, int& lost_pack) {
  if (data_length < (int)sizeof(SCHeader)) {
    PrintLog("[Warning] Not a complete SC header");
    return -1;
  }
  SCHeader now_header;
  lost_pack = 0;
  memcpy(&now_header, data, sizeof(now_header));
#ifdef SC_BIGENDIAN
  now_header.sequence_number = ntohs(now_header.sequence_number);
  now_header.rev = ntohs(now_header.rev);
  now_header.length = ntohs(now_header.length);
#endif  // SC_BIGENDIAN
  // Check length
  if (data_length != (int)sizeof(now_header) + (int)now_header.length) {
    PrintLog("[Warning] SC recv: length error");
    return -1;
  }
  if (now_header.forward != Transforward::To4G) {
    PrintLog("[Warning] SC recv: wrong forward");
    return -1;
  }
  if (first_pack_) {
    first_pack_ = false;
    recv_header_ = now_header;
    return sizeof(now_header);
  }
  // Check id
  if (memcmp(now_header.id, recv_header_.id, sizeof(SCHeader::id))) {
    PrintLog("[Warning] SC recv: id not match");
    return -1;
  }
  recv_header_.sequence_number++;
  uint16_t lost = now_header.sequence_number - recv_header_.sequence_number;
  if (lost > std::numeric_limits<uint16_t>::max() >> 1) {
    // Out of order
    return -1;
  }
  lost_pack = lost;
  recv_header_.sequence_number = now_header.sequence_number;
  return sizeof(recv_header_);
}

bool SC::CheckHeadSize() {
  if (sizeof(SCHeader) != kSCHeadSize) {
    PrintLog(
        "[Fatal error] SC header have wrong size, please check compile "
        "parameters");
    return false;
  }
  return true;
}