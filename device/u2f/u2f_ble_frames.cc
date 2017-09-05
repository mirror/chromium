// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/u2f/u2f_ble_frames.h"

#include "base/logging.h"
#include "base/numerics/safe_conversions.h"

#include <algorithm>

namespace device {

U2fBleFrame::U2fBleFrame(Command command) : command_(command) {}

U2fBleFrame::U2fBleFrame(Command command, std::vector<uint8_t> data)
    : command_(command), data_(std::move(data)) {}

U2fBleFrame::U2fBleFrame(U2fBleFrame&&) = default;
U2fBleFrame& U2fBleFrame::operator=(U2fBleFrame&&) = default;

U2fBleFrame::~U2fBleFrame() = default;

size_t U2fBleFrame::ToFragments(
    size_t max_fragment_size,
    U2fBleFrameInitializationFragment* initial_fragment,
    std::vector<U2fBleFrameContinuationFragment>* other_fragments) const {
  DCHECK(initial_fragment && other_fragments);
  DCHECK_LE(data_.size(), 0xFFFFu);
  DCHECK_GE(max_fragment_size, U2fBleFrameContinuationFragment::kHeaderSize);

  size_t data_fragment_size = std::min(
      max_fragment_size - U2fBleFrameInitializationFragment::kHeaderSize,
      data_.size());
  *initial_fragment = U2fBleFrameInitializationFragment(
      command_, static_cast<uint16_t>(data_.size()), &data_[0],
      data_fragment_size);

  size_t num_continuation_fragments = 0;
  for (size_t pos = data_fragment_size, size = data_.size(); pos < size;
       pos += max_fragment_size - 1, ++num_continuation_fragments) {
    data_fragment_size = std::min(size - pos, max_fragment_size - 1);
    other_fragments->push_back(U2fBleFrameContinuationFragment(
        &data_[pos], data_fragment_size, num_continuation_fragments & 0x7F));
  }

  return num_continuation_fragments;
}

U2fBleFrameInitializationFragment U2fBleFrameInitializationFragment::Parse(
    const uint8_t* data,
    size_t size) {
  DCHECK_GE(size, 3u);
  const auto command = static_cast<U2fBleFrame::Command>(*data);
  const uint16_t data_length = (static_cast<uint16_t>(data[1]) << 8) + data[2];
  return U2fBleFrameInitializationFragment(command, data_length, data + 3,
                                           size - 3);
}

U2fBleFrameInitializationFragment::U2fBleFrameInitializationFragment(
    const U2fBleFrameInitializationFragment&) = default;
U2fBleFrameInitializationFragment& U2fBleFrameInitializationFragment::operator=(
    const U2fBleFrameInitializationFragment&) = default;

std::vector<uint8_t> U2fBleFrameInitializationFragment::Serialize() const {
  std::vector<uint8_t> result;
  result.reserve(size() + 3);
  result.push_back(static_cast<uint8_t>(command_));
  result.push_back((data_length_ >> 8) & 0xFF);
  result.push_back(data_length_ & 0xFF);
  result.insert(result.end(), data(), data() + size());
  return result;
}

U2fBleFrameContinuationFragment U2fBleFrameContinuationFragment::Parse(
    const uint8_t* data,
    size_t size) {
  DCHECK_GE(size, 1u);
  const uint8_t sequence = *data;
  return U2fBleFrameContinuationFragment(data + 1, size - 1, sequence);
}

std::vector<uint8_t> U2fBleFrameContinuationFragment::Serialize() const {
  std::vector<uint8_t> result;
  result.reserve(size() + 1);
  result.push_back(sequence_);
  result.insert(result.end(), data(), data() + size());
  return result;
}

U2fBleFrameAssembler::U2fBleFrameAssembler(
    const U2fBleFrameInitializationFragment& fragment)
    : frame_(fragment.command(), std::vector<uint8_t>()) {
  auto& data = frame_.data();
  data.reserve(fragment.data_length());
  data.insert(data.end(), fragment.data(), fragment.data() + fragment.size());
}

bool U2fBleFrameAssembler::AddFragment(
    const U2fBleFrameContinuationFragment& fragment) {
  if (fragment.sequence() != sequence_number_)
    return false;
  if (++sequence_number_ >= 0x7F)
    sequence_number_ = 0;

  auto& data = frame_.data();
  if (data.size() + fragment.size() > data.capacity())
    return false;
  data.insert(data.end(), fragment.data(), fragment.data() + fragment.size());
  return true;
}

bool U2fBleFrameAssembler::IsDone() const {
  return frame_.data().size() == frame_.data().capacity();
}

U2fBleFrame* U2fBleFrameAssembler::GetFrame() {
  return IsDone() ? &frame_ : nullptr;
}

U2fBleFrameAssembler::~U2fBleFrameAssembler() = default;

}  // namespace device
