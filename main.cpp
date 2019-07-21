#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "coder.h"
#include "json.h"
#include "udp.h"
#include "utils.h"
const char* kConfigFile = "config.json";

int main() {
  /************************************
   * Get config
   ************************************/
  Json::Value root;
  ReadConfig(root, kConfigFile);
  uint32_t en_recv_port = root["encode"]["recv_port"].asUInt();
  std::string en_to_ip = root["encode"]["send_ip"].asString();
  uint32_t en_to_port = root["encode"]["send_port"].asUInt();
  uint32_t de_recv_port = root["decode"]["recv_port"].asUInt();
  std::string de_to_ip = root["decode"]["send_ip"].asString();
  uint32_t de_to_port = root["decode"]["send_port"].asUInt();
  printf("----------------------------------------\n");
  printf("|Encoder message\n");
  printf("----------------------------------------\n");
  printf("[Recv port] %u\n", en_recv_port);
  printf("[Send to ip] %s\n", en_to_ip.data());
  printf("[Send to port] %u\n", en_to_port);
  printf("----------------------------------------\n");
  printf("|Decoder message\n");
  printf("----------------------------------------\n");
  printf("[Recv port] %u\n", de_recv_port);
  printf("[Send to ip] %s\n", de_to_ip.data());
  printf("[Send to port] %u\n", de_to_port);
  /************************************
   * Create work thread
   ************************************/
  std::vector<std::thread> thread_vector;
  thread_vector.emplace_back(Coder::DecodeLoop, de_recv_port, de_to_ip.data(),
                             de_to_port);
  thread_vector.emplace_back(Coder::EncodeLoop, en_recv_port, en_to_ip.data(),
                             en_to_port);
  for (auto& th : thread_vector) th.join();
  return 0;
}