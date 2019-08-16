#include "utils.h"

#include <fstream>

int ReadConfig(Json::Value& config, const char* file_path) {
  std::ifstream fd;
  fd.open(file_path);
  if (!fd) {
    fprintf(stderr, "[Error] Cannot open config file\n");
    return -1;
  }
  try {
    fd >> config;
  } catch (...) {
    fprintf(stderr, "[Error] Cannot parse config file\n");
    fd.close();
    return -1;
  }
  fd.close();
  return 0;
}