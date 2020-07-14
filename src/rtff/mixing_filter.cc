#include "rtff/mixing_filter.h"

#include "rtff/buffer/generic_block.h"
#include "rtff/analyzer.h"
#include "rtff/buffer/ring/multichannel_ring_buffer.h"
#include "rtff/buffer/ring/overlap_ring_buffer.h"
#include "rtff/buffer/ring/multichannel_overlap_ring_buffer.h"

namespace rtff {

class MixingFilter::Impl {
public:
  std::vector<RawBlock> amplitude_block;
  std::vector<TimeFrequencyBlock> frequential_block;
  std::vector<TimeFrequencyBlock> output_frequential_block;
  std::vector<RawBlock> output_amplitude_block;

  std::vector<const TransformedBlock *> input;
  std::vector<TransformedBlock *> output;
};

MixingFilter::MixingFilter(uint8_t input_count, uint8_t output_count)
    : input_count_(input_count), output_count_(output_count), fft_size_(2048),
      overlap_(2048 * 0.5), window_type_(fft_window::Type::Hamming),
      block_size_(512) {}

MixingFilter::~MixingFilter() {}

void MixingFilter::Init(uint8_t channel_count, uint32_t fft_size,
                        uint32_t overlap, std::error_code &err) {
  Init(channel_count, fft_size, overlap, fft_window::Type::Hamming, err);
}

void MixingFilter::Init(uint8_t channel_count, uint32_t fft_size,
                        uint32_t overlap, fft_window::Type windows_type,
                        std::error_code &err) {
  fft_size_ = fft_size;
  overlap_ = overlap;
  window_type_ = windows_type;
  Init(channel_count, err);
}

void MixingFilter::Init(uint8_t channel_count, std::error_code &err) {
  channel_count_ = channel_count;
  InitBuffers();

  // init single block buffers and analyzers
  impl_ = std::make_shared<Impl>();
  impl_->amplitude_block.resize(input_count_);
  impl_->frequential_block.resize(input_count_);
  impl_->output_frequential_block.resize(output_count_);
  impl_->output_amplitude_block.resize(output_count_);

  for (auto i = 0; i < input_count_; i++) {
    impl_->amplitude_block[i].Init(fft_size(), channel_count);
    impl_->frequential_block[i].Init(fft_size() / 2 + 1, channel_count);
    impl_->input.push_back(&(impl_->frequential_block[i]));
    auto ptr = std::make_shared<Analyzer>();
    ptr->Init(fft_size(), overlap(), windows_type(), channel_count, err);
    analyzers_.push_back(ptr);
  }
  for (auto i = 0; i < output_count_; i++) {
    impl_->output_frequential_block[i].Init(fft_size() / 2 + 1, channel_count);
    impl_->output_amplitude_block[i].Init(hop_size(), channel_count);
    impl_->output.push_back(&(impl_->output_frequential_block[i]));
    auto ptr = std::make_shared<Analyzer>();
    ptr->Init(fft_size(), overlap(), windows_type(), channel_count, err);
    synthesizers_.push_back(ptr);
  }

  PrepareToPlay();
}

void MixingFilter::InitBuffers() {
  input_buffers_.clear();
  output_buffers_.clear();

  for (auto i = 0; i < input_count_; i++) {
    input_buffers_.push_back(std::make_shared<MultichannelOverlapRingBuffer>(
        fft_size(), hop_size(), channel_count()));
    // initialize the intput_buffer_ with hop_size frames of zeros
    if (fft_size() > block_size()) {
      input_buffers_[i]->InitWithZeros(fft_size() - block_size());
    }
  }

  // We must make sure the ring buffer is not smaller than the hop size, because
  // the output amplitude buffer will try to write blocks of hop size into it
  uint32_t arbitrary_buffer_size = block_size() * 8;
  if (arbitrary_buffer_size <= hop_size()) {
    arbitrary_buffer_size = hop_size() * 2;
  }
  for (auto i = 0; i < output_count_; i++) {
    output_buffers_.push_back(std::make_shared<MultichannelRingBuffer>(
        arbitrary_buffer_size, channel_count()));
  }
}

void MixingFilter::set_block_size(uint32_t value) {
  block_size_ = value;
  InitBuffers();
  PrepareToPlay();
}
uint32_t MixingFilter::block_size() const { return block_size_; }
uint8_t MixingFilter::channel_count() const { return channel_count_; }

uint32_t MixingFilter::window_size() const {
  return analyzers_[0]->window_size();
}
uint32_t MixingFilter::fft_size() const { return fft_size_; }
uint32_t MixingFilter::overlap() const { return overlap_; }
fft_window::Type MixingFilter::windows_type() const { return window_type_; }
uint32_t MixingFilter::hop_size() const { return fft_size_ - overlap_; }

uint32_t MixingFilter::FrameLatency() const {
  // latency has three different states:
  if (hop_size() % block_size() == 0) {
    // when hop size can be devided by block size
    return fft_size() - block_size();
  } else if (block_size() < fft_size()) {
    return fft_size();
  } else {
    return block_size();
  }
}

void MixingFilter::PrepareToPlay() {}

void MixingFilter::Write(const Waveform *buffers) {
  for (auto idx = 0; idx < input_count_; idx++) {
    auto frame_count = buffers[idx].frame_count();
    input_buffers_[idx]->Write(buffers[idx], frame_count);
  }

  while (true) {
    // feed input in amplitude block and compute frequential block
    for (auto idx = 0; idx < input_count_; idx++) {
      if (input_buffers_[idx]->Read(&(impl_->amplitude_block[idx])) == 0) {
        return; // no data to read in a buffer means no data at all. We are done
                // here
      }
      analyzers_[idx]->Analyze(impl_->amplitude_block[idx],
                               &(impl_->frequential_block[idx]));
    }

    // Run the filter
    ProcessTransformedBlock(impl_->input, impl_->output);

    // get output amplitude block from output frequential block and feed it to
    // output
    for (auto idx = 0; idx < output_count_; idx++) {
      synthesizers_[idx]->Synthesize(impl_->output_frequential_block[idx],
                                     &(impl_->output_amplitude_block[idx]));
      output_buffers_[idx]->Write(impl_->output_amplitude_block[idx],
                                  impl_->output_amplitude_block[idx].size());
    }
  }
}

void MixingFilter::Read(Waveform *buffers) {
  for (auto idx = 0; idx < output_count_; idx++) {
    auto frame_count = buffers[idx].frame_count();
    if (output_buffers_[idx]->Read(&(buffers[idx]), frame_count)) {
      continue;
    }

    // if we don't have enough data to be read, just fill with zeros
    for (auto channel_idx = 0; channel_idx < buffers[idx].channel_count();
         channel_idx++) {
      std::fill(buffers[idx].data(channel_idx),
                buffers[idx].data(channel_idx) + frame_count, 0);
    }
  }
}

} // namespace rtff
