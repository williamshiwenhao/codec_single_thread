/*
@ Create on JUI 16 2019
@ Author SWH
*/
#ifndef __UTILS_H__
#define __UTILS_H__

#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include "json.h"

struct Transforward {
  static const uint8_t To4G = 0;
  static const uint8_t From4G = 1;
};

struct CodecType {
  static const uint8_t AMR_WB = 1;
  static const uint8_t Codec2 = 2;
};

int ReadConfig(Json::Value& config, const char* file_path);

#endif