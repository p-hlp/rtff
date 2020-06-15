#ifndef RTFF_BUFFER_RING_MULTICHANNEL_OVERLAP_RING_BUFER_H_
#define RTFF_BUFFER_RING_MULTICHANNEL_OVERLAP_RING_BUFER_H_

#include <cstdint>
#include <vector>

namespace rtff {

template <typename T>
class Block;
class Waveform;
class OverlapRingBuffer;

/**
 * @brief A multichannel wrapper around the OverlapRingBuffer
 * @see OverlapRingBuffer
 */
class MultichannelOverlapRingBuffer {
 public:
  /**
   * @brief Constructor
   * @param read_size: the number of frames read when calling the Read function
   * @param step_size: the number of frames to remove from the buffer after a
   * call to the Read function
   * @param channel_count: the number of channels of the original signal
   */
  MultichannelOverlapRingBuffer(uint32_t read_size, uint32_t step_size,
                                uint8_t channel_count);

  /**
   * @brief fill the buffer with count zeros
   * @param frame_number: the number of zeros to add into the buffer
   */
  void InitWithZeros(uint32_t frame_number);

  /**
   * @return the OverlapRingBuffer at a given channel
   * @param channel_idx: the index of the channel to access
   */
  OverlapRingBuffer& operator[](uint8_t channel_idx);

  /**
   * @return the OverlapRingBuffer at a given channel
   * @param channel_idx: the index of the channel to access
   */
  const OverlapRingBuffer& operator[](uint8_t channel_idx) const;

  /**
   * @brief write data to the buffer
   * @param buffer: the AudioBuffer to write
   * @param frame_count: the number of samples available in the buffer
   */
  void Write(const Waveform& buffer, uint32_t frame_count);

  /**
   * @brief read data from the buffer and remove step_size data
   * @param buffer: a pre-allocated AudioBuffer of size read_size
   * @return true is read was successful
   */
  bool Read(Waveform* buffer);

  /**
   * @brief write data to the buffer
   * @param buffer: the Buffer<float> to write
   * @param frame_count: the number of samples available in the buffer
   */
  void Write(const Block<float>& buffer, uint32_t frame_count);

  /**
   * @brief read data from the buffer and remove step_size data
   * @param buffer: a pre-allocated Buffer<float> of size read_size
   * @return true is read was successful
   */
  bool Read(Block<float>* buffer);

 private:
  std::vector<OverlapRingBuffer> buffers_;
};

}  // namespace rtff

#endif  // RTFF_BUFFER_RING_MULTICHANNEL_OVERLAP_RING_BUFER_H_
