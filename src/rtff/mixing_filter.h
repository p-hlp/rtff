#ifndef RTFF_MIXING_FILTER_H_
#define RTFF_MIXING_FILTER_H_

#include <complex>
#include <memory>
#include <system_error>
#include <vector>

#include "rtff/buffer/waveform.h"

#include "rtff/fft/window_type.h"

#include "rtff/analyzer.h"
#include "rtff/buffer/ring/overlap_ring_buffer.h"
#include "rtff/buffer/ring/multichannel_overlap_ring_buffer.h"
#include "rtff/buffer/ring/multichannel_ring_buffer.h"

namespace rtff {

/**
 * @brief Created for Upmixing or Downmixing filters, it lets you input a
 * different number of signal than it outputs
 */
template <uint8_t InputCount, uint8_t OutputCount>
class MixingFilter {
public:
  using Input = Waveform[InputCount];
  using Output = Waveform[OutputCount];
  
  using TimeFrequencyInput = std::vector<std::complex<float>*>[InputCount];
  using TimeFrequencyOutput = std::vector<std::complex<float>*>[OutputCount];
  
  MixingFilter() : fft_size_(2048),
  overlap_(2048 * 0.5), window_type_(fft_window::Type::Hamming),
  block_size_(512) {}
  
  virtual ~MixingFilter() {}

  /**
    * @brief Initialize the filter with the default Hamming window
    * @param channel_count: the number of channel of the input signal
    * @param fft_size: the length in samples of the fourier transform window.
    * @param overlap: the number of samples that will be kept between each
    * window.
    * @param err: an error code that gets set if something goes wrong
    */
  void Init(uint8_t channel_count, uint32_t fft_size, uint32_t overlap,
            std::error_code &err) {
    Init(channel_count, fft_size, overlap, fft_window::Type::Hamming, err);
  }

  /**
   * @brief Initialize the filter
   * @param channel_count: the number of channel of the input signal
   * @param fft_size: the length in samples of the fourier transform window.
   * @param overlap: the number of samples that will be kept between each
   * window.
   * @param windows_type: type of analysis and synthesis window for FFT, default
   * to Hamming to ensure backward compatibility
   * @param err: an error code that gets set if something goes wrong
   */
  void Init(uint8_t channel_count, uint32_t fft_size, uint32_t overlap,
            fft_window::Type windows_type, std::error_code &err) {
    fft_size_ = fft_size;
    overlap_ = overlap;
    window_type_ = windows_type;
    Init(channel_count, err);
  }

  /**
   * @brief Initialize the filter with default stft parameters
   * @param channel_count: the number of channel of the input signal
   * @param err: an error code that gets set if something goes wrong
   */
  void Init(uint8_t channel_count, std::error_code &err) {
    channel_count_ = channel_count;
    InitBuffers();
    
    // init single block buffers
    for (auto idx = 0; idx < InputCount; idx++) {
      amplitude_blocks_[idx] = std::make_shared<RawBlock>();
      amplitude_blocks_[idx]->Init(fft_size(), channel_count);
      frequential_blocks_[idx] = std::make_shared<TimeFrequencyBlock>();
      frequential_blocks_[idx]->Init(fft_size() / 2 + 1, channel_count);
      
      tf_input_[idx] = frequential_blocks_[idx]->data_ptr();
    }
    for (auto idx = 0; idx < OutputCount; idx++) {
      output_amplitude_blocks_[idx] = std::make_shared<RawBlock>();
      output_amplitude_blocks_[idx]->Init(hop_size(), channel_count);
      output_frequential_blocks_[idx] = std::make_shared<TimeFrequencyBlock>();
      output_frequential_blocks_[idx]->Init(fft_size() / 2 + 1, channel_count);
      
      tf_output_[idx] = output_frequential_blocks_[idx]->data_ptr();
    }
    
    // And analyzers
    for (auto idx = 0; idx < AnalyzerCount; idx++) {
      analyzers_[idx] = std::make_shared<Analyzer>();
      analyzers_[idx]->Init(fft_size(), overlap(), windows_type(), channel_count, err);
      if (err) {
        return;
      }
    }
    PrepareToPlay();
  }

  /**
   * @brief define the block size
   * @note the block size correspond to the number of frames contained in each
   * AudioBuffer sent to filter using the ProcessBlock function
   * @param value: the block size
   */
  void set_block_size(uint32_t value) {
    block_size_ = value;
    InitBuffers();
    PrepareToPlay();
  }

  /**
   * @brief Write a block of raw audio data to the filter
   * @note the buffer should have the same channel_count as its filter and the
   * frame_number
   * should be equal to the filter block_size
   * @param buffer: the input data
   */
  void Write(const Input input) {
    for (auto idx = 0; idx < InputCount; idx++) {
      const Waveform* waveform = &(input[idx]);
      input_buffers_[idx]->Write(*waveform, waveform->frame_count());
    }

    while(true) {
      // feed input in amplitude block and compute frequential block
      for (auto idx = 0; idx < InputCount; idx++) {
        if (input_buffers_[idx]->Read(amplitude_blocks_[idx].get()) == 0) {
          return;  // no data to read in a buffer means no data at all. We are done here
        }
        analyzers_[idx]->Analyze(*(amplitude_blocks_[idx]), frequential_blocks_[idx].get());
      }
      
      
      // Run the filter
      ProcessTimeFrequencyBlock(tf_input_, fft_size() / 2 + 1,  &tf_output_);
      
      // get output amplitude block from output frequential block and feed it to output
      for (auto idx = 0; idx < OutputCount; idx++) {
        analyzers_[idx]->Synthesize(*(output_frequential_blocks_[idx]), output_amplitude_blocks_[idx].get());
        output_buffers_[idx]->Write(*(output_amplitude_blocks_[idx]), output_amplitude_blocks_[idx]->size());
      }
      
    }
  }

  /**
   * @brief Write a block of raw audio data to the filter
   * @note the buffer should have the same channel_count as its filter and the
   * frame_number
   * should be equal to the filter block_size
   * @param buffer: the input data
   */
  void Read(Output* output) {
    for (auto idx = 0; idx < OutputCount; idx++) {
      Waveform* waveform = output[idx];
      auto frame_count = waveform->frame_count();
      if (output_buffers_[idx]->Read(waveform, frame_count)) {
        continue;
      }

      // if we don't have enough data to be read, just fill with zeros
      for (auto channel_idx = 0; channel_idx < waveform->channel_count();
           channel_idx++) {
        std::fill(waveform->data(channel_idx),
                  waveform->data(channel_idx) + frame_count, 0);
      }
    }
  }

  /**
   * @brief Acccess the number of frame of latency generated by the filter
   * @note Due to fourier transform computation, a filter most usually creates
   * latency. It depends on the block size, overlap and fft size.
   * @return The latency generated by the filter in frames.
   */
  virtual uint32_t FrameLatency() const {
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

  /**
   * @return the fft size in samples
   */
  uint32_t fft_size() const { return fft_size_; }
  /**
   * @return the overlap in samples
   */
  uint32_t overlap() const { return overlap_; }
  /**
   * @return the windows type
   */
  fft_window::Type windows_type() const { return window_type_; }
  /**
   * @return the hop size in sample
   */
  uint32_t hop_size() const { return fft_size_ - overlap_; }
  /**
   * @return the window size in samples
   * @note this value will be the same as the fft size
   */
  uint32_t window_size() const { return analyzers_[0]->window_size(); }
  /**
   * @return the block size
   * @see set_block_size
   */
  uint32_t block_size() const { return block_size_; }
  /**
   * @return the number of channel of the input signal
   */
  uint8_t channel_count() const { return channel_count_; }

protected:
  /**
   * @brief function called at the end of the initialization process.
   * @note Override this to initialize custom member in child classes
   */
  virtual void PrepareToPlay() {}

  /**
   * @brief Process a frequential buffer.
   * @note that function is called by the ProcessBlock function. It shouldn't be
   * called on its own
   * Override this function to design your filter
   */
  virtual void ProcessTimeFrequencyBlock(TimeFrequencyInput input, uint32_t size, TimeFrequencyOutput* output) = 0;

private:
  void InitBuffers() {
    // We must make sure the ring buffer is not smaller than the hop size, because
    // the output amplitude buffer will try to write blocks of hop size into it
    uint32_t arbitrary_buffer_size = block_size() * 8;
    if (arbitrary_buffer_size <= hop_size()) {
      arbitrary_buffer_size = hop_size() * 2;
    }
    
    for (auto in_idx = 0; in_idx < InputCount; in_idx++) {
      input_buffers_[in_idx] = std::make_shared<MultichannelOverlapRingBuffer>(
              fft_size(), hop_size(), channel_count());
      // initialize the intput_buffer_ with hop_size frames of zeros
      if (fft_size() > block_size()) {
        input_buffers_[in_idx]->InitWithZeros(fft_size() - block_size());
      }
    }
    for (auto out_idx = 0; out_idx < OutputCount; out_idx++) {
      output_buffers_[out_idx] = std::make_shared<MultichannelRingBuffer>(
              arbitrary_buffer_size, channel_count());
    }
    
  }

  uint32_t fft_size_;
  uint32_t overlap_;
  fft_window::Type window_type_;
  uint32_t block_size_;
  uint8_t channel_count_;
  
  using InputBufferPtr = std::shared_ptr<MultichannelOverlapRingBuffer>;
  using OutputBufferPtr = std::shared_ptr<MultichannelRingBuffer>;
  
  InputBufferPtr input_buffers_[InputCount];
  OutputBufferPtr output_buffers_[OutputCount];
  
  static const uint8_t AnalyzerCount = std::max(InputCount, OutputCount);
  std::shared_ptr<Analyzer> analyzers_[AnalyzerCount];
  
  std::shared_ptr<RawBlock> amplitude_blocks_[InputCount];
  std::shared_ptr<RawBlock> output_amplitude_blocks_[OutputCount];
  std::shared_ptr<TimeFrequencyBlock> frequential_blocks_[InputCount];
  std::shared_ptr<TimeFrequencyBlock> output_frequential_blocks_[OutputCount];
  
  TimeFrequencyInput tf_input_;
  TimeFrequencyOutput tf_output_;
};

} // namespace rtff

#endif // RTFF_MIXING_FILTER_H_
