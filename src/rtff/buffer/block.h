#ifndef RTFF_BUFFER_BLOCK_H_
#define RTFF_BUFFER_BLOCK_H_

#include <vector>

namespace rtff {

template <typename T>
class Block {
public:
  virtual uint32_t size() const = 0;
  virtual uint8_t channel_count() const = 0;
  virtual T* channel(uint8_t index) = 0;
  virtual const T* channel(uint8_t index) const = 0;
  virtual std::vector<T*> data_ptr() = 0;
};

}  // namespace rtff

#endif  // RTFF_BUFFER_BLOCK_H_
