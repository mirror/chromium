// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "zucchini/source_patch.h"

#include <iostream>
#include <utility>

namespace zucchini {

/******** SourcePatch::CommandGenerator ********/

SourcePatch::CommandGenerator::CommandGenerator(Stream command)
    : command_(command) {}

SourcePatch::CommandGenerator::CommandGenerator(const CommandGenerator&) =
    default;

bool SourcePatch::CommandGenerator::operator()(uint32_t* v) {
  return command_(v);
}

/******** SourcePatch::EquivalenceGenerator ********/

SourcePatch::EquivalenceGenerator::EquivalenceGenerator(Stream src_skip,
                                                        Stream dst_skip,
                                                        Stream copy_count,
                                                        offset_t threshold,
                                                        Equivalence base_equiv)
    : src_skip_(src_skip),
      dst_skip_(dst_skip),
      copy_count_(copy_count),
      threshold_(threshold),
      cur_equiv_(base_equiv) {}

SourcePatch::EquivalenceGenerator::EquivalenceGenerator(
    const EquivalenceGenerator&) = default;

SourcePatch::EquivalenceGenerator::~EquivalenceGenerator() = default;

bool SourcePatch::EquivalenceGenerator::operator()(Equivalence* e) {
  int32_t diff_src = 0;
  uint32_t diff_dst = 0;
  uint32_t length = 0;
  // TODO(huangs): |length| may be "negative" sometimes; fix?
  if (!src_skip_(&diff_src) || !dst_skip_(&diff_dst) || !copy_count_(&length))
    return false;
  cur_equiv_.Advance(diff_src, diff_dst, length + threshold_);
  *e = cur_equiv_;
  return true;
}

/******** SourcePatch::ReferenceDeltaGenerator ********/

SourcePatch::ReferenceDeltaGenerator::ReferenceDeltaGenerator(
    Stream reference_delta)
    : reference_delta_(reference_delta) {}

SourcePatch::ReferenceDeltaGenerator::~ReferenceDeltaGenerator() = default;

bool SourcePatch::ReferenceDeltaGenerator::operator()(int32_t* r) {
  return reference_delta_(r);
}

/******** SourcePatch::ExtraDataGenerator ********/

SourcePatch::ExtraDataGenerator::ExtraDataGenerator(Stream extra_data)
    : extra_data_(extra_data) {}

SourcePatch::ExtraDataGenerator::~ExtraDataGenerator() = default;

bool SourcePatch::ExtraDataGenerator::operator()(Region r) {
  return extra_data_(r);
}

/******** SourcePatch::RawDeltaGenerator ********/

SourcePatch::RawDeltaGenerator::RawDeltaGenerator(Stream delta_skip,
                                                  Stream delta_diff)
    : delta_skip_(delta_skip), delta_diff_(delta_diff), prev_copy_offset_(0) {}

SourcePatch::RawDeltaGenerator::~RawDeltaGenerator() = default;

bool SourcePatch::RawDeltaGenerator::operator()(RawDeltaUnit* d) {
  uint32_t skip = 0;
  if (!delta_skip_(&skip))
    return false;
  d->copy_offset = prev_copy_offset_ + skip;
  // Add 1 to match RawDeltaReceptor.
  prev_copy_offset_ = d->copy_offset + 1;
  return delta_diff_(&d->diff);
}

/******** SourcePatch::LabelGenerator ********/

SourcePatch::LabelGenerator::LabelGenerator(Stream* labels, size_t count)
    : labels_(labels), remaining_(static_cast<std::ptrdiff_t>(count)) {}

SourcePatch::LabelGenerator::~LabelGenerator() = default;

bool SourcePatch::LabelGenerator::operator()(offset_t* offset) {
  if (!remaining_)  // Out of data. This is not an error.
    return false;
  --remaining_;
  uint32_t tmp = 0;
  if (!(*labels_)(&tmp))
    return false;
  current_ += tmp;
  *offset = current_;
  return true;
}

/******** SourcePatch::LocalEquivalenceScanner ********/

SourcePatch::LocalEquivalenceScanner::LocalEquivalenceScanner(
    offset_t equivalence_threshold,
    Stream src_skip,
    Stream dst_skip,
    Stream copy_count)
    : equivalence_threshold_(equivalence_threshold),
      marching_src_skip_(src_skip),
      marching_dst_skip_(dst_skip),
      marching_copy_count_(copy_count) {
  AdvanceEquivalence();
}

SourcePatch::LocalEquivalenceScanner::LocalEquivalenceScanner(
    const LocalEquivalenceScanner&) = default;

SourcePatch::LocalEquivalenceScanner::~LocalEquivalenceScanner() = default;

bool SourcePatch::LocalEquivalenceScanner::AdvanceEquivalence() {
  cur_equiv_ = next_equiv_;
  cur_src_skip_it_ = marching_src_skip_.current();
  cur_dst_skip_it_ = marching_dst_skip_.current();
  cur_copy_count_it_ = marching_copy_count_.current();
  int32_t diff_src = 0;
  uint32_t diff_dst = 0;
  uint32_t length = 0;
  if (!marching_src_skip_(&diff_src) || !marching_dst_skip_(&diff_dst) ||
      !marching_copy_count_(&length)) {
    return false;  // Reached end of data.
  }
  next_equiv_.Advance(diff_src, diff_dst, length + equivalence_threshold_);
  return true;
}

// |[start_offset, end_offset)| represents the "current element". Comments in
// this function are concerned only with the "dst" part of equivalences.
bool SourcePatch::LocalEquivalenceScanner::Advance(offset_t start_offset,
                                                   offset_t end_offset) {
  if (start_offset < prev_end_offset_ || start_offset >= end_offset)
    return false;  // Invalid offsets or range (including empty).

  prev_end_offset_ = end_offset;
  if (cur_src_skip_it_ == marching_copy_count_.end() ||
      cur_dst_skip_it_ == marching_dst_skip_.end() ||
      cur_copy_count_it_ == marching_copy_count_.end()) {
    // Function should only be called when we know more elements exist.
    return false;
  }

  // Advance |next_equiv_| until its "lo" reaches or exceeds |start_offset|.
  while (next_equiv_.dst < start_offset && AdvanceEquivalence()) {
  }
  // Zucchini-gen should not generate |cur_equiv_| that crosses |start_offset|:
  //   |start_offset|: ..........|..........
  //   Possible:             [cur|next)
  //   Possible:          [cur)  [next)     <= Gap contains extra data.
  //   Possible:             [cur)  [next)
  //   Possible:          [cur)     [next)
  //   Impossible:             [cur|next)
  //   Impossible:             [cur)  [next)
  if (cur_equiv_.dst + cur_equiv_.length > start_offset)
    return false;
  // |cur_equiv_|'s "hi" is at or before |start_offset|. If we use |cur_*_it_|
  // to Advance() it then we'd reach the first equivalence in the current
  // element. So |cur_equiv_| is the |base_equiv|, and |prev*_it_| are start
  // iterators for EquivalenceGenerator of the current element.
  base_equiv_ = cur_equiv_;
  Stream::iterator start_src_skip_it = cur_src_skip_it_;
  Stream::iterator start_dst_skip_it = cur_dst_skip_it_;
  Stream::iterator start_copy_count_it = cur_copy_count_it_;

  // Advance |next_equiv_| until its "lo" reaches or exceeds |end_offset|.
  while (next_equiv_.dst < end_offset && AdvanceEquivalence()) {
  }
  if (cur_equiv_.dst + cur_equiv_.length > end_offset)
    return false;
  // Same situation as above, but for |end_offset|. In this case |cur_*_it_| are
  // end-sentinels for EquivalenceGenerator of the current element.
  local_src_skip_ = Stream(start_src_skip_it, cur_src_skip_it_);
  local_dst_skip_ = Stream(start_dst_skip_it, cur_dst_skip_it_);
  local_copy_count_ = Stream(start_copy_count_it, cur_copy_count_it_);

  return true;
}

SourcePatch::EquivalenceGenerator
SourcePatch::LocalEquivalenceScanner::LocalGen() {
  return SourcePatch::EquivalenceGenerator(local_src_skip_, local_dst_skip_,
                                           local_copy_count_,
                                           equivalence_threshold_, base_equiv_);
}

/******** SourcePatch ********/

SourcePatch::SourcePatch(SourceStreamSet* streams,
                         offset_t equivalence_threshold)
    : streams_(streams),
      equivalence_threshold_(equivalence_threshold),
      command_gen_(streams_->Get(PatchField::kCommand)),
      extra_data_gen_(streams_->Get(PatchField::kExtraData)),
      raw_delta_gen_(streams_->Get(PatchField::kRawDeltaSkip),
                     streams_->Get(PatchField::kRawDeltaDiff)),
      local_reference_delta_gen_(streams_->Get(PatchField::kReferenceDelta)),
      equiv_scanner_(equivalence_threshold_,
                     streams_->Get(PatchField::kSrcSkip),
                     streams_->Get(PatchField::kDstSkip),
                     streams_->Get(PatchField::kCopyCount)) {
  uint32_t patch_type_int = NUM_PATCH_TYPES;
  if (!command_gen_(&patch_type_int)) {
    error_message_ = "Error reading patch type.";
    return;
  }
  patch_type_ = static_cast<zucchini::PatchType>(patch_type_int);
  switch (patch_type_) {
    case PATCH_TYPE_RAW:
      // TODO(huangs): Revisit this case: Maybe assign 0, and remove type
      // storage from Zucchini-gen?
      num_elements_ = 1;
      break;
    case PATCH_TYPE_SINGLE:
      num_elements_ = 1;
      break;
    case PATCH_TYPE_ENSEMBLE: {
      if (!command_gen_(&num_elements_) || num_elements_ == 0) {
        error_message_ = "Error reading number of elements.";
        return;
      }
      std::cout << "Found " << num_elements_ << " elements." << std::endl;
      break;
    }
    default:
      error_message_ = "Invalid patch type.";
      return;
  }

  for (size_t i = PatchField::kLabels; i < streams_->Count(); ++i)
    label_streams_.push_back(streams_->Get(i));
}

SourcePatch::~SourcePatch() = default;

bool SourcePatch::OutputErrorAndReturnFalse() {
  std::cout << error_message_ << std::endl;
  return false;
}

bool SourcePatch::CheckOK() {
  if (error_message_.empty())
    return true;
  return OutputErrorAndReturnFalse();
}

bool SourcePatch::LoadNextElement(Region old_image,
                                  Region new_image,
                                  EnsembleMatcher::Match* match) {
  if (cur_element_idx_ >= num_elements_) {
    error_message_ = "No matches left.";
    return false;
  }
  ++cur_element_idx_;

  EnsembleMatcher::Match temp_match;
  temp_match.old_element.base_image = old_image;
  temp_match.new_element.base_image = new_image;

  // Get element regions.
  if (patch_type_ == PATCH_TYPE_RAW || patch_type_ == PATCH_TYPE_SINGLE) {
    // Entire old and new images get patched.
    temp_match.old_element.sub_image = old_image;
    temp_match.new_element.sub_image = new_image;

  } else if (patch_type_ == PATCH_TYPE_ENSEMBLE) {
    uint32_t old_offset = 0;
    uint32_t old_size = 0;
    uint32_t new_offset = 0;
    uint32_t new_size = 0;
    if (!command_gen_(&old_offset) || !command_gen_(&old_size) ||
        !command_gen_(&new_offset) || !command_gen_(&new_size)) {
      error_message_ = "Error reading next match.";
      return false;
    }
    if (!CheckDataFit(old_offset, old_size, old_image.size())) {
      error_message_ = "Invalid old element.";
      return false;
    }
    if (!CheckDataFit(new_offset, new_size, new_image.size())) {
      error_message_ = "Invalid new element.";
      return false;
    }

    temp_match.old_element.sub_image =
        Region(old_image.begin() + old_offset, old_size);
    temp_match.new_element.sub_image =
        Region(new_image.begin() + new_offset, new_size);

  } else {
    error_message_ = "Unknown patch type.";
    return false;
  }

  // Read executable type.
  uint32_t exe_type_int = EXE_TYPE_UNKNOWN;
  if (!command_gen_(&exe_type_int)) {
    error_message_ = "Error reading executable type.";
    return false;
  }
  ExecutableType exe_type = static_cast<ExecutableType>(exe_type_int);
  // TODO(huangs): We're validating downstream. Should do it here.
  temp_match.old_element.exe_type = exe_type;
  temp_match.new_element.exe_type = exe_type;

  // Prepare local equivalence generators.
  // TODO(huangs): Can this ever be empty? Investigate and handle.
  if (!equiv_scanner_.Advance(temp_match.new_element.start_offset(),
                              temp_match.new_element.end_offset())) {
    error_message_ = "Error loading next local equivalence.";
    return false;
  }

  // Prepare local Label generators.
  uint32_t pool_count = 0;
  if (!command_gen_(&pool_count)) {
    error_message_ = "Error reading pool count.";
    return false;
  }
  if (pool_count > label_streams_.size()) {
    error_message_ = "Pool count exceeds number of label streams.";
    return false;
  }
  local_label_gen_list_ = decltype(local_label_gen_list_)();  // Clear.
  local_label_gen_list_.resize(pool_count);
  for (uint32_t pool = 0; pool < pool_count; ++pool) {
    Stream* labels = &label_streams_[pool];
    uint32_t count = 0;
    if (!(*labels)(&count)) {
      error_message_ = "Error reading label items count.";
      return false;
    }
    local_label_gen_list_[pool].reset(new LabelGenerator(labels, count));
  }

  *match = temp_match;
  return true;
}

SourcePatch::EquivalenceGenerator SourcePatch::GlobalEquivalenceGen() {
  return SourcePatch::EquivalenceGenerator(
      streams_->Get(PatchField::kSrcSkip), streams_->Get(PatchField::kDstSkip),
      streams_->Get(PatchField::kCopyCount), equivalence_threshold_, {0, 0, 0});
}

SourcePatch::EquivalenceGenerator SourcePatch::LocalEquivalenceGen() {
  return equiv_scanner_.LocalGen();
}

}  // namespace zucchini
