// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/patch_reader.h"

#include <algorithm>
#include <utility>

#include "base/numerics/safe_conversions.h"

namespace zucchini {

namespace patch {

bool ParseElementMatch(BufferSource* source, ElementMatch* element_match) {
  PatchElementHeader element_header;
  if (!source->GetValue(&element_header))
    return false;
  element_match->old_element.exe_type =
      static_cast<ExecutableType>(element_header.exe_type);
  element_match->old_element.offset = element_header.old_offset;
  element_match->old_element.length = element_header.old_length;
  element_match->new_element.exe_type =
      static_cast<ExecutableType>(element_header.exe_type);
  element_match->new_element.offset = element_header.new_offset;
  element_match->new_element.length = element_header.new_length;
  return true;
}

bool ParseBuffer(BufferSource* source, BufferSource* buffer) {
  uint64_t size = 0;
  if (!source->GetValue(&size))
    return false;
  if (!source->GetRegion(base::checked_cast<size_t>(size), buffer))
    return false;
  return true;
}

}  // namespace patch

/******** EquivalenceSource ********/

EquivalenceSource::EquivalenceSource() = default;
EquivalenceSource::EquivalenceSource(const EquivalenceSource&) = default;
EquivalenceSource::~EquivalenceSource() = default;

bool EquivalenceSource::Initialize(BufferSource* source) {
  return patch::ParseBuffer(source, &src_skip_) &&
         patch::ParseBuffer(source, &dst_skip_) &&
         patch::ParseBuffer(source, &copy_count_);
}

base::Optional<Equivalence> EquivalenceSource::GetNext() {
  if (src_skip_.empty() || dst_skip_.empty() || copy_count_.empty())
    return base::nullopt;

  static_assert(std::is_same<offset_t, uint32_t>::value,
                "offset_t == uint32_t");
  Equivalence equivalence = {};
  if (!patch::ParseVarUInt<uint32_t>(&copy_count_, &equivalence.length))
    return base::nullopt;

  int32_t src_offset_diff = 0;
  if (!patch::ParseVarInt<int32_t>(&src_skip_, &src_offset_diff))
    return base::nullopt;
  equivalence.src_offset = previous_src_offset_ + src_offset_diff;
  previous_src_offset_ = equivalence.src_offset + equivalence.length;

  uint32_t dst_offset_diff = 0;
  if (!patch::ParseVarUInt<uint32_t>(&dst_skip_, &dst_offset_diff))
    return base::nullopt;
  equivalence.dst_offset = previous_dst_offset_ + dst_offset_diff;
  previous_dst_offset_ = equivalence.dst_offset + equivalence.length;

  return equivalence;
}

/******** ExtraDataSource ********/

ExtraDataSource::ExtraDataSource() = default;
ExtraDataSource::ExtraDataSource(const ExtraDataSource&) = default;
ExtraDataSource::~ExtraDataSource() = default;

bool ExtraDataSource::Initialize(BufferSource* source) {
  return patch::ParseBuffer(source, &extra_data_);
}

base::Optional<ConstBufferView> ExtraDataSource::GetNext(offset_t size) {
  ConstBufferView buffer;
  if (!extra_data_.GetRegion(size, &buffer))
    return base::nullopt;
  return buffer;
}

/******** RawDeltaSource ********/

RawDeltaSource::RawDeltaSource() = default;
RawDeltaSource::RawDeltaSource(const RawDeltaSource&) = default;
RawDeltaSource::~RawDeltaSource() = default;

bool RawDeltaSource::Initialize(BufferSource* source) {
  return patch::ParseBuffer(source, &raw_delta_skip_) &&
         patch::ParseBuffer(source, &raw_delta_diff_);
}

base::Optional<RawDeltaUnit> RawDeltaSource::GetNext() {
  if (raw_delta_skip_.empty() || raw_delta_diff_.empty())
    return base::nullopt;

  static_assert(std::is_same<offset_t, uint32_t>::value,
                "offset_t == uint32_t");
  RawDeltaUnit delta;
  if (!patch::ParseVarUInt<uint32_t>(&raw_delta_skip_, &delta.copy_offset))
    return base::nullopt;
  if (!raw_delta_diff_.GetValue<int8_t>(&delta.diff))
    return base::nullopt;
  return delta;
}

/******** ReferenceDeltaSource ********/

ReferenceDeltaSource::ReferenceDeltaSource() = default;
ReferenceDeltaSource::ReferenceDeltaSource(const ReferenceDeltaSource&) =
    default;
ReferenceDeltaSource::~ReferenceDeltaSource() = default;

bool ReferenceDeltaSource::Initialize(BufferSource* source) {
  return patch::ParseBuffer(source, &reference_delta_);
}

base::Optional<int32_t> ReferenceDeltaSource::GetNext() {
  if (reference_delta_.empty())
    return base::nullopt;
  int32_t delta;
  if (!patch::ParseVarInt<int32_t>(&reference_delta_, &delta))
    return base::nullopt;
  return delta;
}

/******** TargetSource ********/

TargetSource::TargetSource() = default;
TargetSource::TargetSource(const TargetSource&) = default;
TargetSource::~TargetSource() = default;

bool TargetSource::Initialize(BufferSource* source) {
  return patch::ParseBuffer(source, &extra_targets_);
}

base::Optional<offset_t> TargetSource::GetNext() {
  if (extra_targets_.empty())
    return base::nullopt;

  static_assert(std::is_same<offset_t, uint32_t>::value,
                "offset_t == uint32_t");
  offset_t target = 0;
  if (!patch::ParseVarUInt<offset_t>(&extra_targets_, &target))
    return base::nullopt;
  target += previous_target_;
  previous_target_ = target + 1;
  return target;
}

/******** PatchElementReader ********/

PatchElementReader::PatchElementReader() = default;
PatchElementReader::PatchElementReader(const PatchElementReader&) = default;
PatchElementReader::~PatchElementReader() = default;

bool PatchElementReader::Initialize(BufferSource* source) {
  bool ok = patch::ParseElementMatch(source, &element_match_) &&
            equivalences_.Initialize(source) &&
            extra_data_.Initialize(source) && raw_delta_.Initialize(source) &&
            reference_delta_.Initialize(source);
  if (!ok)
    return false;
  uint64_t pool_count = 0;
  if (!source->GetValue(&pool_count))
    return false;
  for (uint32_t i = 0; i < pool_count; ++i) {
    uint8_t pool_tag = 0;
    if (!source->GetValue(&pool_tag))
      return false;

    if (pool_tag >= extra_targets_.size())
      extra_targets_.resize(pool_tag + 1);

    if (!extra_targets_[pool_tag].Initialize(source))
      return false;
  }
  return true;
}

/******** EnsemblePatchReader ********/

base::Optional<EnsemblePatchReader> EnsemblePatchReader::Create(
    ConstBufferView buffer) {
  BufferSource source(buffer);
  EnsemblePatchReader patch;
  if (!patch.Initialize(&source))
    return base::nullopt;
  return patch;
}

EnsemblePatchReader::EnsemblePatchReader() = default;
EnsemblePatchReader::EnsemblePatchReader(const EnsemblePatchReader&) = default;
EnsemblePatchReader::~EnsemblePatchReader() = default;

bool EnsemblePatchReader::Initialize(BufferSource* source) {
  if (!source->GetValue(&header_))
    return false;
  if (header_.magic != PatchHeader::kMagic)
    return false;
  if (!source->GetValue(&patch_type_))
    return false;
  if (patch_type_ != PatchType::kRawPatch &&
      patch_type_ != PatchType::kSinglePatch &&
      patch_type_ != PatchType::kEnsemblePatch)
    return false;

  uint64_t element_count = 0;
  if (!source->GetValue(&element_count))
    return false;
  if (patch_type_ == PatchType::kRawPatch ||
      patch_type_ == PatchType::kSinglePatch) {
    if (element_count != 1)
      return false;  // Only one element expected.
  }
  for (uint32_t i = 0; i < element_count; ++i) {
    PatchElementReader element_patch;
    if (!element_patch.Initialize(source))
      return false;
    elements_.push_back(std::move(element_patch));
  }
  return true;
}

}  // namespace zucchini
