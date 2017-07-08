// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/patch_sink.h"

#include <algorithm>
#include <utility>

#include "base/logging.h"

namespace zucchini {

bool SerializeInto(ElementMatch element_match, BufferSink* sink) {
  if (element_match.old_element.exe_type != element_match.new_element.exe_type)
    return false;

  ElementPatchHeader element_header;
  element_header.old_offset = element_match.old_element.offset;
  element_header.new_offset = element_match.new_element.offset;
  element_header.old_length = element_match.old_element.length;
  element_header.new_length = element_match.new_element.length;
  element_header.exe_type = element_match.old_element.exe_type;

  return sink->PutValue<ElementPatchHeader>(element_header);
}

size_t SerializedSize(ElementMatch element_match) {
  return sizeof(ElementPatchHeader);
}

bool SerializeInto(const std::vector<uint8_t>& buffer, BufferSink* sink) {
  return sink->PutValue<uint64_t>(buffer.size()) &&
         sink->PutRange(buffer.begin(), buffer.end());
}

size_t SerializedSize(const std::vector<uint8_t>& buffer) {
  return buffer.size() + sizeof(uint64_t);
}

EquivalenceSink::EquivalenceSink() = default;
EquivalenceSink::~EquivalenceSink() = default;
EquivalenceSink::EquivalenceSink(const std::vector<uint8_t>& src_skip,
                                 const std::vector<uint8_t>& dst_skip,
                                 const std::vector<uint8_t>& copy_count)
    : src_skip_(src_skip), dst_skip_(dst_skip), copy_count_(copy_count) {}
EquivalenceSink::EquivalenceSink(EquivalenceSink&&) = default;

void EquivalenceSink::PutNext(const Equivalence& equivalence) {
  DCHECK(equivalence.dst_offset >= dst_offset_);

  EncodeVarInt<int32_t>(equivalence.src_offset - src_offset_,
                        std::back_inserter(src_skip_));

  EncodeVarUInt<uint32_t>(equivalence.dst_offset - dst_offset_,
                          std::back_inserter(dst_skip_));

  EncodeVarUInt<uint32_t>(equivalence.length, std::back_inserter(copy_count_));

  src_offset_ = equivalence.src_offset + equivalence.length;
  dst_offset_ = equivalence.dst_offset + equivalence.length;
}

size_t EquivalenceSink::SerializedSize() const {
  return zucchini::SerializedSize(src_skip_) +
         zucchini::SerializedSize(dst_skip_) +
         zucchini::SerializedSize(copy_count_);
}

bool EquivalenceSink::SerializeInto(BufferSink* buffer) const {
  return zucchini::SerializeInto(src_skip_, buffer) &&
         zucchini::SerializeInto(dst_skip_, buffer) &&
         zucchini::SerializeInto(copy_count_, buffer);
}

ExtraDataSink::ExtraDataSink() = default;
ExtraDataSink::ExtraDataSink(const std::vector<uint8_t>& extra_data)
    : extra_data_(extra_data) {}
ExtraDataSink::ExtraDataSink(ExtraDataSink&&) = default;
ExtraDataSink::~ExtraDataSink() = default;

size_t ExtraDataSink::SerializedSize() const {
  return zucchini::SerializedSize(extra_data_);
}

bool ExtraDataSink::SerializeInto(BufferSink* buffer) const {
  return zucchini::SerializeInto(extra_data_, buffer);
}

void ExtraDataSink::PutNext(ConstBufferView region) {
  std::copy(region.begin(), region.end(), std::back_inserter(extra_data_));
}

RawDeltaSink::RawDeltaSink() = default;
RawDeltaSink::RawDeltaSink(const std::vector<uint8_t>& raw_delta_skip,
                           const std::vector<uint8_t>& raw_delta_diff)
    : raw_delta_skip_(raw_delta_skip), raw_delta_diff_(raw_delta_diff) {}
RawDeltaSink::RawDeltaSink(RawDeltaSink&&) = default;
RawDeltaSink::~RawDeltaSink() = default;

size_t RawDeltaSink::SerializedSize() const {
  return zucchini::SerializedSize(raw_delta_skip_) +
         zucchini::SerializedSize(raw_delta_diff_);
}

bool RawDeltaSink::SerializeInto(BufferSink* buffer) const {
  return zucchini::SerializeInto(raw_delta_skip_, buffer) &&
         zucchini::SerializeInto(raw_delta_diff_, buffer);
}

void RawDeltaSink::PutNext(RawDeltaUnit delta) {
  EncodeVarUInt<uint32_t>(delta.copy_offset,
                          std::back_inserter(raw_delta_skip_));
  raw_delta_diff_.push_back(delta.diff);
}

ReferenceDeltaSink::ReferenceDeltaSink() = default;
ReferenceDeltaSink::ReferenceDeltaSink(
    const std::vector<uint8_t>& reference_delta)
    : reference_delta_(reference_delta) {}
ReferenceDeltaSink::ReferenceDeltaSink(ReferenceDeltaSink&&) = default;
ReferenceDeltaSink::~ReferenceDeltaSink() = default;

size_t ReferenceDeltaSink::SerializedSize() const {
  return zucchini::SerializedSize(reference_delta_);
}

bool ReferenceDeltaSink::SerializeInto(BufferSink* buffer) const {
  return zucchini::SerializeInto(reference_delta_, buffer);
}

void ReferenceDeltaSink::PutNext(int32_t diff) {
  EncodeVarInt<int32_t>(diff, std::back_inserter(reference_delta_));
}

TargetSink::TargetSink() = default;
TargetSink::TargetSink(const std::vector<uint8_t>& extra_targets)
    : extra_targets_(extra_targets) {}
TargetSink::TargetSink(TargetSink&&) = default;
TargetSink::~TargetSink() = default;

size_t TargetSink::SerializedSize() const {
  return zucchini::SerializedSize(extra_targets_);
}

bool TargetSink::SerializeInto(BufferSink* buffer) const {
  return zucchini::SerializeInto(extra_targets_, buffer);
}

void TargetSink::PutNext(uint32_t target) {
  DCHECK(target > previous_target_);

  EncodeVarUInt<uint32_t>(target - previous_target_,
                          std::back_inserter(extra_targets_));

  previous_target_ = target + 1;
}

PatchElementSink::PatchElementSink() = default;
PatchElementSink::PatchElementSink(ElementMatch element_match)
    : element_match_(element_match) {}
PatchElementSink::PatchElementSink(PatchElementSink&&) = default;
PatchElementSink::~PatchElementSink() = default;

size_t PatchElementSink::SerializedSize() const {
  size_t serialized_size =
      zucchini::SerializedSize(element_match_) +
      equivalences_.SerializedSize() + extra_data_.SerializedSize() +
      raw_delta_.SerializedSize() + reference_delta_.SerializedSize();

  serialized_size += sizeof(uint64_t);
  for (const auto& extra_symbols : extra_targets_) {
    serialized_size += extra_symbols.second.SerializedSize() + 1;
  }
  return serialized_size;
}

bool PatchElementSink::SerializeInto(BufferSink* sink) const {
  bool ok = zucchini::SerializeInto(element_match_, sink) &&
            equivalences_.SerializeInto(sink) &&
            extra_data_.SerializeInto(sink) && raw_delta_.SerializeInto(sink) &&
            reference_delta_.SerializeInto(sink);
  if (!ok)
    return false;

  if (!sink->PutValue<uint64_t>(extra_targets_.size()))
    return false;
  for (const auto& extra_target_sink : extra_targets_) {
    if (!sink->PutValue<uint8_t>(extra_target_sink.first.value()))
      return false;
    if (!extra_target_sink.second.SerializeInto(sink))
      return false;
  }
  return true;
}

void PatchElementSink::SetEquivalenceSink(EquivalenceSink&& equivalences) {
  equivalences_ = std::move(equivalences);
}

void PatchElementSink::SetExtraDataSink(ExtraDataSink&& extra_data) {
  extra_data_ = std::move(extra_data);
}

void PatchElementSink::SetRawDeltaSink(RawDeltaSink&& raw_delta) {
  raw_delta_ = std::move(raw_delta);
}

void PatchElementSink::SetReferenceDeltaSink(
    ReferenceDeltaSink reference_delta) {
  reference_delta_ = std::move(reference_delta);
}

void PatchElementSink::SetTargetSink(PoolTag pool_tag,
                                     TargetSink&& extra_targets) {
  DCHECK(pool_tag != kNoPoolTag);
  extra_targets_[pool_tag] = std::move(extra_targets);
}

EnsemblePatchSink::~EnsemblePatchSink() = default;

EnsemblePatchSink::EnsemblePatchSink(const PatchHeader& header)
    : header_(header) {
  DCHECK(header_.magic == PatchHeader::kMagic);
}

EnsemblePatchSink::EnsemblePatchSink(ConstBufferView old_image,
                                     ConstBufferView new_image) {
  header_.magic = PatchHeader::kMagic;
  header_.old_size = static_cast<uint32_t>(old_image.size());
  header_.old_crc = CalculateCrc32(old_image.begin(), old_image.end());
  header_.new_size = static_cast<uint32_t>(new_image.size());
  header_.new_crc = CalculateCrc32(new_image.begin(), new_image.end());
}

void EnsemblePatchSink::AddElement(PatchElementSink&& element_patch) {
  elements_.push_back(std::move(element_patch));
}

size_t EnsemblePatchSink::SerializedSize() const {
  size_t serialized_size = sizeof(uint64_t) + sizeof(PatchHeader);
  for (const auto& element_patch : elements_) {
    serialized_size += element_patch.SerializedSize();
  }
  return serialized_size;
}

bool EnsemblePatchSink::SerializeInto(BufferSink* sink) const {
  if (!sink->PutValue<PatchHeader>(header_))
    return false;
  if (!sink->PutValue<uint64_t>(elements_.size()))
    return false;
  for (const auto& element : elements_) {
    if (!element.SerializeInto(sink))
      return false;
  }
  return true;
}

}  // namespace zucchini
