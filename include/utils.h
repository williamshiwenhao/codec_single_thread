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

int ReadConfig(Json::Value& config, const char* file_path);

#endif