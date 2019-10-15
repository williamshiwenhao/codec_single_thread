#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "codec.h"
#include "json.h"
#include "session.h"
#include "udp.h"
#include "utils.h"

const char* kConfigFile = "config.json";
const int kScSamplePrePacket = 480;

void UpLoop(SessionParam param) {
  UploadSession session;
  if (session.Init(param)) {
    printf("[Error] Cannot init upload session");
    return;
  }
  session.ProcessLoop();
}
void DownLoop(SessionParam param) {
  DownloadSession session;
  if (session.Init(param)) {
    printf("[Error] Cannot init upload session");
    return;
  }
  session.ProcessLoop();
}

int main() {
  /************************************
   * Get config
   ************************************/
  Json::Value root;
  ReadConfig(root, kConfigFile);
  root = root["session"];
  if (!root.isArray()) {
    fprintf(stderr, "[Error] Config file error, shoule be array");
    return -1;
  }
  std::vector<std::thread> th;
  for (int i = 0; i < root.size(); ++i) {
    Json::Value config = root[i];
    SessionParam param;
    std::string src_codec = config["src_codec"].asString();
    std::string dst_codec = config["dst_codec"].asString();
    std::string str_forward = config["forward"].asString();
    unsigned recv_port = config["recv_port"].asUInt();
    std::string to_ip = config["ip"].asString();
    unsigned to_port = config["send_port"].asUInt();
    bool if_add_udpip = config["add_udpip"].asBool();
    bool if_parse_udpip = config["parse_udpip"].asBool();
    std::string udpip_sip, udpip_dip;
    uint16_t udpip_sport, udpip_dport;
    if (if_add_udpip) {
      udpip_sip = config["udpip_sip"].asString();
      udpip_dip = config["udpip_dip"].asString();
      udpip_sport = config["udpip_sport"].asUInt();
      udpip_dport = config["udpip_dport"].asUInt();
      if (udpip_sip.size() > 16 || udpip_dip.size() > 16) {
        fprintf(stderr, "[Error] Invalid ip adress\n");
        continue;
      }
    }
    printf("-----------------------------\n");
    printf("[Session]\n");
    printf("-----------------------------\n");
    printf("[Src codec] %s\n", src_codec.c_str());
    printf("[Dst codec] %s\n", dst_codec.c_str());
    printf("[Forward] %s\n", str_forward.c_str());
    printf("[recv_port] %u\n", recv_port);
    printf("[Send to IP] %s\n", to_ip.c_str());
    printf("[Send to port] %u\n", to_port);
    printf("[Send UDP/IP] %s\n", if_add_udpip ? "True" : "False");
    printf("[Parse UDP/IP] %s \n", if_parse_udpip ? "True" : "False");
    if (if_add_udpip) {
      printf("[UDPIP SIP] %s\n", udpip_sip.c_str());
      printf("[UDPIP DIP] %s\n", udpip_dip.c_str());
      printf("[UDPIP SPORT] %u\n", udpip_sport);
      printf("[UDPIP DPORT] %u\n", udpip_dport);
    }
    printf("-----------------------------\n\n");
    param.encoder_type = StringToType(dst_codec);
    param.decoder_type = StringToType(src_codec);
    param.recv_port = recv_port;
    if (param.encoder_type == CodecType::Err ||
        param.decoder_type == CodecType::Err) {
      fprintf(stderr, "[Error] Config error, codec type error\n");
      return -1;
    }
    strcpy(param.ip, to_ip.c_str());
    param.remote_port = (uint16_t)to_port;
    param.ueid = 0x20;
    param.if_add_udpip = if_add_udpip;
    param.if_send_sc = if_parse_udpip;
    if (if_add_udpip) {
      strcpy(param.udpip_sip, udpip_sip.c_str());
      strcpy(param.udpip_dip, udpip_dip.c_str());
      param.udpip_sport = udpip_sport;
      param.udpip_dport = udpip_dport;
    }
    if (StringToForward(str_forward) == Transforward::To4G) {
      th.emplace_back(UpLoop, param);
    } else {
      th.emplace_back(DownLoop, param);
    }
  }
  for (auto& t : th) t.join();

  /************************************
   * Create work thread
   ************************************/
  return 0;
}