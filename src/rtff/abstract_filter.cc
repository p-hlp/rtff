#include "rtff/abstract_filter.h"

#include "rtff/mixing_filter.h"

namespace rtff {

class AbstractFilter::Impl : public MixingFilter<1, 1> {
public:
  Impl(AbstractFilter *instance) : instance_(instance) {}
  void
  ProcessTimeFrequencyBlock(MixingFilter<1, 1>::TimeFrequencyInput input,
                            uint32_t size,
                            MixingFilter<1, 1>::TimeFrequencyOutput *output) {

    // Process the block
    instance_->ProcessTransformedBlock(input[0], size);
    // Copy to the output
    // TODO: this breaks the overall performance. The copy is useless in N to N
    // filters
    std::vector<std::complex<float> *> output_data = (*output)[0];
    for (auto channel_idx = 0; channel_idx < channel_count(); channel_idx++) {
      auto out_map =
          Eigen::Map<Eigen::VectorXcf>((*output)[0][channel_idx], size);
      auto in_map = Eigen::Map<Eigen::VectorXcf>(input[0][channel_idx], size);
      out_map = in_map;
    }
  }

protected:
  void PrepareToPlay() override { instance_->PrepareToPlay(); }

private:
  AbstractFilter *instance_;
};

AbstractFilter::AbstractFilter() : impl_(std::make_shared<Impl>(this)) {}

AbstractFilter::~AbstractFilter() {}

void AbstractFilter::Init(uint8_t channel_count, uint32_t fft_size,
                          uint32_t overlap, std::error_code &err) {
  impl_->Init(channel_count, fft_size, overlap, err);
}

void AbstractFilter::Init(uint8_t channel_count, uint32_t fft_size,
                          uint32_t overlap, fft_window::Type windows_type,
                          std::error_code &err) {
  impl_->Init(channel_count, fft_size, overlap, windows_type, err);
}

void AbstractFilter::Init(uint8_t channel_count, std::error_code &err) {
  impl_->Init(channel_count, err);
}

void AbstractFilter::set_block_size(uint32_t value) {
  impl_->set_block_size(value);
}
uint32_t AbstractFilter::block_size() const { return impl_->block_size(); }
uint8_t AbstractFilter::channel_count() const { return impl_->channel_count(); }

uint32_t AbstractFilter::window_size() const { return impl_->window_size(); }
uint32_t AbstractFilter::fft_size() const { return impl_->fft_size(); }
uint32_t AbstractFilter::overlap() const { return impl_->overlap(); }
fft_window::Type AbstractFilter::windows_type() const {
  return impl_->windows_type();
}
uint32_t AbstractFilter::hop_size() const { return impl_->hop_size(); }

uint32_t AbstractFilter::FrameLatency() const { return impl_->FrameLatency(); }

void AbstractFilter::ProcessBlock(Waveform *buffer) {
  Write(*buffer);
  Read(buffer);
}

void AbstractFilter::Write(const Waveform &buffer) { impl_->Write(&buffer); }

void AbstractFilter::Read(Waveform *buffer) {
  impl_->Read(reinterpret_cast<MixingFilter<1, 1>::Output *>(buffer));
}

void AbstractFilter::PrepareToPlay() {}
} // namespace rtff
