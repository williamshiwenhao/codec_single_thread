/**
 * @file rtp.h
 * @author SWH
 * @brief A rtp session use pjsip library. The class do not handle socket and
 * SSRC conflict, so sessions cannot share the same port
 * @version 0.1
 * @date 2019-08-05
 *
 * @copyright Copyright (c) 2019
 *
 */
#ifndef __RTP_H__
#define __RTP_H__
#include <pjlib.h>  // pj_rand()
#include <pjmedia/rtcp.h>
#include <pjmedia/rtp.h>
#include <cstdint>

#include "logger.h"
#include "udp.h"
struct RtpHeader {
  uint16_t v : 2;
  uint16_t p : 1;
  uint16_t x : 1;
  uint16_t cc : 4;
  uint16_t m : 1;
  uint16_t pt : 7;
  uint16_t sequence_num;
  uint32_t timestamp;
  uint32_t ssrc;
};

const int kDefaultMast = 0;

struct MediaParam {
  int payload_type;
  uint32_t clock_rate;
  uint32_t samples_pre_frames;
  uint32_t byre_pre_frame;
};

class RtpSession {
 public:
  RtpSession() = default;
  RtpSession(const RtpSession&) = delete;
  RtpSession& operator=(const RtpSession&) = delete;
  ~RtpSession();
  /**
   * Init rtp session
   *
   * @param parm media param
   * @return int 0 if success
   */
  int Init(MediaParam& parm);

  /**
   * Generate rtp header
   *
   * @param payload_len payload length in byte
   * @param buff pointer to buffer
   * @param length the length of buff
   * @param frame_num number of frames
   * @return int header length when return > 0, false when <= 0
   */
  int GenerateRtpHeader(int payload_len, uint8_t* buff, const int length,
                        const int frame_num = 1);

  /**
   * Decode incoming rtp packet into header and payload
   *
   * @param pkt Pointer to packet
   * @param pkt_len Packet length
   * @param payload Pointer to payload
   * @param payload_length Length of payload
   * @return int 0 if success
   */
  int DecodeRtpHeader(uint8_t* pkt, int pkt_len, const void** payload,
                      unsigned* payload_length);

  /**
   * Generate rtcp. Add sender/receive report and source description
   *
   * @param buff
   * @param length
   * @return int Packet length, or -1 if error
   */
  int GenerateRtcp(uint8_t* buff, int length);

  int DecodeRtcp(uint8_t* pkt, int pkt_length);

  /**
   * Generate bye packet, before bye packet, there will be a SR/RR packet
   *
   * @param buff
   * @param length
   * @return int Packet length, or -1 if error
   */
  int GenerateBye(uint8_t* buff, int length);

 private:
  uint32_t SSRCGenerator();
  MediaParam param_;
  pjmedia_rtp_session rtp_session_;
  pjmedia_rtcp_session rtcp_session_;
};

#endif  //__RTP_H__