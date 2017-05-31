// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "zucchini/sink_patch.h"

#include <utility>

namespace zucchini {

namespace {

size_t LabelsField(size_t pool) {
  return size_t(PatchField::kLabels) + pool;
}

}  // namespace

/******** SinkPatch::CommandReceptor ********/

SinkPatch::CommandReceptor::CommandReceptor(Stream command)
    : command_(command) {}

void SinkPatch::CommandReceptor::operator()(uint32_t v) {
  command_(v);
}

/******** SinkPatch::EquivalenceReceptor ********/

SinkPatch::EquivalenceReceptor::EquivalenceReceptor(Stream src_skip,
                                                    Stream dst_skip,
                                                    Stream copy_count,
                                                    offset_t threshold)
    : src_skip_(src_skip),
      dst_skip_(dst_skip),
      copy_count_(copy_count),
      threshold_(threshold) {}

void SinkPatch::EquivalenceReceptor::operator()(const Equivalence& e) {
  // Apply delta-encoding, which is the inverse of Equivalence::Advance().
  src_skip_(int32_t(e.src - equivalence_src_));
  dst_skip_(uint32_t(e.dst - equivalence_dst_));
  // TODO(huangs): Investigate why |e.length < threshold_| sometimes.
  copy_count_(uint32_t(e.length - threshold_));
  equivalence_src_ = e.src + e.length;
  equivalence_dst_ = e.dst + e.length;
}

/******** SinkPatch::ExtraDataReceptor ********/

SinkPatch::ExtraDataReceptor::ExtraDataReceptor(Stream extra_data)
    : extra_data_(extra_data) {}

void SinkPatch::ExtraDataReceptor::operator()(Region r) {
  extra_data_(r);
}

/******** SinkPatch::RawDeltaReceptor ********/

SinkPatch::RawDeltaReceptor::RawDeltaReceptor(Stream delta_skip,
                                              Stream delta_diff)
    : delta_skip_(delta_skip), delta_diff_(delta_diff) {}

void SinkPatch::RawDeltaReceptor::operator()(RawDeltaUnit d) {
  delta_skip_(uint32_t(d.copy_offset - prev_copy_offset_));
  delta_diff_(uint8_t(d.diff));
  // Add 1 so consecutive bytewise changes would emit 0 when delta encoded. This
  // is helpful for compression.
  prev_copy_offset_ = d.copy_offset + 1;
}

/******** SinkPatch::ReferenceDeltaReceptor ********/

SinkPatch::ReferenceDeltaReceptor::ReferenceDeltaReceptor(
    Stream reference_delta)
    : reference_delta_(reference_delta) {}

void SinkPatch::ReferenceDeltaReceptor::operator()(int32_t diff) {
  reference_delta_(diff);
}

/******** SinkPatch::LabelReceptor ********/

SinkPatch::LabelReceptor::LabelReceptor(Stream labels, size_t count)
    : labels_(labels), remaining_(static_cast<std::ptrdiff_t>(count)) {}

void SinkPatch::LabelReceptor::operator()(uint32_t l) {
  // If too many calls take place, continue to decrement and quietly give up.
  // Errors checking occurs later.
  if (remaining_-- <= 0)
    return;
  labels_(l - current_);
  current_ = l;
}

/******** SinkPatch ********/

SinkPatch::SinkPatch(SinkStreamSet* streams, offset_t equivalence_threshold)
    : streams_(streams),
      equivalence_threshold_(equivalence_threshold),
      command_rec_(streams_->Get(PatchField::kCommand)),
      equivalence_rec_(streams_->Get(PatchField::kSrcSkip),
                       streams_->Get(PatchField::kDstSkip),
                       streams_->Get(PatchField::kCopyCount),
                       equivalence_threshold_),
      extra_data_rec_(streams_->Get(PatchField::kExtraData)),
      raw_delta_rec_(streams_->Get(PatchField::kRawDeltaSkip),
                     streams_->Get(PatchField::kRawDeltaDiff)),
      reference_delta_rec_(streams_->Get(PatchField::kReferenceDelta)) {}

SinkPatch::~SinkPatch() = default;

void SinkPatch::WritePatchType(PatchType patch_type) {
  patch_type_ = patch_type;
  command_rec_(uint32_t(patch_type_));
}

void SinkPatch::WriteNumElements(size_t num_elements) {
  assert(patch_type_ == PATCH_TYPE_ENSEMBLE);
  num_elements_ = num_elements;
  command_rec_(uint32_t(num_elements_));
}

bool SinkPatch::StartElementWrite(const Ensemble::Element& old_element,
                                  const Ensemble::Element& new_element,
                                  uint32_t max_num_pool) {
  if (patch_type_ == PATCH_TYPE_ENSEMBLE) {
    command_rec_(uint32_t(old_element.start_offset()));
    command_rec_(uint32_t(old_element.sub_image.size()));
    command_rec_(uint32_t(new_element.start_offset()));
    command_rec_(uint32_t(new_element.sub_image.size()));
  }
  ExecutableType exe_type = new_element.exe_type;
  assert(old_element.exe_type == exe_type);
  command_rec_(uint32_t(exe_type));
  command_rec_(max_num_pool);
  assert(label_rec_list_.empty());
  label_rec_list_.resize(max_num_pool);
  return true;
}

bool SinkPatch::EndElementWrite() {
  size_t max_num_pool = label_rec_list_.size();
  // For any uninitialized element in |label_rec_list_|, we append 0 to the
  // corresponding stream to maintain consistency. This allows the patch file
  // to be parsed independently (i.e., no reliance on the "old" image).
  for (size_t pool = 0; pool < max_num_pool; ++pool) {
    if (!label_rec_list_[pool].get()) {
      Stream labels = streams_->Get(LabelsField(pool));
      labels(uint32_t(0));
    } else {
      if (label_rec_list_[pool]->GetRemaining() != 0) {
        // Error: Didn't write the same number of Labels as promised!
        return false;
      }
    }
  }
  label_rec_list_ = decltype(label_rec_list_)();  // Clear.
  return true;
}

SinkPatch::LabelReceptor* SinkPatch::LocalLabels(uint8_t pool, size_t count) {
  assert(pool < label_rec_list_.size());
  std::unique_ptr<LabelReceptor>& label_rec = label_rec_list_[pool];
  if (!label_rec) {
    Stream labels = streams_->Get(LabelsField(pool));
    labels(static_cast<uint32_t>(count));
    label_rec.reset(new LabelReceptor(labels, count));
  }
  return label_rec.get();
}

}  // namespace zucchini
