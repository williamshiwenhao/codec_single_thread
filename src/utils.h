/*
@ Create on JUI 16 2019
@ Author SWH
*/
#ifndef __UTILS_H__
#define __UTILS_H__

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <map>

#include "codec.h"
#include "json.h"

enum class Transforward : uint8_t { To4G = 0, From4G = 1 };

enum class CodecType : uint8_t {
  AMR_WB = 1,
  Codec2 = 2,
  PcmA = 3,
  PcmU = 4,
  Err = 255
};

int ReadConfig(Json::Value& config, const char* file_path);
MediaParam GenerateDefaultParam(CodecType type);

inline CodecType StringToType(const std::string& str) {
  if (str == "amr" || str == "AMR") return (CodecType::AMR_WB);
  if (str == "codec2" || str == "Codec2") return (CodecType::Codec2);
  if (str == "PCMA" || str == "PcmA") return (CodecType::PcmA);
  if (str == "PCMU" || str == "PcmU") return (CodecType::PcmU);
  return CodecType::Err;
}

inline Transforward StringToForward(const std::string& str) {
  if (str == "To4G" || str == "TO4G") return Transforward::To4G;
  if (str == "From4G" || str == "FROM4G") return Transforward::From4G;
  return Transforward::To4G;
}

class PacketSorter {
 public:
  /**
   * @brief Init or reset the socter
   *
   * @param increase sequence number increase by increase every packet
   */
  void Init(uint32_t increase);
  /**
   * @brief Insert a packet into socter
   *
   * @param sn sequence number
   * @param data pointer to data, socter would not copy it
   * @return int 0 insert normal, 1 it is the next packet, -1, sn behind and
   * abort it Remember, it will auto increase one if get a packet
   */
  int Insert(uint32_t sn, uint8_t* data);

  /**
   * @brief Get a packet if possible
   *
   * @return uint8_t* Data or null if no packet aviliable
   */
  uint8_t* Get();

  /**
   * @brief Skip one packet
   *
   */
  void Skip(uint32_t num = 1);

  enum PacketStatus : int {
    kPacketNow = 0,
    kPacketNext = 1,
    kPacketBehind = -1
  };

 private:
  static bool Less(const uint32_t& lh, const uint32_t& rh) {
    return int32_t(lh) - int32_t(rh) < 0;
  }
  struct CmpLess {
    bool operator()(const uint32_t& lh, const uint32_t& rh) {
      return Less(lh, rh);
    }
  };
  bool is_first_ = true;
  uint32_t increase_;
  uint32_t now_sn_;

  std::map<uint32_t, uint8_t*, CmpLess> data_;
};

#endif