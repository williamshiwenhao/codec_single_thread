#include "utils.h"
#include "codec.h"

#include <cstdint>
#include <fstream>

int ReadConfig(Json::Value& config, const char* file_path) {
  std::ifstream fd;
  fd.open(file_path);
  if (!fd) {
    fprintf(stderr, "[Error] Cannot open config file\n");
    return -1;
  }
  try {
    fd >> config;
  } catch (...) {
    fprintf(stderr, "[Error] Cannot parse config file\n");
    fd.close();
    return -1;
  }
  fd.close();
  return 0;
}

MediaParam GenerateDefaultParam(CodecType type) {
  static const MediaParam amr_wb{99, 16000, 320};
  static const MediaParam codec2{96, 8000, 160};
  static const MediaParam pcm_u{0, 8000, 160};
  static const MediaParam pcm_a{8, 8000, 160};
  switch (type) {
    case CodecType::AMR_WB:
      return amr_wb;
    case CodecType::Codec2:
      return codec2;
    case CodecType::PcmA:
      return pcm_a;
    case CodecType::PcmU:
      return pcm_u;
  }
  static const MediaParam empty{0, 0, 0};
  return empty;
}

void PacketSorter::Init(uint32_t increase) {
  is_first_ = true;
  increase_ = increase;
  now_sn_ = 0;
  for (auto it : data_) {
    delete it.second;
  }
  data_.clear();
}

int PacketSorter::Insert(uint32_t sn, uint8_t* data) {
  if (now_sn_ == sn) {
    now_sn_ += increase_;
    return kPacketNow;
  }
  if (is_first_) {
    now_sn_ = sn + increase_;
    is_first_ = false;
    return kPacketNow;
  }
  if (Less(sn, now_sn_)) {
    return kPacketBehind;
  }
  if (Less(now_sn_, sn)) {
    data_.emplace(sn, data);
    return kPacketNext;
  }
  return kPacketNow;
}

uint8_t* PacketSorter::Get() {
  if (data_.empty()) return nullptr;
  auto it = data_.begin();
  if (it->first == now_sn_) {
    uint8_t* val = it->second;
    data_.erase(it);
    return val;
  }
  return nullptr;
}

void PacketSorter::Skip(uint32_t num) {
  now_sn_ += num * increase_;
  while (!data_.empty()) {
    auto it = data_.begin();
    if (Less(it->first, now_sn_)) {
      delete it->second;
      data_.erase(it);
    } else {
      break;
    }
  }
}
