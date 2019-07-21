#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "json.h"
#include "udp.h"
#include "utils.h"
const char* kConfigFile = "config.json";

int main() {
  Json::Value config;
  ReadConfig(config, kConfigFile);
  std::cout << config.size() << std::endl;
  printf("Finished\n");
  return 0;
}