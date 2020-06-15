#ifndef RTFF_BUFFER_RING_MULTICHANNEL_RING_BUFER_H_
#define RTFF_BUFFER_RING_MULTICHANNEL_RING_BUFER_H_

#include <cstdint>
#include <vector>

namespace rtff {

template <typename T>
class Block;
class Waveform;
class RingBuffer;
/**
 * @brief A multichannel wrapper around the RingBuffer
 */
class MultichannelRingBuffer {
 public:
  /**
   * @brief Constructor
   * @param container_size: the maximum number of data a user can write without
   * reading
   * @param channel_count: the number of channel of the original signal
   */
  MultichannelRingBuffer(uint32_t container_size, uint8_t channel_count);

  /**
   * @brief fill the buffer with count zeros
   * @param frame_number: the number of zeros to add into the buffer
   */
  void InitWithZeros(uint32_t frame_number);

  /**
   * @return the RingBuffer at a given channel
   * @param channel_idx: the index of the channel to access
   */
  RingBuffer& operator[](uint8_t channel_idx);
  /**
   * @return the RingBuffer at a given channel
   * @param channel_idx: the index of the channel to access
   */
  const RingBuffer& operator[](uint8_t channel_idx) const;

  /**
   * @brief write data to the buffer
   * @param buffer: the AudioBuffer to write
   * @param frame_count: the number of samples available in the buffer
   */
  void Write(const Waveform& buffer, uint32_t frame_count);
  /**
   * @brief read data from the buffer and remove frame_count data
   * @param buffer: a pre-allocated AudioBuffer of size frame_count
   * @param frame_count: the number of frames to read
   * @return true is read was successful
   */
  bool Read(Waveform* buffer, uint32_t frame_count);
  /**
   * @brief write data to the buffer
   * @param buffer: the Buffer<float> to write
   * @param frame_count: the number of samples available in the buffer
   */
  void Write(const Block<float>& buffer, uint32_t frame_count);
  /**
   * @brief read data from the buffer and remove frame_count data
   * @param buffer: a pre-allocated Buffer<float> of size frame_count
   * @param frame_count: the number of frames to read
   * @return true is read was successful
   */
  bool Read(Block<float>* buffer, uint32_t frame_count);

 private:
  std::vector<RingBuffer> buffers_;
};

}  // namespace rtff

#endif  // RTFF_BUFFER_RING_MULTICHANNEL_RING_BUFER_H_
