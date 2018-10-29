#ifndef RTFF_ABSTRACT_FILTER_H_
#define RTFF_ABSTRACT_FILTER_H_

#include <complex>
#include <memory>
#include <system_error>
#include <vector>

#include "rtff/buffer/audio_buffer.h"

namespace rtff {

class MultichannelOverlapRingBuffer;
class MultichannelRingBuffer;
class FilterImpl;

/**
 * @brief Base class of frequential filters.
 * Feed Time/Amplitude audio data and process them and Time/Frequency
 * @example
 *   class MyFilter: public Filter {
 *    private:
 *     void ProcessFreqBlock(FrequentialBuffer* buffer) override {
 *       IMPLEMENT YOUR FILTER
 *     }
 *   };
 *   auto f = MyFilter();
 *   f.Init(2, err);  // stereo filtering
 *   f.ProcessBlock(my_audio_data);
 */
class AbstractFilter {
 public:
  AbstractFilter();
  virtual ~AbstractFilter();
  /**
   * @brief Initialize the filter
   * @param channel_count: the number of channel of input buffers
   * @param fft_size
   * @param the overlap size in frames used in the fft
   * @param err
   */
  void Init(uint8_t channel_count, uint32_t fft_size, uint32_t overlap,
            std::error_code& err);

  /**
   * @brief Initialize the filter with default stft parameters
   * @param channel_count: the number of channel of input buffers
   * @param err
   */
  void Init(uint8_t channel_count, std::error_code& err);

  /**
   * @brief define the block size
   * block size is the number of frames sent in the input (ProcessBlock
   * function)
   */
  void set_block_size(uint32_t value);

  /**
   * @brief Converts the input AudioBuffer to a Frequential buffer and calls
   * ProcessFreqBlock
   * @param buffer
   */
  virtual void ProcessBlock(AudioBuffer* buffer);

  /**
   * @brief The latency generated by the filter in frames.
   */
  virtual uint32_t FrameLatency() const;

  uint32_t fft_size() const;
  uint32_t overlap() const;
  uint32_t hop_size() const;
  uint32_t window_size() const;
  uint32_t block_size() const;
  uint8_t channel_count() const;

 protected:
  /**
   * @brief function called at the end of the initialization process. Override
   * this to initialize custom member in child classes
   */
  virtual void PrepareToPlay();

  /**
   * @brief Process a frequential buffer.
   * @note that function is called by the ProcessBlock function. It shouldn't be
   * called on its own
   */
  virtual void ProcessTransformedBlock(std::vector<std::complex<float>*> data,
                                       uint32_t size) = 0;

 private:
  void InitBuffers();

  uint32_t fft_size_;
  uint32_t overlap_;
  uint32_t block_size_;
  uint8_t channel_count_;
  std::shared_ptr<MultichannelOverlapRingBuffer> input_buffer_;
  std::shared_ptr<MultichannelRingBuffer> output_buffer_;

  std::shared_ptr<FilterImpl> impl_;

  class Impl;
  std::shared_ptr<Impl> buffers_;
};

}  // namespace rtff

#endif  // RTFF_ABSTRACT_FILTER_H_
