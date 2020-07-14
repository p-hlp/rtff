#ifndef RTFF_ABSTRACT_FILTER_H_
#define RTFF_ABSTRACT_FILTER_H_

#include "rtff/mixing_filter.h"

namespace rtff {

/**
 * @brief Base class of frequential filters.
 * Feed raw audio data and process them in the time frequency domain
 */
class AbstractFilter : public MixingFilter {
public:
  AbstractFilter();
  virtual ~AbstractFilter();
  
  /**
  * @brief Write a block of raw audio data to the filter
  * @note the buffer should have the same channel_count as its filter and the frame_number
  * should be equal to the filter block_size
  * @param buffer: the input data
  */
  void Write(const Waveform &buffer);
  
  /**
  * @brief Process a buffer
  * @note the buffer should have the same channel_count and its frame_number
  * should be equal to the filter block_size
  * @param buffer: the data
  * @deprecated prefer the Read/Write interface
  */
  [[deprecated]] void ProcessBlock(Waveform *buffer);

protected:
  /**
   * @brief Process a frequential buffer.
   * @note that function is called by the ProcessBlock function. It shouldn't be
   * called on its own
   * Override this function to design your filter
   */
  virtual void ProcessTransformedBlock(Block<std::complex<float>> *) = 0;

private:
  void ProcessTransformedBlock(
      const std::vector<const TransformedBlock *> &input,
      const std::vector<TransformedBlock *> &output) override;
};

} // namespace rtff

#endif // RTFF_ABSTRACT_FILTER_H_
