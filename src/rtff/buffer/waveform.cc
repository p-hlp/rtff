#include "rtff/buffer/waveform.h"

namespace rtff {

Waveform::Waveform(uint32_t frame_count, uint8_t channel_count) {
  for (auto channel_idx = 0; channel_idx < channel_count; channel_idx++) {
    data_.push_back(std::vector<float>(frame_count));
  }
}

void Waveform::fromInterleaved(const float* data) {
  for (auto channel_idx = 0; channel_idx < channel_count(); channel_idx++) {
    auto& channel = data_[channel_idx];
    for (auto frame_idx = 0; frame_idx < frame_count(); frame_idx++) {
      channel[frame_idx] = data[(frame_idx * channel_count()) + channel_idx];
    }
  }
}
void Waveform::toInterleaved(float* data) const {
  for (auto channel_idx = 0; channel_idx < channel_count(); channel_idx++) {
    auto& channel = data_[channel_idx];
    for (auto frame_idx = 0; frame_idx < frame_count(); frame_idx++) {
      data[(frame_idx * channel_count()) + channel_idx] = channel[frame_idx];
    }
  }
}

float* Waveform::data(uint8_t channel_idx) {
  return data_[channel_idx].data();
}

const float* Waveform::data(uint8_t channel_idx) const {
  return data_[channel_idx].data();
}

uint32_t Waveform::frame_count() const {
  if (data_.size() == 0) {
    return 0;
  }
  return data_[0].size();
}

uint8_t Waveform::channel_count() const { return data_.size(); }

}  // namespace rtff
