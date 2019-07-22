#include "coder.h"
#include <cstdio>
#include "logger.h"

void Coder::EncodeLoop(const uint32_t recv_port, const char* ip,
                       const uint32_t remote_port) {
  Coder* coder = new Coder();
  if (coder->Init(recv_port, ip, remote_port)) {
    PrintLog("Encode loop init failed\n");
    return;
  }
  int send_length = codec2_bits_per_frame(coder->coder_base_) / 8;
  while (true) {
    coder->Recv(coder->pcm_, sizeof(coder->pcm_));
    coder->Encode();
    coder->Send(coder->coded_, send_length);
  }
}

void Coder::DecodeLoop(const uint32_t recv_port, const char* ip,
                       const uint32_t remote_port) {
  Coder* coder = new Coder();
  if (coder->Init(recv_port, ip, remote_port)) {
    PrintLog("Encode loop init failed\n");
    return;
  }
  int send_length = codec2_samples_per_frame(coder->coder_base_) * 2;
  while (true) {
    coder->Recv(coder->coded_, sizeof(coder->coded_));
    coder->Decode();
    coder->Send(coder->pcm_, send_length);
  }
}

void Coder::PrintCoderMsg() {
  CODEC2* test_coder = codec2_create(kCodecMode);
  if (test_coder == nullptr) {
    printf("[Test] Failed\n");
    return;
  }
  int val;
  val = codec2_samples_per_frame(test_coder);
  printf("[Test] Samples pre frame = %d\n", val);
  val = codec2_bits_per_frame(test_coder);
  printf("[Test] Bits pre frame = %d\n", val);

  codec2_destroy(test_coder);
}

int Coder::Init(const uint32_t recv_port, const char* ip,
                const uint32_t remote_port) {
  // Init coder
  if (coder_base_ != nullptr) {
    PrintLog("[Error] Recreating coder");
    return -1;
  }
  coder_base_ = codec2_create(kCodecMode);
  if (coder_base_ == nullptr) {
    PrintLog("[Error] Cannot create a new coder struct");
    return -1;
  }
  // Init
  if (socket_.Init()) {
    PrintLog("[Error] Udp socket error");
    return -1;
  }
  if (socket_.Bind(recv_port)) {
    PrintLog("[Error] Udp bind error");
    return -1;
  }
  socket_.SetSendIp(ip, remote_port);
  return 0;
}

Coder::~Coder() {
  socket_.Close();
  codec2_destroy(coder_base_);
}

void Coder::Encode() {
  codec2_encode(coder_base_, reinterpret_cast<unsigned char*>(coded_),
                reinterpret_cast<short*>(pcm_));
}

void Coder::Decode() {
  codec2_decode(coder_base_, reinterpret_cast<short*>(pcm_),
                reinterpret_cast<unsigned char*>(coded_));
}

int Coder::Recv(unsigned char* buff, const int length) {
  return socket_.RecvFrom((char*)buff, length);
}

int Coder::Send(unsigned char* buff, const int length) {
  return socket_.Send((char*)buff, length);
}
