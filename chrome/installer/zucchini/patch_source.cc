// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/patch_source.h"

#include <algorithm>
#include <utility>

namespace zucchini {

bool Parse(ElementMatch* element_match, BufferSource* source) {
  ElementPatchHeader element_header;
  if (!source->ReadValue(&element_header))
    return false;
  element_match->old_element.exe_type = element_header.exe_type;
  element_match->old_element.offset = element_header.old_offset;
  element_match->old_element.length = element_header.old_length;
  element_match->new_element.exe_type = element_header.exe_type;
  element_match->new_element.offset = element_header.new_offset;
  element_match->new_element.length = element_header.new_length;
  return true;
}

bool Parse(BufferSource* buffer, BufferSource* source) {
  uint64_t size = 0;
  if (!source->ReadValue(&size))
    return false;
  *buffer = source->GetRegion(size);
  if (buffer->size() != size)
    return false;
  return true;
}

EquivalenceSource::EquivalenceSource() = default;
EquivalenceSource::EquivalenceSource(const EquivalenceSource&) = default;
EquivalenceSource::~EquivalenceSource() = default;

bool EquivalenceSource::Parse(BufferSource* source) {
  return zucchini::Parse(&src_skip_, source) &&
         zucchini::Parse(&dst_skip_, source) &&
         zucchini::Parse(&copy_count_, source);
}

base::Optional<Equivalence> EquivalenceSource::GetNext() {
  if (src_skip_.empty() || dst_skip_.empty() || copy_count_.empty())
    return base::nullopt;

  Equivalence equivalence = {};
  if (!ParseVarUInt<uint32_t>(&equivalence.length, &copy_count_))
    return base::nullopt;

  int32_t src_offset_diff = 0;
  if (!ParseVarInt<int32_t>(&src_offset_diff, &src_skip_))
    return base::nullopt;
  equivalence.src_offset = previous_src_offset_ + src_offset_diff;
  previous_src_offset_ = equivalence.src_offset + equivalence.length;

  uint32_t dst_offset_diff = 0;
  if (!ParseVarUInt<uint32_t>(&dst_offset_diff, &dst_skip_))
    return base::nullopt;
  equivalence.dst_offset = previous_dst_offset_ + dst_offset_diff;
  previous_dst_offset_ = equivalence.dst_offset + equivalence.length;

  return equivalence;
}

bool EquivalenceSource::Done() const {
  return src_skip_.empty() && dst_skip_.empty() && copy_count_.empty();
}

ExtraDataSource::ExtraDataSource() = default;
ExtraDataSource::ExtraDataSource(const ExtraDataSource&) = default;
ExtraDataSource::~ExtraDataSource() = default;

bool ExtraDataSource::Parse(BufferSource* source) {
  return zucchini::Parse(&extra_data_, source);
}

base::Optional<ConstBufferView> ExtraDataSource::GetNext(offset_t size) {
  ConstBufferView buffer = extra_data_.GetRegion(size);
  if (buffer.size() != size)
    return base::nullopt;
  return buffer;
}

bool ExtraDataSource::Done() const {
  return extra_data_.empty();
}

RawDeltaSource::RawDeltaSource() = default;
RawDeltaSource::RawDeltaSource(const RawDeltaSource&) = default;
RawDeltaSource::~RawDeltaSource() = default;

bool RawDeltaSource::Parse(BufferSource* source) {
  return zucchini::Parse(&raw_delta_skip_, source) &&
         zucchini::Parse(&raw_delta_diff_, source);
}

base::Optional<RawDeltaUnit> RawDeltaSource::GetNext() {
  if (raw_delta_skip_.empty() || raw_delta_diff_.empty())
    return base::nullopt;
  RawDeltaUnit delta;
  if (!ParseVarUInt<uint32_t>(&delta.copy_offset, &raw_delta_skip_))
    return base::nullopt;
  if (!raw_delta_diff_.ReadValue<int8_t>(&delta.diff))
    return base::nullopt;
  return delta;
}

bool RawDeltaSource::Done() const {
  return raw_delta_skip_.empty() && raw_delta_diff_.empty();
}

ReferenceDeltaSource::ReferenceDeltaSource() = default;
ReferenceDeltaSource::ReferenceDeltaSource(const ReferenceDeltaSource&) =
    default;
ReferenceDeltaSource::~ReferenceDeltaSource() = default;

bool ReferenceDeltaSource::Parse(BufferSource* source) {
  return zucchini::Parse(&reference_delta_, source);
}

base::Optional<int32_t> ReferenceDeltaSource::GetNext() {
  if (reference_delta_.empty())
    return base::nullopt;
  int32_t delta;
  if (!ParseVarInt<int32_t>(&delta, &reference_delta_))
    return base::nullopt;
  return delta;
}

bool ReferenceDeltaSource::Done() const {
  return reference_delta_.empty();
}

TargetSource::TargetSource() = default;
TargetSource::TargetSource(const TargetSource&) = default;
TargetSource::~TargetSource() = default;

bool TargetSource::Parse(BufferSource* source) {
  return zucchini::Parse(&extra_targets_, source);
}

base::Optional<offset_t> TargetSource::GetNext() {
  if (extra_targets_.empty())
    return base::nullopt;
  offset_t target = 0;
  if (!ParseVarUInt<offset_t>(&target, &extra_targets_))
    return base::nullopt;
  target += previous_target_;
  previous_target_ = target + 1;
  return target;
}

bool TargetSource::Done() const {
  return extra_targets_.empty();
}

PatchElementSource::PatchElementSource() = default;
PatchElementSource::PatchElementSource(const PatchElementSource&) = default;
PatchElementSource::~PatchElementSource() = default;

bool PatchElementSource::Parse(BufferSource* source) {
  bool ok = zucchini::Parse(&element_match_, source) &&
            equivalences_.Parse(source) && extra_data_.Parse(source) &&
            raw_delta_.Parse(source) && reference_delta_.Parse(source);
  if (!ok)
    return false;
  uint64_t pool_count = 0;
  if (!source->ReadValue(&pool_count))
    return false;
  for (uint32_t i = 0; i < pool_count; ++i) {
    uint8_t pool_tag = 0;
    if (!source->ReadValue(&pool_tag))
      return false;
    if (pool_tag >= extra_targets_.size()) {
      extra_targets_.resize(pool_tag + 1);
    }
    TargetSource extra_target;
    if (!extra_target.Parse(source))
      return false;
    extra_targets_[pool_tag] = std::move(extra_target);
  }
  return true;
}

base::Optional<EnsemblePatchSource> EnsemblePatchSource::Create(
    ConstBufferView buffer) {
  BufferSource source(buffer);
  EnsemblePatchSource patch;
  if (!patch.Parse(&source)) {
    return base::nullopt;
  }
  return patch;
}

EnsemblePatchSource::EnsemblePatchSource() = default;
EnsemblePatchSource::EnsemblePatchSource(const EnsemblePatchSource&) = default;
EnsemblePatchSource::~EnsemblePatchSource() = default;

bool EnsemblePatchSource::Parse(BufferSource* source) {
  if (!source->ReadValue(&header_))
    return false;
  if (header_.magic != PatchHeader::kMagic)
   return false;

  uint64_t element_count = 0;
  if (!source->ReadValue(&element_count))
    return false;
  for (uint32_t i = 0; i != element_count; ++i) {
    PatchElementSource element_patch;
    if (!element_patch.Parse(source))
      return false;
    elements_.push_back(std::move(element_patch));
  }
  return true;
}

}  // namespace zucchini
