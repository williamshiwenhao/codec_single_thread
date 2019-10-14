/*
@ Create on JUI 16 2019
@ Author SWH
*/
#ifndef __UTILS_H__
#define __UTILS_H__

#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include "codec.h"
#include "json.h"

enum class Transforward : uint8_t { To4G = 0, From4G = 1 };

enum class CodecType : uint8_t { AMR_WB = 1, Codec2 = 2, PcmA = 3, PcmU = 4 };

int ReadConfig(Json::Value& config, const char* file_path);
MediaParam GenerateDefaultParam(uint8_t type);

inline CodecType StringToType(const std::string& str) {
  if (str == "amr" || str == "AMR") return (CodecType::AMR_WB);
  if (str == "codec2" || str == "Codec2") return (CodecType::Codec2);
  if (str == "PCMA" || str == "PcmA") return (CodecType::PcmA);
  if (str == "PCMU" || str == "PcmU") return (CodecType::PcmU);
  return 0;
}

inline uint8_t StringToForward(const std::string& str) {
  if (str == "To4G" || str == "TO4G") return (uint8_t)Transforward::To4G;
  if (str == "From4G" || str == "FROM4G") return (uint8_t)Transforward::From4G;
  return 3;
}

#endif