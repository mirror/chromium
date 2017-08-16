// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/zucchini_gen.h"

#include <memory>
#include <utility>

#include "base/logging.h"
#include "chrome/installer/zucchini/disassembler.h"
#include "chrome/installer/zucchini/element_detection.h"
#include "chrome/installer/zucchini/encoded_view.h"
#include "chrome/installer/zucchini/equivalence_map.h"
#include "chrome/installer/zucchini/image_index.h"
#include "chrome/installer/zucchini/patch_writer.h"
#include "chrome/installer/zucchini/suffix_array.h"

namespace zucchini {

namespace {

// Parameters for patch generation.
constexpr double kMinEquivalenceSimilarity = 12.0;
constexpr double kLargeEquivalenceSimilarity = 64.0;

}  // namespace

base::Optional<ImageIndex> MakeImageIndex(ConstBufferView image,
                                          Disassembler* disasm) {
  std::vector<ReferenceGroup> ref_groups = disasm->MakeReferenceGroups();
  std::vector<ReferenceTypeTraits> reference_traits(ref_groups.size());
  std::transform(ref_groups.begin(), ref_groups.end(), reference_traits.begin(),
                 [](const ReferenceGroup& group) { return group.traits(); });
  ImageIndex image_index(image, std::move(reference_traits));
  for (const auto& ref_group : ref_groups) {
    if (!image_index.InsertReferences(ref_group.type_tag(),
                                      std::move(*ref_group.GetReader(disasm))))
      return base::nullopt;
  }
  return image_index;
}

std::vector<offset_t> MakeNewTargetsFromEquivalenceMap(
    const std::vector<offset_t>& old_targets,
    const std::vector<Equivalence>& equivalences) {
  auto current_equivalence = equivalences.begin();
  std::vector<offset_t> new_targets;
  new_targets.reserve(old_targets.size());
  for (offset_t src : old_targets) {
    while (current_equivalence != equivalences.end() &&
           current_equivalence->src_end() <= src)
      ++current_equivalence;

    if (current_equivalence != equivalences.end() &&
        current_equivalence->src_offset <= src) {
      // Select the longest equivalence that contains |src|. In case of a tie,
      // prefer equivalence with minimal |dst_offset|.
      auto best_equivalence = current_equivalence;
      for (auto next_equivalence = current_equivalence;
           next_equivalence != equivalences.end() &&
           src >= next_equivalence->src_offset;
           ++next_equivalence) {
        if (next_equivalence->length > best_equivalence->length ||
            (next_equivalence->length == best_equivalence->length &&
             next_equivalence->dst_offset < best_equivalence->dst_offset)) {
          // If an |next_equivalence| is longer or equal to |best_equivalence|,
          // it can be show that |src < next_equivalence->src_end()| i.e., |src|
          // is inside |next_equivalence|.
          DCHECK_LT(src, next_equivalence->src_end());
          best_equivalence = next_equivalence;
        }
      }
      new_targets.push_back(src - best_equivalence->src_offset +
                            best_equivalence->dst_offset);
    } else {
      new_targets.push_back(kUnusedIndex);
    }
  }
  return new_targets;
}

std::vector<offset_t> FindExtraTargets(
    const std::vector<Reference>& new_references,
    const EquivalenceMap& equivalence_map) {
  auto equivalence = equivalence_map.begin();
  std::vector<offset_t> targets;
  for (const Reference& ref : new_references) {
    while (equivalence != equivalence_map.end() &&
           equivalence->eq.dst_end() <= ref.location)
      ++equivalence;

    if (equivalence == equivalence_map.end())
      break;
    if (ref.location >= equivalence->eq.dst_offset && !IsMarked(ref.target))
      targets.push_back(ref.target);
  }
  return targets;
}

bool GenerateEquivalencesAndExtraData(ConstBufferView new_image,
                                      const EquivalenceMap& equivalence_map,
                                      PatchElementWriter* patch_writer) {
  // Make 2 passes through |equivalence_map| to reduce write churn.
  // Pass 1: Write all equivalences.
  EquivalenceSink equivalences_sink;
  for (const EquivalenceCandidate& candidate : equivalence_map)
    equivalences_sink.PutNext(candidate.eq);
  patch_writer->SetEquivalenceSink(std::move(equivalences_sink));

  // Pass 2: Write data in gaps in |new_image| before / between  after
  // |equivalence_map| as "extra data".
  ExtraDataSink extra_data_sink;
  offset_t dst_offset = 0;
  for (const EquivalenceCandidate& candidate : equivalence_map) {
    extra_data_sink.PutNext(
        new_image[{dst_offset, candidate.eq.dst_offset - dst_offset}]);
    dst_offset = candidate.eq.dst_end();
    DCHECK_LE(dst_offset, new_image.size());
  }
  extra_data_sink.PutNext(
      new_image[{dst_offset, new_image.size() - dst_offset}]);
  patch_writer->SetExtraDataSink(std::move(extra_data_sink));
  return true;
}

bool GenerateRawDelta(ConstBufferView old_image,
                      ConstBufferView new_image,
                      const EquivalenceMap& equivalence_map,
                      const ImageIndex& new_image_index,
                      PatchElementWriter* patch_writer) {
  RawDeltaSink raw_delta_sink;

  // Visit |equivalence_map| blocks in |new_image| order. Find and emit all
  // bytewise differences.
  offset_t base_copy_offset = 0;
  for (const EquivalenceCandidate& candidate : equivalence_map) {
    Equivalence equivalence = candidate.eq;
    // For each bytewise delta from |old_image| to |new_image|, compute "copy
    // offset" and pass it along with delta to the sink.
    for (offset_t i = 0; i < equivalence.length; ++i) {
      if (new_image_index.IsReference(equivalence.dst_offset + i))
        continue;  // Skip references since they're handled elsewhere.

      int8_t diff = new_image[equivalence.dst_offset + i] -
                    old_image[equivalence.src_offset + i];
      if (diff)
        raw_delta_sink.PutNext({base_copy_offset + i, diff});
    }
    base_copy_offset += equivalence.length;
  }
  patch_writer->SetRawDeltaSink(std::move(raw_delta_sink));
  return true;
}

bool GenerateReferencesDelta(const ImageIndex& old_index,
                             const ImageIndex& new_index,
                             const EquivalenceMap& equivalence_map,
                             PatchElementWriter* patch_writer) {
  ReferenceDeltaSink reference_delta_sink;
  for (uint8_t type = 0; type < old_index.TypeCount(); ++type) {
    TypeTag type_tag(type);
    size_t ref_width = old_index.GetTraits(type_tag).width;

    // |candidate| visits ordered, non-overlapping destination ranges of
    // equivalence_map.
    EquivalenceMap::const_iterator candidate = equivalence_map.begin();
    // |src_ref| is the counterpart to |dst_ref| in source. On visiting a new
    // |candidate|, we find it by binary search (lower_bound()). Once the
    // initial match is found, we can linearly advance. Here we assign
    // uninitialized state.
    const auto& old_refs = old_index.GetReferences(type_tag);
    auto uninit_src_ref = old_refs.end();
    auto src_ref = uninit_src_ref;

    // |dst_ref| and |candidate| are visited in lockstep.
    for (auto dst_ref : new_index.GetReferences(type_tag)) {
      // Increment |candidate| until it catches up to |dst_ref|.
      while (candidate != equivalence_map.end() &&
             candidate->eq.dst_offset + candidate->eq.length <=
                 dst_ref.location) {
        ++candidate;
        src_ref = uninit_src_ref;
      }
      if (candidate == equivalence_map.end())
        continue;

      // If |dst_ref| lags |candidate|, then it's a reference that's outside
      // of |equivalence_map|, so we skip it.
      if (candidate->eq.dst_offset > dst_ref.location) {
        // |dst_ref| should not straddle start of |candidate|.
        DCHECK_GE(candidate->eq.dst_offset, dst_ref.location + ref_width);
        continue;
      }

      // Found: Now |dst_ref| and |candidate| intersect. |dst_ref| should not
      // straddle end of |candidate|.
      DCHECK_LE(dst_ref.location + ref_width,
                candidate->eq.dst_offset + candidate->eq.length);

      offset_t src_loc = candidate->eq.src_offset +
                         (dst_ref.location - candidate->eq.dst_offset);
      if (src_ref == uninit_src_ref) {
        src_ref = std::lower_bound(
            old_refs.begin(), old_refs.end(), src_loc,
            [](const Reference& a, offset_t b) { return a.location < b; });
      } else {
        while (src_ref != old_refs.end() && src_ref->location < src_loc)
          ++src_ref;
      }
      DCHECK(src_ref != uninit_src_ref);
      DCHECK_EQ(src_loc, src_ref->location);
      offset_t dst_target = dst_ref.target;
      offset_t src_target = src_ref->target;
      DCHECK(IsMarked(dst_target));
      DCHECK(IsMarked(src_target));

      reference_delta_sink.PutNext(static_cast<int32_t>(
          UnmarkIndex(dst_target) - UnmarkIndex(src_target)));
    }
  }
  patch_writer->SetReferenceDeltaSink(std::move(reference_delta_sink));
  return true;
}

bool GenerateExtraTargets(const std::vector<offset_t>& extra_targets,
                          PoolTag pool_tag,
                          PatchElementWriter* patch_writer) {
  TargetSink target_sink;
  for (offset_t target : extra_targets) {
    target_sink.PutNext(target);
  }
  patch_writer->SetTargetSink(pool_tag, std::move(target_sink));
  return true;
}

bool GenerateRawElement(const std::vector<offset_t>& old_sa,
                        ConstBufferView old_image,
                        ConstBufferView new_image,
                        PatchElementWriter* patch_writer) {
  ImageIndex old_image_index(old_image, {});
  ImageIndex new_image_index(new_image, {});

  EquivalenceMap equivalences;
  equivalences.Build(old_sa, old_image_index, new_image_index,
                     kMinEquivalenceSimilarity);

  patch_writer->SetReferenceDeltaSink({});
  return GenerateEquivalencesAndExtraData(new_image, equivalences,
                                          patch_writer) &&
         GenerateRawDelta(old_image, new_image, equivalences, new_image_index,
                          patch_writer);
}

bool GenerateExecutableElement(ExecutableType exe_type,
                               ConstBufferView old_image,
                               ConstBufferView new_image,
                               PatchElementWriter* patch_writer) {
  // Initialize Disassemblers.
  std::unique_ptr<Disassembler> old_disasm =
      MakeDisassemblerOfType(old_image, exe_type);
  std::unique_ptr<Disassembler> new_disasm =
      MakeDisassemblerOfType(new_image, exe_type);
  if (!old_disasm || !new_disasm) {
    LOG(ERROR) << "Failed to create Disassembler";
    return false;
  }
  DCHECK_EQ(old_disasm->GetExeType(), new_disasm->GetExeType());

  // Initialize ImageIndexes.
  base::Optional<ImageIndex> old_image_index =
      MakeImageIndex(old_image, old_disasm.get());
  base::Optional<ImageIndex> new_image_index =
      MakeImageIndex(new_image, new_disasm.get());
  if (!old_image_index || !new_image_index) {
    LOG(ERROR) << "Failed to create ImageIndex.";
    return false;
  }
  DCHECK_EQ(old_image_index->PoolCount(), new_image_index->PoolCount());
  size_t pool_count = old_image_index->PoolCount();

  EncodedView old_view(&old_image_index.value());
  std::vector<offset_t> old_sa =
      MakeSuffixArray<InducedSuffixSort>(old_view, old_view.Cardinality());
  EquivalenceMap equivalences;
  equivalences.Build(old_sa, *old_image_index, *new_image_index,
                     kLargeEquivalenceSimilarity);

  // Initialize LabelManagers.
  std::vector<OrderedLabelManager> old_label_managers(pool_count);
  std::vector<UnorderedLabelManager> new_label_managers(pool_count);
  for (uint8_t pool = 0; pool < old_image_index->PoolCount(); ++pool) {
    old_label_managers[pool].InsertOffsets(
        old_image_index->GetTargets(PoolTag(pool)));

    const auto& old_label_manager = old_label_managers[pool];
    auto& new_label_manager = new_label_managers[pool];

    // Target projection to initialize |new_label_manager|.
    std::vector<offset_t> new_labels = MakeNewTargetsFromEquivalenceMap(
        old_label_manager.Labels(), equivalences.MakeForwardEquivalences());
    new_label_manager.Init(std::move(new_labels));

    old_image_index->LabelAssociatedTargets(PoolTag(pool), old_label_manager,
                                            new_label_manager);
    new_image_index->LabelTargets(PoolTag(pool), new_label_manager);
  }

  old_sa = MakeSuffixArray<InducedSuffixSort>(old_view, old_view.Cardinality());
  equivalences.Build(old_sa, *old_image_index, *new_image_index,
                     kMinEquivalenceSimilarity);

  for (uint8_t pool = 0; pool < old_image_index->PoolCount(); ++pool) {
    const auto& old_label_manager = old_label_managers[pool];
    auto& new_label_manager = new_label_managers[pool];

    // Restore |new_image_index| to offsets, since |new_label_manager| will
    // change.
    new_image_index->UnlabelTargets(PoolTag(pool), new_label_manager);

    // Label Projection to reinitialize |new_label_manager|.
    std::vector<offset_t> new_labels = MakeNewTargetsFromEquivalenceMap(
        old_label_manager.Labels(), equivalences.MakeForwardEquivalences());
    new_label_manager.Init(std::move(new_labels));

    // Convert |old_image_index| and |new_image_index| to marked indexes.
    old_image_index->LabelTargets(PoolTag(pool), old_label_manager);
    new_image_index->LabelTargets(PoolTag(pool), new_label_manager);

    // Find extra targets in |new_image_index|, emit into patch, merge them to
    // |new_labelsl_manager|, and update new references.
    OrderedLabelManager extra_label_manager;
    for (TypeTag type : new_image_index->GetTypeTags(PoolTag(pool))) {
      extra_label_manager.InsertOffsets(
          FindExtraTargets(new_image_index->GetReferences(type), equivalences));
    }
    if (!GenerateExtraTargets(extra_label_manager.Labels(), PoolTag(pool),
                              patch_writer))
      return false;

    for (offset_t offset : extra_label_manager.Labels())
      new_label_manager.InsertNewOffset(offset);

    // Convert |new_image_index| to indexes, using updated |new_label_manager|.
    new_image_index->LabelTargets(PoolTag(pool), new_label_manager);
  }

  return GenerateEquivalencesAndExtraData(new_image, equivalences,
                                          patch_writer) &&
         GenerateRawDelta(old_image, new_image, equivalences, *new_image_index,
                          patch_writer) &&
         GenerateReferencesDelta(*old_image_index, *new_image_index,
                                 equivalences, patch_writer);
}

/******** Exported Functions ********/

status::Code GenerateEnsemble(ConstBufferView old_image,
                              ConstBufferView new_image,
                              EnsemblePatchWriter* patch_writer) {
  patch_writer->SetPatchType(PatchType::kEnsemblePatch);

  base::Optional<Element> old_element =
      DetectElementFromDisassembler(old_image);
  base::Optional<Element> new_element =
      DetectElementFromDisassembler(new_image);

  if (!old_element.has_value() || !new_element.has_value() ||
      old_element->exe_type != new_element->exe_type)
    return GenerateRaw(old_image, new_image, patch_writer);

  if (old_element->region() != old_image.region() ||
      new_element->region() != new_image.region()) {
    LOG(ERROR) << "Ensemble patching is currently unsupported.";
    return status::kStatusFatal;
  }

  PatchElementWriter patch_element(ElementMatch{*old_element, *new_element});
  if (!GenerateExecutableElement(old_element->exe_type, old_image, new_image,
                                 &patch_element))
    return status::kStatusFatal;
  patch_writer->AddElement(std::move(patch_element));
  return status::kStatusSuccess;
}

status::Code GenerateRaw(ConstBufferView old_image,
                         ConstBufferView new_image,
                         EnsemblePatchWriter* patch_writer) {
  patch_writer->SetPatchType(PatchType::kRawPatch);

  ImageIndex old_image_index(old_image, {});
  EncodedView old_view(&old_image_index);
  std::vector<offset_t> old_sa =
      MakeSuffixArray<InducedSuffixSort>(old_view, old_view.Cardinality());

  PatchElementWriter patch_element(
      {Element(old_image.region()), Element(new_image.region())});
  if (!GenerateRawElement(old_sa, old_image, new_image, &patch_element))
    return status::kStatusFatal;
  patch_writer->AddElement(std::move(patch_element));
  return status::kStatusSuccess;
}

}  // namespace zucchini
