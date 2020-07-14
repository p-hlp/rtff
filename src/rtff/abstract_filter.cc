#include "rtff/abstract_filter.h"

namespace rtff {

AbstractFilter::AbstractFilter() : MixingFilter(1, 1) {}

AbstractFilter::~AbstractFilter() {}

void AbstractFilter::Write(const Waveform &buffer) {
  MixingFilter::Write(&buffer);
}

void AbstractFilter::ProcessBlock(Waveform *buffer) {
  Write(*buffer);
  Read(buffer);
}

void AbstractFilter::ProcessTransformedBlock(
    const std::vector<const TransformedBlock *> &inputs,
    const std::vector<TransformedBlock *> &outputs) {
  auto input = inputs[0];
  auto output = outputs[0];

  // TODO: this breaks the overall performance. Copy is useless in N to N
  // filters
  for (auto channel = 0; channel < input->channel_count(); channel++) {
    memcpy(output->channel(channel), input->channel(channel),
           input->size() * sizeof(std::complex<float>));
  }

  ProcessTransformedBlock(output);
}

} // namespace rtff
