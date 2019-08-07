/**
 * @file rtp.cpp
 * @author SWH
 * @brief A rtp session using pjsip
 * @version 0.1
 * @date 2019-08-05
 *
 * @copyright Copyright (c) 2019
 *
 */
#include <cstdlib>
#include <ctime>

#include "rtp.h"

int RtpSession::Init(MediaParam& param) {
  param_ = param;
  pj_uint32_t ssrc = RtpSession::SSRCGenerator();
  pj_status_t pj_status;
  pj_status = pjmedia_rtp_session_init(&rtp_session_, param.payload_type, ssrc);
  if (pj_status != PJ_SUCCESS) {
    PrintLog("[Error] Rtp session init error");
    return -1;
  }
  pjmedia_rtcp_init(&rtcp_session_, "rtcp", param.clock_rate,
                    param.samples_pre_frames, ssrc);
}

RtpSession::~RtpSession() {}

int RtpSession::GenerateRtpHeader(int payload_len, uint8_t* buff,
                                  const int length, const int frame_num) {
  pj_status_t status;
  const void* rtp_header;  /// Length of rtp header in byte
  int header_length = 0;
  /// Generate rtp header
  status = pjmedia_rtp_encode_rtp(
      &rtp_session_, param_.payload_type, kDefaultMast, payload_len,
      param_.samples_pre_frames * frame_num, &rtp_header, &header_length);
  if (status != PJ_SUCCESS) {
    PrintLog("[Error] RtpSession error, can not generate rtp header");
    return -1;
  }
  if (header_length > length) {
    PrintLog("[Error] RtpSession error, no enough memory for rtp header");
    return -1;
  } else {
    memcpy(buff, rtp_header, header_length);
  }
  /// Update rtcp
  pjmedia_rtcp_tx_rtp(&rtcp_session_, frame_num * param_.byre_pre_frame);
  return header_length;
}

int RtpSession::DecodeRtpHeader(uint8_t* pkt, int pkt_len, const void** payload,
                                unsigned* payload_length) {
  pj_status_t status;
  const pjmedia_rtp_hdr* rtp_header;
  /// Parse rtp
  status = pjmedia_rtp_decode_rtp(&rtp_session_, pkt, pkt_len, &rtp_header,
                                  (const void**)payload, payload_length);
  if (status != PJ_SUCCESS) {
    PrintLog("[Warning] Rtp decode failed");
    return -1;
  }
  /// Update rtcp session
  pjmedia_rtcp_rx_rtp(&rtcp_session_, pj_ntohs(rtp_header->seq),
                      pj_ntohl(rtp_header->ts), *payload_length);
  return 0;
}

int RtpSession::GenerateRtcp(uint8_t* buff, int length) {
  int idx = 0;
  pj_status_t status;
  /// Generate rtcp sender report(SR) or receive report(RR)
  void* pkt = nullptr;
  int pkt_length = 0;
  pjmedia_rtcp_build_rtcp(&rtcp_session_, &pkt, &pkt_length);
  if (pkt == nullptr || pkt_length == 0) {
    /// Unlikely
    PrintLog("[Warning] Cannot generate rtcp packet");
    return -1;
  }
  if (idx + pkt_length > length) {
    PrintLog("[Error] No enough memory for rtcp packet");
    return -1;
  }
  memcpy(buff, pkt, pkt_length);
  return pkt_length;
}

int RtpSession::GenerateBye(uint8_t* buff, int length) {
  pj_status_t status;
  // Generate ss/sr first
  int idx = GenerateRtcp(buff, length);
  if (idx <= 0) {
    PrintLog("[Warning] Generate bye failed, cannot generate SR/RR");
    return -1;
  }
  int bye_size = length - idx;
  status = pjmedia_rtcp_build_rtcp_bye(&rtcp_session_, buff + idx,
                                       (pj_size_t*)(&bye_size), nullptr);
  if (status != PJ_SUCCESS) {
    PrintLog("[Warning] Cannot generate bye packet");
    return -1;
  }
  return idx + bye_size;
}

int RtpSession::DecodeRtcp(uint8_t* pkt, int pkt_length) {
  pjmedia_rtcp_rx_rtcp(&rtcp_session_, pkt, pkt_length);
}

uint32_t RtpSession::SSRCGenerator() {
  // TODO: rtp ssrc should use md5, now are using random directly
  return pj_rand();
}