#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "codec.h"
#include "json.h"
#include "test.h"
#include "udp.h"
#include "utils.h"

const char* kConfigFile = "config.json";

int main() {
  /************************************
   * Get config
   ************************************/
  Json::Value root;
  ReadConfig(root, kConfigFile);
  std::string sender_ip = root["sender"]["ip"].asString();
  unsigned sender_local_port = root["sender"]["local_port"].asUInt();
  unsigned sender_remote_port = root["sender"]["remote_port"].asUInt();
  int sender_packet = root["sender"]["packets"].asInt();
  int sender_frame_pre_packet = root["sender"]["frame_pre_packet"].asInt();
  printf("----------------------------------------\n");
  printf("|Sender message\n");
  printf("----------------------------------------\n");
  printf("[Local port] %u\n", sender_local_port);
  printf("[Send to ip] %s\n", sender_ip.data());
  printf("[Send to port] %u\n", sender_remote_port);
  printf("[Packets] %d\n", sender_packet);
  printf("[Frame pre packet] %d\n", sender_frame_pre_packet);
  /************************************
   * Create work thread
   ************************************/
  RtpSenderLoop(sender_ip.c_str(), sender_remote_port, sender_local_port,
                sender_packet, sender_frame_pre_packet);
  return 0;
}