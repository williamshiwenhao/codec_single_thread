#include <cstdint>
#include <cstdio>
#include <iostream>

#pragma pack(push)
#pragma pack(2)
struct SC2Header {
  uint8_t forward;           // 0 upward, 1 downward
  uint8_t id[5];             // 5 bytes id
  uint16_t sequence_number;  // increase one every pack
  uint16_t rev;              // reversed
  uint16_t length;           // packet length, dose not contain header length
};
#pragma pack(pop)
int main() {
  printf("SC2Header = %lu\n", sizeof(SC2Header));
  return 0;
}