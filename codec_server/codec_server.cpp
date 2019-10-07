#include <atomic>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "codec.h"
#include "json.h"
#include "logger.h"
#include "rtp.h"
#include "udp.h"
#include "utils.h"

const char* kConfigFile = "config.json";

const int kPacketLength = 960;
const int kFramePrePacket = 3;
std::string test_ip;

void EncodeLoop(uint16_t listen_port, const char* ip,
                const uint16_t remote_port) {
  UdpSocket udp_server;
  if (udp_server.Init()) {
    return;
  }
  if (udp_server.Bind(listen_port)) return;
  udp_server.SetSendIp(ip, remote_port);
  uint8_t buff[1600];
  uint8_t send_buff[1600];
  // Test rtp
  uint8_t rtp_buff[1600];
  RtpSession rtp;
  MediaParam param;
  param.clock_rate = 8000;
  param.payload_type = 0;
  param.samples_pre_frames = 160;
  rtp.Init(param);
  PcmUEnCoder pcm_encoder;
  pcm_encoder.Init();
  UdpSocket test_socket;
  test_socket.Init();
  test_socket.SetSendIp(test_ip.c_str(), remote_port);

  Codec2EnCoder encoder;
  encoder.Init();
  int cnt = 0;
  while (true) {
    int length = udp_server.RecvFrom((char*)buff, sizeof(buff));
    printf("%d\n", cnt++);
    short* audio_data = (short*)buff;
    if (length != 960) {
      fprintf(stderr, "[Error] length error\n");
      continue;
    }
    // for (int i = 0; i < 480; ++i) {
    //   printf("%d\t", audio_data[i]);
    // }
    // printf("\n\n");
    length = encoder.Codec(buff, length, send_buff, sizeof(send_buff));
    if (length != 18) {
      fprintf(stderr, "[Error] Encode error");
    }
    udp_server.Send((const char*)send_buff, length);

    // Send rtp
    int pcm_len = pcm_encoder.Codec(buff, 960, rtp_buff + kRtpHeaderSize,
                                    sizeof(rtp_buff) - kRtpHeaderSize);
    int8_t* pcm_data = (int8_t*)(rtp_buff + kRtpHeaderSize);
    for (int i = 0; i < 480; ++i) {
      printf("%d\t", pcm_data[i]);
    }
    printf("\n\n");
    if (pcm_len != 480) {
      fprintf(stderr, "[Error] PCM encode error\n");
      continue;
    }
    int test_rtp = rtp.GenerateRtpHeader(rtp_buff, kRtpHeaderSize, 3);
    if (test_rtp != kRtpHeaderSize) {
      fprintf(stderr, "[Error] Rtp error\n");
      continue;
    }
    test_socket.Send((const char*)rtp_buff, test_rtp + pcm_len);
  }
  udp_server.Close();
}

void DecodeLoop(uint16_t listen_port, const char* ip,
                const uint16_t remote_port, bool& work) {
  UdpSocket udp_server;
  if (udp_server.Init()) {
    return;
  }
  if (udp_server.Bind(listen_port)) return;
  udp_server.SetSendIp(ip, remote_port);
  uint8_t buff[1600];
  uint8_t send_buff[1600];
  Codec2DeCoder decoder;
  decoder.Init();
  while (work) {
    int length = udp_server.RecvFrom((char*)buff, sizeof(buff));
    if (length != 18) {
      fprintf(stderr, "[Error] length error\n");
      continue;
    }
    length = decoder.Codec(buff, length, send_buff, sizeof(send_buff));
    if (length != 960) {
      fprintf(stderr, "[Error] Encode error");
    }
    udp_server.Send((const char*)send_buff, length);
  }
  udp_server.Close();
}

void HandleConsole(bool& work) {
  std::string str;
  while (std::cin >> str) {
    if (str == std::string{"q"} || str == std::string("quit")) {
      work = false;
      return;
    }
  }
}

int main() {
  /************************************
   * Get config
   ************************************/
  Json::Value root;
  ReadConfig(root, kConfigFile);
  Json::Value encoder_config = root["encoder"];
  Json::Value decoder_config = root["decoder"];
  unsigned encoder_recv_port = encoder_config["recv_port"].asUInt();
  std::string encoder_ip = encoder_config["send_ip"].asString();
  unsigned encoder_send_port = encoder_config["send_port"].asUInt();
  unsigned decoder_recv_port = decoder_config["recv_port"].asUInt();
  std::string decoder_ip = decoder_config["send_ip"].asString();
  unsigned decoder_send_port = decoder_config["send_port"].asUInt();
  test_ip = root["test_ip"].asString();
  printf("----------------------------------------\n");
  printf("|Encoder message\n");
  printf("----------------------------------------\n");
  printf("[Recv port] %u\n", encoder_recv_port);
  printf("[Send to ip] %s\n", encoder_ip.data());
  printf("[Send to port] %u\n", encoder_send_port);
  printf("----------------------------------------\n");
  printf("|Decoder message\n");
  printf("----------------------------------------\n");
  printf("[Recv port] %u\n", decoder_recv_port);
  printf("[Send to ip] %s\n", decoder_ip.data());
  printf("[Send to port] %u\n", decoder_send_port);
  printf("----------------------------------------\n");
  printf("|Test message\n");
  printf("----------------------------------------\n");
  printf("[Test ip] %s\n", test_ip.c_str());
  printf("----------------------------------------\n");
  printf("|Codec2 message\n");
  printf("----------------------------------------\n");
  Codec2PrintCoderMsg();
  /************************************
   * Create work thread
   ************************************/
  std::vector<std::thread> th;
  std::thread encoder_th(EncodeLoop, encoder_recv_port, encoder_ip.c_str(),
                         encoder_send_port);

  encoder_th.join();
  return 0;
}