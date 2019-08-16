/**
 * @file session.cpp
 * @author SWH
 * @brief
 * @version 0.1
 * @date 2019-08-06
 *
 * @copyright Copyright (c) 2019
 *
 */
#include "session.h"
#include "utils.h"

int Session::Init(const SessionParam& param) {
  param_ = param;
  // Parameter check
  if (param.forward != Transforward::From4G &&
      param.forward != Transforward::To4G) {
    PrintLog("[Error] Session init error, forward error");
    return -1;
  }
  // Init socket
  if ((param.rtp_port_base & 1) != 0) {
    PrintLog("[Error] Rtp port should be even");
    return -1;
  }
  /// Init socket and bind port
  if (sc_socket_.Init()) {
    PrintLog("[Error] Session init: cannot init sc socket");
    return -1;
  }
  if (rtp_socket_.Init()) {
    PrintLog("[Error] Session init: cannot init rtp socket");
    return -1;
  }
  if (param.forward == Transforward::To4G) {
    if (sc_socket_.Bind(param.sc_port)) {
      PrintLog("[Error] Session init: cannot bind sc socket");
      return -1;
    }
  }
  if (rtp_socket_.Bind(param.rtp_port_base)) {
    PrintLog("[Error] Session init: cannot bind rtp socket");
    return -1;
  }
  if (param.forward == Transforward::To4G) {
    sc_socket_.SetSendIp(param.ip, param.remote_port);
  } else {
    rtp_socket_.SetSendIp(param.ip, param.remote_port);
  }

  // Init rtp session and sc_session
  if (rtp_session_.Init(param.rtp_media)) {
    PrintLog("[Error] Session init error: cannot init rtp session");
    return -1;
  }
  if (sc_session_.Init(param.sc_id)) {
    PrintLog("[Error] Session init error: cannot init sc session");
    return -1;
  }

  // Init coder
  switch (param.encoder_type) {
    case CodecType::AMR_WB:
      encoder_ = new AmrWbEnCoder();
      break;
    case CodecType::Codec2:
      encoder_ = new Codec2EnCoder();
      break;
    default:
      PrintLog("[Error] Session error, unrecognized encoder type");
      return -1;
  }
  switch (param.decoder_type) {
    case CodecType::AMR_WB:
      decoder_ = new AmrWbDeCoder();
      break;
    case CodecType::Codec2:
      decoder_ = new Codec2DeCoder();
      break;
    default:
      PrintLog("[Error] Session error, unrecognized decoder type");
      return -1;
  }
  if (encoder_->Init()) {
    PrintLog("[Error] Session init error, encoder init error");
    return -1;
  }
  if (decoder_->Init()) {
    PrintLog("[Error] Session init error, decoder init error");
    return -1;
  }
  return 0;
}

Session::~Session() {
  if (encoder_) delete encoder_;
  if (decoder_) delete decoder_;
}

UdpSocket* Session::GetSocket() {
  if (param_.forward == Transforward::From4G) {
    return &rtp_socket_;
  }
  return &sc_socket_;
}

int Session::Codec(uint8_t* input, int input_length, uint8_t* output,
                   int output_length) {
  // Decode head
  int lost_samples = 0;
  int recv_frames;
  int status;
  int recv_samples;
  if (param_.forward == Transforward::From4G) {
    status = rtp_session_.DecodeRtpHeader(input, input_length, lost_samples,
                                          recv_frames);
    recv_samples = param_.rtp_media.samples_pre_frames;
  } else {
    int lost_pack = 0;
    status = sc_session_.Recv(input, input_length, lost_pack);
    lost_samples = lost_pack * param_.sc_media.samples_pre_frames *
                   param_.sc_media.frames_pre_packet;
    recv_samples =
        param_.sc_media.samples_pre_frames * param_.sc_media.frames_pre_packet;
    recv_frames = param_.sc_media.frames_pre_packet;
  }
  if (status < 0) {
    PrintLog("[Warning] Session codec: decode head warning");
    return -1;
  }
  input += status;
  input_length -= status;
  // Decode frame
  status =
      decoder_->Codec(input, input_length, buff_in_.data(), buff_in_.size());
  if (status < 0) {
    PrintLog("[Warning] Session codec: decode frame error");
    buff_in_.assign(0, buff_in_.size());
    status = recv_samples;
  }
  // Encode frame
  int codec_len = encoder_->Codec(buff_in_.data(), status, buff_out_.data(),
                                  buff_out_.size());
  if (codec_len < 0) {
    PrintLog("[Warning] Session codec: encode error");
    codec_len =
        encoder_->GetWhite(buff_out_.data(), buff_out_.size(), recv_frames);
    if (codec_len < 0) {
      PrintLog("[Warning] Session codec: try to get white frames but failed");
      return -1;
    }
  }
  // Encode head
  if (param_.forward == Transforward::From4G) {
    status = sc_session_.Send(codec_len, output, output_length);
  } else {
    status = rtp_session_.GenerateRtpHeader(output, output_length, recv_frames);
  }
  if (status < 0) {
    PrintLog("[Warning] Session codec: encode head error");
    return -1;
  }
  if (output_length >= status + codec_len)
    memcpy(output + status, buff_out_.data(), codec_len);
  else {
    PrintLog("[Warning] Session codec: no enough memory");
    return -1;
  }
  return status + codec_len;
}

void Session::RecvLoop(bool& work) {
  std::vector<uint8_t> buff(kBuffSize);
  UdpSocket *recv_socket, *send_socket;
  if (param_.forward == Transforward::From4G) {
    recv_socket = &rtp_socket_;
    send_socket = &sc_socket_;
  } else {
    recv_socket = &sc_socket_;
    send_socket = &rtp_socket_;
  }
  while (work) {
    int len = recv_socket->RecvFrom((char*)buff.data(), buff.size());
    if (len <= 0) {
      PrintLog("[Warning] Recv error");
      continue;
    }
    len = Codec(buff.data(), len, buff.data(), buff.size());
    if (len <= 0) {
      PrintLog("[Warning] Codec error");
      continue;
    }
    send_socket->Send((char*)buff.data(), len);
  }
}
