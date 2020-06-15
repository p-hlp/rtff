#include "rtff/buffer/ring/multichannel_overlap_ring_buffer.h"

#include "rtff/buffer/ring/overlap_ring_buffer.h"
#include "rtff/buffer/waveform.h"
#include "rtff/buffer/block.h"

namespace rtff {

MultichannelOverlapRingBuffer::MultichannelOverlapRingBuffer(
    uint32_t read_size, uint32_t step_size, uint8_t channel_count) {
  for (auto channel_idx = 0; channel_idx < channel_count; channel_idx++) {
    buffers_.push_back(OverlapRingBuffer(read_size, step_size));
  }
}

void MultichannelOverlapRingBuffer::InitWithZeros(uint32_t frame_number) {
  for (auto& buffer : buffers_) {
    buffer.InitWithZeros(frame_number);
  }
}

OverlapRingBuffer& MultichannelOverlapRingBuffer::operator[](
    uint8_t channel_idx) {
  return buffers_[channel_idx];
}
const OverlapRingBuffer& MultichannelOverlapRingBuffer::operator[](
    uint8_t channel_idx) const {
  return buffers_[channel_idx];
}

void MultichannelOverlapRingBuffer::Write(const Waveform& buffer,
                                          uint32_t frame_count) {
  assert(buffer.channel_count() == buffers_.size());
  for (auto channel_idx = 0; channel_idx < buffers_.size(); channel_idx++) {
    buffers_[channel_idx].Write(buffer.data(channel_idx), frame_count);
  }
}
bool MultichannelOverlapRingBuffer::Read(Waveform* buffer) {
  assert(buffer->channel_count() == buffers_.size());
  for (auto channel_idx = 0; channel_idx < buffers_.size(); channel_idx++) {
    if (!buffers_[channel_idx].Read(buffer->data(channel_idx))) {
      return false;
    }
  }
  return true;
}

void MultichannelOverlapRingBuffer::Write(const Block<float>& buffer,
                                          uint32_t frame_count) {
  assert(buffer.channel_count() == buffers_.size());
  for (auto channel_idx = 0; channel_idx < buffers_.size(); channel_idx++) {
    buffers_[channel_idx].Write(buffer.channel(channel_idx).data(),
                                frame_count);
  }
}
bool MultichannelOverlapRingBuffer::Read(Block<float>* buffer) {
  assert(buffer->channel_count() == buffers_.size());
  for (auto channel_idx = 0; channel_idx < buffers_.size(); channel_idx++) {
    if (!buffers_[channel_idx].Read(buffer->channel(channel_idx).data())) {
      return false;
    }
  }
  return true;
}

}  // namespace rtff
