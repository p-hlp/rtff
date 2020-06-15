#include "rtff/analyzer.h"

#include "rtff/fft/fft.h"

namespace rtff {

void Analyzer::Init(uint32_t fft_size, uint32_t overlap,
                      fft_window::Type windows_type,
                      uint8_t channel_count, std::error_code& err) {
  fft_size_ = fft_size;
  overlap_ = overlap;

  // init the window to an hamming
  analysis_window_ = Window::Make(windows_type, fft_size);
  synthesis_window_ = Window::Make(windows_type, fft_size);
  unwindow_ = Window::MakeInverse(windows_type, windows_type,
                                  fft_size, hop_size());

  // init the fft
  fft_ = Fft::Create(fft_size_, err);
  if (err) {
    return;
  }

  // init inverse transform temp data
  previous_buffer_.resize(channel_count);
  result_buffer_.resize(channel_count);
  post_ifft_buffer_.resize(channel_count);
  for (auto channel_idx = 0; channel_idx < channel_count; channel_idx++) {
    previous_buffer_[channel_idx] =
        Eigen::VectorXf::Zero(window_size() - hop_size());
    post_ifft_buffer_[channel_idx].resize(window_size());
    result_buffer_[channel_idx].resize(window_size());
  }
}

uint32_t Analyzer::overlap() const { return overlap_; }
uint32_t Analyzer::fft_size() const { return fft_size_; }
uint32_t Analyzer::window_size() const { return analysis_window().size(); }
uint32_t Analyzer::hop_size() const { return fft_size_ - overlap_; }
const Eigen::VectorXf& Analyzer::analysis_window() const {
  return analysis_window_;
}
const Eigen::VectorXf& Analyzer::synthesis_window() const {
  return synthesis_window_;
}

void Analyzer::Analyze(RawBlock& amplitude,
                         TimeFrequencyBlock* frequential) {
  for (uint8_t channel_idx = 0; channel_idx < amplitude.channel_count();
       channel_idx++) {
    // apply the analysis window
    amplitude.channel(channel_idx).array() *= analysis_window_.array();
    // compute the fft and store it into the frequential buffer
    fft_->Forward(amplitude.channel(channel_idx).data(),
                  frequential->channel(channel_idx).data());
  }
}

void Analyzer::Synthesize(const TimeFrequencyBlock& frequential,
                            RawBlock* amplitude) {
  for (uint8_t channel_idx = 0; channel_idx < frequential.channel_count();
       channel_idx++) {
    auto& result_ = result_buffer_[channel_idx];
    auto& previous_ = previous_buffer_[channel_idx];
    auto& post_ifft = post_ifft_buffer_[channel_idx];

    // ifft
    fft_->Backward(frequential.channel(channel_idx).data(), post_ifft.data());
    // apply synthesis window and sum to previous data
    // sum with previous data
    memset(result_.data(), 0, result_.size() * sizeof(float));

    result_.head(previous_.size()) = previous_;
    result_.array() +=
        post_ifft.array() * synthesis_window_.array() / unwindow_.array();

    // keep previous buffer for synthesis
    previous_ = result_.tail(previous_.size());

    // unwindow to get the right buffer
    amplitude->channel(channel_idx).noalias() =
        result_.head(hop_size()).transpose();
  }
}

}  // namespace rtff
