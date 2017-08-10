// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/zucchini_gen.h"

#include <utility>

namespace zucchini {

std::vector<offset_t> MakeNewLabelsFromEquivalenceMap(
    const std::vector<offset_t>& old_offsets,
    const std::vector<Equivalence>& equivalences) {
  auto current_equivalence = equivalences.begin();
  std::vector<offset_t> new_labels;
  new_labels.reserve(old_offsets.size());
  for (offset_t src : old_offsets) {
    while (current_equivalence != equivalences.end() &&
           current_equivalence->src_offset + current_equivalence->length <= src)
      ++current_equivalence;

    if (current_equivalence != equivalences.end() &&
        current_equivalence->src_offset <= src) {
      // Find the equivalence containing |src| with maximal length, and in case
      // of a tie, one with minimal destination offset.
      auto best_equivalence = current_equivalence;
      for (auto next_equivalence = current_equivalence;
           next_equivalence != equivalences.end() &&
           src >= next_equivalence->src_offset;
           ++next_equivalence) {
        if (next_equivalence->length > best_equivalence->length ||
            (next_equivalence->length == best_equivalence->length &&
             next_equivalence->dst_offset < best_equivalence->dst_offset)) {
          best_equivalence = next_equivalence;
        }
      }
      new_labels.push_back(src - best_equivalence->src_offset +
                           best_equivalence->dst_offset);
    } else {
      new_labels.push_back(kUnusedIndex);
    }
  }
  return new_labels;
}

std::vector<offset_t> FindExtraLabels(
    const std::vector<Reference>& new_references,
    const EquivalenceMap& equivalence_map) {
  auto equivalence = equivalence_map.begin();
  std::vector<offset_t> offsets;
  for (const Reference& ref : new_references) {
    while (equivalence != equivalence_map.end() &&
           equivalence->dst_offset + equivalence->length <= ref.location)
      ++equivalence;
    if (equivalence != equivalence_map.end() &&
        ref.location >= equivalence->dst_offset && !IsMarked(ref.target))
      offsets.push_back(ref.target);
  }
  return offsets;
}

status::Code GenerateEnsemble(ConstBufferView old_image,
                              ConstBufferView new_image,
                              EnsemblePatchWriter* patch_writer) {
  patch_writer->SetPatchType(PatchType::kEnsemblePatch);

  // Dummy patch element to fill patch_writer.
  PatchElementWriter patch_element(
      {Element(old_image.region()), Element(new_image.region())});
  patch_element.SetEquivalenceSink({});
  patch_element.SetExtraDataSink({});
  patch_element.SetRawDeltaSink({});
  patch_element.SetReferenceDeltaSink({});
  patch_writer->AddElement(std::move(patch_element));

  // TODO(etiennep): Implement.
  return zucchini::status::kStatusSuccess;
}

status::Code GenerateRaw(ConstBufferView old_image,
                         ConstBufferView new_image,
                         EnsemblePatchWriter* patch_writer) {
  patch_writer->SetPatchType(PatchType::kRawPatch);

  // Dummy patch element to fill patch_writer.
  PatchElementWriter patch_element(
      {Element(old_image.region()), Element(new_image.region())});
  patch_element.SetEquivalenceSink({});
  patch_element.SetExtraDataSink({});
  patch_element.SetRawDeltaSink({});
  patch_element.SetReferenceDeltaSink({});
  patch_writer->AddElement(std::move(patch_element));

  // TODO(etiennep): Implement.
  return zucchini::status::kStatusSuccess;
}

}  // namespace zucchini
