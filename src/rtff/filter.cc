#include "rtff/filter.h"

namespace rtff {

Filter::Filter()
    : rtff::AbstractFilter(),
      execute([](Block<std::complex<float>>*) {}) {}

Filter::~Filter() {}
  
void Filter::ProcessTransformedBlock(Block<std::complex<float>>* data) {
  execute(data);
}

}  // namespace rtff
