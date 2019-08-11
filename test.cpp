#include "test.h"
#include "logger.h"

#include <chrono>
#include <thread>

const int kFrameTime = 20;  /// ms

void RtpSenderLoop(const char* ip, uint16_t remote_port, uint16_t local_port,
                   int packet_num, int frame_pre_packet) {
  UdpSocket udp;
  uint8_t buff[1600];
  memset(buff, 'a', sizeof(buff));
  if (udp.Init()) {
    PrintLog("[Error] Socket error");
    return;
  }
  if (udp.Bind(local_port)) return;
  udp.SetSendIp(ip, remote_port);
  RtpSession rtp;
  MediaParam rtp_param;
  rtp_param.payload_type = 1;
  rtp_param.clock_rate = 8000;
  rtp_param.samples_pre_frames = 160;
  rtp_param.byte_pre_frame = 320;
  int pay_load_len = frame_pre_packet * rtp_param.byte_pre_frame;
  if (rtp.Init(rtp_param)) return;
  for (int i = 0; i < packet_num; ++i) {
    /// Build packet
    int len = rtp.GenerateRtpHeader(buff, sizeof(buff), frame_pre_packet);
    udp.Send((char*)buff, len + pay_load_len);
    std::this_thread::sleep_for(
        std::chrono::milliseconds(kFrameTime * frame_pre_packet));
  }
}