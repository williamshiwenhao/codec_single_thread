/**
 * @file session.h
 * @author SWH
 * @brief A codec session
 * @version 0.1
 * @date 2019-08-05
 *
 * @copyright Copyright (c) 2019
 *
 */
#ifndef __SESSION_H__
#define __SESSION_H__
#include <cstdint>
#include <vector>

#include "codec.h"
#include "logger.h"
#include "rtp.h"
#include "sc.h"
#include "udp.h"
#include "utils.h"

#define SENT_RTCP

const int kBuffSize = 1600;

struct SessionParam {
  uint16_t sc_port;        // port for 2.4k audio
  uint16_t rtp_port_base;  // port for rtp port, must be even
  uint8_t forward;         // determin the codec changer forward
  char ip[16];             // IP of IMS
  uint16_t remote_port;    // Port to send data
  MediaParam sc_media;
  MediaParam rtp_media;
  int sc_samples_pre_packet;
  uint8_t encoder_type;
  uint8_t decoder_type;
  uint32_t sc_id;
  int send_frame;
};

class Session {
 public:
  Session() = default;                /// Use default construct function
  Session(const Session &) = delete;  /// Disable copy construct function
  Session &operator=(const Session &) = delete;  /// Disable assignment
  int Init(const SessionParam &);                /// Init a session
  ~Session();
  UdpSocket *GetSocket();  /// Get sockets for something like epoll()

  /**
   * @Decode received data to pcm
   *
   * @param input pointer to input data
   * @param input_length input length
   * @param output pointer to output data
   * @param output_length output length
   * @return int number of samples or error
   */
  int Recv(uint8_t *input, int input_length, uint8_t *output,
           int output_length);

  /**
   * @Code pcm audio and send it
   *
   * @param input pcm audio
   * @param input_length length of input in bytes
   * @param output output buff
   * @param output_length output buff length
   * @return int length of packet or error
   */
  int Send(uint8_t *input, int input_length, uint8_t *output,
           int output_length);

  /**
   * Receive packet and handle it
   *
   */
  void RecvLoop(bool &work);

 private:
  SessionParam param_;
  std::vector<uint8_t> buff_in_{std::vector<uint8_t>(kBuffSize)};
  std::vector<uint8_t> buff_out_{std::vector<uint8_t>(kBuffSize)};
  std::vector<uint8_t> codec_cache_{
      std::vector<uint8_t>(kScFrames * kPcmFrameLength * 2)};
  int cached_sample_ = 0;
  UdpSocket sc_socket_, rtp_socket_;
  Coder *decoder_ = nullptr, *encoder_ = nullptr;
  RtpSession rtp_session_;
  SC sc_session_;
  int output_head_length_;
};

#endif  // __SESSION_H__