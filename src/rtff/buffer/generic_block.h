#ifndef RTFF_BUFFER_GENERIC_BLOCK_H_
#define RTFF_BUFFER_GENERIC_BLOCK_H_

#include <rtff/buffer/block.h>

#include <vector>
#include <Eigen/Core>

namespace rtff {
/**
 * @brief A multichannel data storage class
 */
template <typename T>
class GenericBlock : public Block<T> {
 public:
  using Vector = Eigen::Matrix<T, Eigen::Dynamic, 1>;
  /**
   * @brief Initialize and allocate memory
   * @param frame_count: the number of samples of each channel
   * @param channel_count: the number of channels
   */
  void Init(uint32_t frame_count, uint8_t channel_count) {
    data_.clear();
    for (uint8_t channel_idx = 0; channel_idx < channel_count; channel_idx++) {
      data_.push_back(Vector::Zero(frame_count));
    }
  }

  /**
   * @param channel_idx: the channel index
   * @return a reference to the raw data stored in a vector
   */
  T* channel(uint8_t channel_idx) override { return data_[channel_idx].data(); }
  const T* channel(uint8_t channel_idx) const override {
    return data_[channel_idx].data();
  }

  /**
   * @return the number of channels
   */
  uint8_t channel_count() const override { return data_.size(); }

  /**
   * @return a vector of pointers giving access to raw data
   */
  std::vector<T*> data_ptr() override {
    std::vector<T*> result;
    for (auto& vector : data_) {
      result.push_back(vector.data());
    }
    return result;
  }

  /**
   * @return the number of samples contained in each channel
   */
  uint32_t size() const override { return data_[0].size(); }

 private:
  std::vector<Vector> data_;
};

using RawBlock = GenericBlock<float>;
using TimeFrequencyBlock = GenericBlock<std::complex<float>>;

}  // namespace rtff

#endif  // RTFF_BUFFER_GENERIC_BLOCK_H_
