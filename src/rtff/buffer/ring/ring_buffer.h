#ifndef RTFF_BUFFER_RING_RING_BUFER_H_
#define RTFF_BUFFER_RING_RING_BUFER_H_

#include <cstdint>
#include <vector>

namespace rtff {

template <typename T>
class Block;
class Waveform;

/**
 * @brief RingBuffer represent a circular buffer. It is used to store enough
 * data before starting a process without having to allocate memory dynamically
 * @see https://en.wikipedia.org/wiki/Circular_buffer
 */
class RingBuffer {
 public:
  /**
   * @brief Constructor
   * @param container_size: the maximum number of data a user can write without
   * reading
   */
  RingBuffer(uint32_t container_size);
  /**
   * @brief fill the buffer with count zeros
   * @param count: the number of zeros to add into the buffer
   */
  void InitWithZeros(uint32_t count);
  /**
   * @brief write data to the buffer
   * @param data: pointer to the data
   * @param frame_count: the number of samples available in the data array
   */
  void Write(const float* data, uint32_t frame_count);
  /**
   * @brief read data from the buffer and remove frame_count data
   * @param data: a pre-allocated array of size frame_count
   * @param frame_count: the number of frames to read
   * @return true is read was successful
   */
  bool Read(float* data, uint32_t frame_count);

 private:
  uint32_t write_index_;
  uint32_t read_index_;
  uint32_t available_data_size_;
  std::vector<float> buffer_;
};
}  // namespace rtff

#endif  // RTFF_BUFFER_RING_RING_BUFER_H_
