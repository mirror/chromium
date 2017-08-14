// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/zucchini_apply.h"

#include <algorithm>
#include <map>
#include <memory>
#include <utility>

#include "base/logging.h"
#include "chrome/installer/zucchini/element_detection.h"
#include "chrome/installer/zucchini/label_manager.h"

namespace zucchini {

std::vector<offset_t> MakeNewTargetsFromPatch(
    const std::vector<offset_t>& old_targets,
    const EquivalenceSource& equivalence_source) {
  // |old_targets| is sorted. This enables binary search usage below.
  std::vector<offset_t> new_targets(old_targets.size(), kUnusedIndex);

  // First pass: For each equivalence, attempt to claim target in |new_targets|
  // associated with each target in the "old" interval of the equivalence. "Old"
  // interval collisions may occur.
  {
    EquivalenceSource equiv_source(equivalence_source);
    for (auto equivalence = equiv_source.GetNext(); equivalence.has_value();
         equivalence = equiv_source.GetNext()) {
      offset_t src = equivalence->src_offset;
      auto it = std::lower_bound(old_targets.begin(), old_targets.end(), src);
      offset_t idx = it - old_targets.begin();
      for (; it != old_targets.end() && *it < src + equivalence->length;
           ++it, ++idx) {
        // To resolve collisions, we temporarily assign target as marked maximal
        // length of equivalence. As a result, longer equivalences are favored.
        // In case of a tie, first claimant wins all.
        if (new_targets[idx] == kUnusedIndex ||
            UnmarkIndex(new_targets[idx]) < equivalence->length) {
          new_targets[idx] = MarkIndex(equivalence->length);
        }
      }
    }
  }

  // Second pass: Assign each claimed target in |new_targets|.
  {
    EquivalenceSource equiv_source(equivalence_source);
    for (auto equivalence = equiv_source.GetNext(); equivalence.has_value();
         equivalence = equiv_source.GetNext()) {
      auto it = std::lower_bound(old_targets.begin(), old_targets.end(),
                                 equivalence->src_offset);
      offset_t idx = it - old_targets.begin();
      for (; it != old_targets.end() &&
             *it < equivalence->src_offset + equivalence->length;
           ++it, ++idx) {
        if (IsMarked(new_targets[idx]) &&
            UnmarkIndex(new_targets[idx]) == equivalence->length) {
          new_targets[idx] =
              *it - equivalence->src_offset + equivalence->dst_offset;
        }
      }
    }
  }
  return new_targets;
}

bool ApplyEquivalenceAndExtraData(ConstBufferView old_image,
                                  const PatchElementReader& patch_reader,
                                  MutableBufferView new_image) {
  EquivalenceSource equiv_source = patch_reader.GetEquivalenceSource();
  ExtraDataSource extra_data_source = patch_reader.GetExtraDataSource();
  MutableBufferView::iterator dst_it = new_image.begin();
  for (auto equivalence = equiv_source.GetNext(); equivalence.has_value();
       equivalence = equiv_source.GetNext()) {
    auto* next_dst_it = new_image.begin() + equivalence->dst_offset;
    CHECK(next_dst_it >= dst_it);
    offset_t gap = static_cast<offset_t>(next_dst_it - dst_it);
    base::Optional<ConstBufferView> extra_data = extra_data_source.GetNext(gap);
    if (!extra_data) {
      LOG(ERROR) << "Error reading extra_data";
      return false;
    }
    dst_it = std::copy(extra_data->begin(), extra_data->end(), dst_it);
    CHECK_EQ(dst_it, next_dst_it);
    dst_it = std::copy_n(old_image.begin() + equivalence->src_offset,
                         equivalence->length, dst_it);
    CHECK_EQ(dst_it, next_dst_it + equivalence->length);
  }
  offset_t gap = static_cast<offset_t>(new_image.end() - dst_it);
  base::Optional<ConstBufferView> extra_data = extra_data_source.GetNext(gap);
  if (!extra_data) {
    LOG(ERROR) << "Error reading extra_data";
    return false;
  }
  std::copy(extra_data->begin(), extra_data->end(), dst_it);
  if (!equiv_source.Done() || !extra_data_source.Done()) {
    LOG(ERROR) << "Error reading equivalence and extra_data";
    return false;
  }
  return true;
}

bool ApplyRawDelta(const PatchElementReader& patch_reader,
                   MutableBufferView new_image) {
  EquivalenceSource equiv_source = patch_reader.GetEquivalenceSource();
  RawDeltaSource raw_delta_source = patch_reader.GetRawDeltaSource();
  // Traverse |equiv_source| and |raw_delta_source| in lockstep.
  auto equivalence = equiv_source.GetNext();
  offset_t base_offset = 0;
  for (auto delta = raw_delta_source.GetNext(); delta.has_value();
       delta = raw_delta_source.GetNext()) {
    while (equivalence.has_value() &&
           delta->copy_offset >= base_offset + equivalence->length) {
      base_offset += equivalence->length;
      equivalence = equiv_source.GetNext();
    }
    if (!equivalence.has_value()) {
      LOG(ERROR) << "Error reading equivalences";
      return false;
    }
    CHECK_GE(delta->copy_offset, base_offset);
    CHECK_LT(delta->copy_offset, base_offset + equivalence->length);

    // Invert byte diff.
    new_image[equivalence->dst_offset - base_offset + delta->copy_offset] +=
        delta->diff;
  }
  if (!raw_delta_source.Done()) {
    LOG(ERROR) << "Error reading raw_delta";
    return false;
  }
  return true;
}

bool ApplyReferencesCorrection(ExecutableType exe_type,
                               ConstBufferView old_image,
                               const PatchElementReader& patch,
                               MutableBufferView new_image) {
  auto old_disasm = MakeDisassemblerOfType(old_image, exe_type);
  auto new_disasm =
      MakeDisassemblerOfType(ConstBufferView(new_image), exe_type);
  if (!old_disasm || !new_disasm) {
    LOG(ERROR) << "Failed to create Disassembler";
    return false;
  }

  ReferenceDeltaSource ref_delta_source = patch.GetReferenceDeltaSource();

  std::map<PoolTag, std::vector<ReferenceGroup>> pool_groups;

  for (const auto& ref_group : old_disasm->GetReferenceGroups()) {
    pool_groups[ref_group.pool_tag()].push_back(ref_group);
  }

  std::vector<ReferenceGroup> new_groups = new_disasm->GetReferenceGroups();
  for (const auto& pool_and_sub_groups : pool_groups) {
    PoolTag pool_tag = pool_and_sub_groups.first;
    const std::vector<ReferenceGroup>& sub_groups = pool_and_sub_groups.second;

    // Load all old targets for the pool.
    OrderedLabelManager old_label_manager;
    for (ReferenceGroup group : sub_groups)
      old_label_manager.InsertTargets(
          std::move(*group.GetReader(old_disasm.get())));

    // Generate estimated new targets for the pool.
    std::vector<offset_t> new_targets = MakeNewTargetsFromPatch(
        old_label_manager.Labels(), patch.GetEquivalenceSource());
    UnorderedLabelManager new_label_manager;
    new_label_manager.Init(std::move(new_targets));

    TargetSource target_source = patch.GetExtraTargetSource(pool_tag);
    for (auto offset = target_source.GetNext(); offset.has_value();
         offset = target_source.GetNext())
      new_label_manager.InsertNewOffset(offset.value());
    if (!target_source.Done()) {
      LOG(ERROR) << "Error reading extra_targets";
      return false;
    }

    // Correct all new references, and write results to |new_disasm|.
    for (ReferenceGroup group : sub_groups) {
      std::unique_ptr<ReferenceWriter> ref_writer =
          new_groups[group.type_tag().value()].GetWriter(new_image,
                                                         new_disasm.get());

      EquivalenceSource equiv_source = patch.GetEquivalenceSource();
      for (auto equivalence = equiv_source.GetNext(); equivalence.has_value();
           equivalence = equiv_source.GetNext()) {
        std::unique_ptr<ReferenceReader> ref_gen = group.GetReader(
            equivalence->src_offset,
            equivalence->src_offset + equivalence->length, old_disasm.get());
        for (auto ref = ref_gen->GetNext(); ref; ref = ref_gen->GetNext()) {
          DCHECK_GE(ref->location, equivalence->src_offset);
          DCHECK_LT(ref->location,
                    equivalence->src_offset + equivalence->length);
          offset_t index = old_label_manager.IndexOfOffset(ref->target);
          auto delta = ref_delta_source.GetNext();
          if (!delta.has_value()) {
            LOG(ERROR) << "Error reading reference_delta";
            return false;
          }
          ref->target = new_label_manager.OffsetOfIndex(index + delta.value());
          ref->location =
              ref->location - equivalence->src_offset + equivalence->dst_offset;
          ref_writer->PutNext(*ref);
        }
      }
    }
  }
  return true;
}

bool ApplyElement(ExecutableType exe_type,
                  ConstBufferView old_image,
                  const PatchElementReader& patch_reader,
                  MutableBufferView new_image) {
  return ApplyEquivalenceAndExtraData(old_image, patch_reader, new_image) &&
         ApplyRawDelta(patch_reader, new_image) &&
         ApplyReferencesCorrection(exe_type, old_image, patch_reader,
                                   new_image);
}

status::Code Apply(ConstBufferView old_image,
                   const EnsemblePatchReader& patch_reader,
                   MutableBufferView new_image) {
  if (!patch_reader.CheckOldFile(old_image)) {
    LOG(ERROR) << "Invalid old_image.";
    return status::kStatusInvalidOldImage;
  }

  for (const auto& element_patch : patch_reader.elements()) {
    ElementMatch match = element_patch.element_match();
    if (!ApplyElement(match.old_element.exe_type,
                      old_image[match.old_element.region()], element_patch,
                      new_image[match.new_element.region()]))
      return status::kStatusFatal;
  }

  if (!patch_reader.CheckNewFile(ConstBufferView(new_image))) {
    LOG(ERROR) << "Invalid new_image.";
    return status::kStatusInvalidNewImage;
  }
  return status::kStatusSuccess;
}

}  // namespace zucchini
