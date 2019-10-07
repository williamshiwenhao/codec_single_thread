#include <arpa/inet.h>
#include <cstring>
#include <limits>
#include <memory>

#include "logger.h"
#include "sc.h"
#include "utils.h"

int SC::Init(uint32_t ueid) {
  if (ResetSend(ueid)) return -1;
  return ResetRecv();
}

int SC::ResetSend(uint32_t ueid) {
  if (!CheckHeadSize()) return -1;
  memset(&send_header_, 0, sizeof(send_header_));
  send_header_.ueid = ueid;
  SetScForward(send_header_, uint8_t(Transforward::From4G));
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
  send_header_.length = payload_length;  // CHECK: 载荷长度应当不包括头长度
  SetScSn(send_header_, GetScSn(send_header_) + 1);
  ScCopyToNet((SCHeader*)buff, &send_header_);
  return sizeof(send_header_);
}

int SC::Recv(uint8_t* data, int data_length, int& lost_pack) {
  if (data_length < (int)sizeof(SCHeader)) {
    PrintLog("[Warning] Not a complete SC header");
    return -1;
  }
  SCHeader now_header;
  ScCopyFromNet(&now_header, (SCHeader*)data);
  lost_pack = 0;

  // Check length
  if (data_length != (int)sizeof(now_header) + (int)now_header.length) {
    PrintLog("[Warning] SC recv: length error");
    printf("length = %d\n", now_header.length);
    return -1;
  }
  // TODO: 注释下面的代码是为了测试方便，可以自环。正式使用时应当去除注释
  if (GetScForward(now_header) != uint8_t(Transforward::To4G)) {
    PrintLog("[Warning] SC recv: wrong forward");
    return -1;
  }
  if (first_pack_) {
    first_pack_ = false;
    recv_header_ = now_header;
    return sizeof(now_header);
  }
  // Check id
  if (recv_header_.ueid != now_header.ueid) {
    PrintLog("[Warning] SC recv: id not match");
    return -1;
  }
  SetScSn(recv_header_, GetScSn(recv_header_) + 1);
  int lost = GetScSn(now_header) - GetScSn(recv_header_);
  if (lost > std::numeric_limits<uint32_t>::max() >> 1) {
    PrintLog("[Notice] Out of order");
    printf("id = %u\n", GetScSn(now_header));
    return -1;
  }
  lost_pack = lost;
  SetScSn(recv_header_, GetScSn(now_header));
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