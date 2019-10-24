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

int UploadSession::Init(SessionParam param) {
  param_ = param;
  first_pack_ = true;
  MediaParam send_media_param = GenerateDefaultParam(param.encoder_type);
  rtp_.Init(send_media_param);
  sc_recv_.Init(param.ueid);
  // Init Socket
  if (recv_socket_.Init() || send_socket_.Init()) {
    PrintLog("[Error] Socket init error");
    return -1;
  }
  if (recv_socket_.Bind(param_.recv_port)) {
    PrintLog("[Error] Socket bind error");
    printf("port = %u\n", param_.recv_port);
    return -1;
  }
  send_socket_.SetSendIp(param.ip, param.remote_port);
  // Init coder
  switch (param.encoder_type) {
    case CodecType::AMR_WB:
      encoder_ = new AmrWbEnCoder();
      break;
    case CodecType::Codec2:
      encoder_ = new Codec2EnCoder();
      break;
    case CodecType::PcmA:
      encoder_ = new PcmAEnCoder();
      break;
    case CodecType::PcmU:
      encoder_ = new PcmUEnCoder();
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
    case CodecType::PcmA:
      decoder_ = new PcmADeCoder();
      break;
    case CodecType::PcmU:
      decoder_ = new PcmUDeCoder();
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

UploadSession::~UploadSession() {
  if (encoder_) delete encoder_;
  if (decoder_) delete decoder_;
}

int UploadSession::Recv(uint8_t* data, int len, uint8_t* output,
                        int output_len) {
  int status;
  int lost_pack = 0;
  status = sc_recv_.Recv(data, len, lost_pack);
  if (first_pack_) {
    first_pack_ = false;
    sc_send_.Init(sc_recv_.GetUEID(), Transforward::To4G, sc_recv_.GetSn());
  }
  int recv_frame = kScFrames;
  int recv_samples = recv_frame * kPcmFrameSample;
  if (status < 0) {
    PrintLog("[Warning] Session codec: decode head warning");
    return -1;
  }
  data += status;
  len -= status;
  // Decode frame
  status = decoder_->Codec(data, len, output, output_len);
  if (status < 0) {
    PrintLog("[Warning] Session codec: decode frame error");
    return -1;
  }
  return status;
}

int UploadSession::Send(uint8_t* input, int input_len, uint8_t* output,
                        int output_len) {
  if (input_len % kPcmFrameLength != 0) {
    PrintLog("[Warning] Upload Send error, wrong input length");
    return -1;
  }
  if (param_.if_send_sc) {
    output += kSCHeadSize;
    output_len -= kSCHeadSize;
  }
  if (param_.if_add_udpip) {
    output += kUdpIpLen;
    output_len -= kUdpIpLen;
  }
  output += kRtpHeaderSize;
  int len = 0;
  output_len -= kRtpHeaderSize;
  int codec_len = encoder_->Codec(input, input_len, output, output_len);
  if (codec_len < 0) {
    PrintLog("[Warning] Session codec: encode error");
    return -1;
  }
  len += codec_len;
  output -= kRtpHeaderSize;
  int rtp_len = rtp_.GenerateRtpHeader(output, kRtpHeaderSize,
                                       input_len / kPcmFrameLength);
  if (rtp_len < 0) {
    PrintLog("[Warning] Session send: rtp error");
    return -1;
  }
  len += rtp_len;
  if (param_.if_add_udpip) {
    output -= kUdpIpLen;
    int udp_len = udp_.GenerateHeader(output, kUdpIpLen);
    if (udp_len < 0) {
      PrintLog("[Warning] Session send: udp error");
      return -1;
    }
    len += udp_len;
  }
  if (param_.if_send_sc) {
    output -= kSCHeadSize;
    int sc_len = sc_send_.Send(len, output, kSCHeadSize);
    if (sc_len < 0) {
      PrintLog("[Warning] Session send: sc error");
      return -1;
    }
    len += sc_len;
  }
  return len;
}

void UploadSession::ProcessLoop(int send_frame) {
  printf("Send frame %d\n", send_frame);
  std::vector<uint8_t> recv_buff(65536), codec_buff(65536), send_buff(65536);
  std::deque<uint8_t> frame_buff(65536);
  frame_buff.clear();
  while (true) {
    int recv_len =
        recv_socket_.RecvFrom((char*)recv_buff.data(), recv_buff.size());
    if (recv_len <= 0) {
      PrintLog("[Warning] Receive error");
      continue;
    }
    int codec_length =
        Recv(recv_buff.data(), recv_len, codec_buff.data(), codec_buff.size());
    if (codec_length < 0) {
      PrintLog("[Warning] Recv error");
      continue;
    }
    if ((codec_length == (send_frame << 1)) && frame_buff.empty()) {
      int send_length = Send(codec_buff.data(), codec_length, send_buff.data(),
                             send_buff.size());
      if (send_length <= 0) {
        PrintLog("[Warning] Send process error");
        continue;
      }
      send_socket_.Send((char*)send_buff.data(), send_length);
    } else {
      frame_buff.insert(frame_buff.end(), codec_buff.begin(),
                        codec_buff.begin() + codec_length);
    }
    while (frame_buff.size() >= (send_frame << 1)) {
      for (int i = 0; i < (send_frame << 1); ++i) {
        codec_buff[i] = frame_buff.front();
        frame_buff.pop_front();
      }
      int send_length = Send(codec_buff.data(), send_frame << 1,
                             send_buff.data(), send_buff.size());
      if (send_length <= 0) {
        PrintLog("[Warning] Send process error");
        continue;
      }
      int send_len = send_socket_.Send((char*)send_buff.data(), send_length);
      if (send_len <= 0) {
        PrintLog("[Warning] Upload send warning");
      }
    }
  }
}

int DownloadSession::Init(const SessionParam param) {
  param_ = param;
  first_pack_ = true;
  // MediaParam send_media_param = GenerateDefaultParam(param.encoder_type);
  // rtp_.Init(send_media_param);
  sc_send_.Init(param.ueid);
  // Init Socket
  if (recv_socket_.Init() || send_socket_.Init()) {
    PrintLog("[Error] Socket init error");
    return -1;
  }
  if (recv_socket_.Bind(param_.recv_port)) {
    PrintLog("[Error] Socket bind error");
    return -1;
  }
  send_socket_.SetSendIp(param.ip, param.remote_port);
  // Init coder
  switch (param.encoder_type) {
    case CodecType::AMR_WB:
      encoder_ = new AmrWbEnCoder();
      break;
    case CodecType::Codec2:
      encoder_ = new Codec2EnCoder();
      break;
    case CodecType::PcmA:
      encoder_ = new PcmAEnCoder();
      break;
    case CodecType::PcmU:
      encoder_ = new PcmUEnCoder();
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
    case CodecType::PcmA:
      decoder_ = new PcmADeCoder();
      break;
    case CodecType::PcmU:
      decoder_ = new PcmUDeCoder();
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

int DownloadSession::Recv(uint8_t* input, int input_length, uint8_t* output,
                          int output_length) {
  if (param_.if_send_sc) {
    int lost_pack;
    int sc_len = sc_recv_.Recv(input, input_length, lost_pack);
    if (sc_len < 0) {
      PrintLog("[Error] Recv sc error\n");
      return -1;
    }
    if (lost_pack) {
      PrintLog("[Warning] Recv sc lost");
    }
    input += kSCHeadSize;
    input_length -= kSCHeadSize;
  }
  if (param_.if_add_udpip) {
    int udpip_len = UdpParser(input, input_length);
    if (udpip_len < 0) {
      PrintLog("[Error] Recv udpip error");
      return -1;
    }
    input += udpip_len;
    input_length -= udpip_len;
  }
  int lost_frame = 0;
  int recv_frame = 0;
  int rtp_len =
      rtp_.DecodeRtpHeader(input, input_length, lost_frame, recv_frame);
  if (rtp_len < 0) {
    PrintLog("[Error] Rtp recv error");
    return -1;
  }
  // FIXME:
  // if (lost_frame) {
  //   PrintLog("[Warning] RTP lost frame");
  // }
  input += rtp_len;
  input_length -= rtp_len;
  int codec_length =
      decoder_->Codec(input, input_length, output, output_length);
  if (codec_length < 0) {
    PrintLog("[Error] Codec error");
    return -1;
  }
  // FIXME: RTP的recvframe 有buf
  // if (codec_length != recv_frame << 1) {
  //   printf("recv %d codec_length %d\n", recv_frame, codec_length);
  //   PrintLog("[Error] Error rtp frame and decode frame");
  //   return -1;
  // }
  return codec_length;
}

int DownloadSession::Send(uint8_t* input, int input_length, uint8_t* output,
                          int output_length) {
  output += kSCHeadSize;
  output_length -= kSCHeadSize;
  if (input_length != kScFrames * kPcmFrameLength) {
    printf("input_length %d\n", input_length);
    PrintLog("[Error]Senssion send, wrong length");
    return -1;
  }
  int codec_len = encoder_->Codec(input, input_length, output, output_length);
  if (codec_len < 0) {
    PrintLog("[Waring] Downl Session error, codec error");
    return -1;
  }
  output -= kSCHeadSize;
  int sc_len = sc_send_.Send(codec_len, output, kSCHeadSize);
  if (sc_len < 0) {
    PrintLog("[Error] DOwnoad session sc send error");
    return -1;
  }
  return sc_len + codec_len;
}

void DownloadSession::ProcessLoop(int send_frame) {
  printf("Send frame %d\n", send_frame);
  std::vector<uint8_t> recv_buff(65536), codec_buff(65536), send_buff(65536);
  std::deque<uint8_t> frame_buff(65536);
  frame_buff.clear();
  while (true) {
    int recv_len =
        recv_socket_.RecvFrom((char*)recv_buff.data(), recv_buff.size());
    if (recv_len <= 0) {
      PrintLog("[Warning] Receive error");
      continue;
    }
    int codec_length =
        Recv(recv_buff.data(), recv_len, codec_buff.data(), codec_buff.size());
    if (codec_length < 0) {
      PrintLog("[Warning] Recv error");
      continue;
    }
    if ((codec_length == (send_frame << 1)) && frame_buff.empty()) {
      int send_length = Send(codec_buff.data(), codec_length, send_buff.data(),
                             send_buff.size());
      if (send_length <= 0) {
        PrintLog("[Warning] Send process error");
        continue;
      }
      send_socket_.Send((char*)send_buff.data(), send_length);
    } else {
      frame_buff.insert(frame_buff.end(), codec_buff.begin(),
                        codec_buff.begin() + codec_length);
    }
    while (frame_buff.size() >= (send_frame << 1)) {
      for (int i = 0; i < (send_frame << 1); ++i) {
        codec_buff[i] = frame_buff.front();
        frame_buff.pop_front();
      }
      int send_length = Send(codec_buff.data(), send_frame << 1,
                             send_buff.data(), send_buff.size());
      if (send_length <= 0) {
        PrintLog("[Warning] Send process error");
        continue;
      }
      int send_len = send_socket_.Send((char*)send_buff.data(), send_length);
      if (send_len <= 0) {
        PrintLog("[Warning] Upload send warning");
      }
    }
  }
}