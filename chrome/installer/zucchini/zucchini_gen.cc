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
#include "chrome/installer/zucchini/targets_affinity.h"

namespace zucchini {

namespace {

// Parameters for patch generation.
constexpr double kMinEquivalenceSimilarity = 12.0;

}  // namespace

std::vector<offset_t> FindExtraTargets(
    const std::vector<offset_t>& projected_old_targets,
    const std::vector<offset_t>& new_targets) {
  std::vector<offset_t> extra_targets;
  std::set_difference(new_targets.begin(), new_targets.end(),
                      projected_old_targets.begin(),
                      projected_old_targets.end(),
                      std::back_inserter(extra_targets));
  return extra_targets;
}

SimilarityMap CreateSimilarityMap(
    const ImageIndex& old_image_index,
    const ImageIndex& new_image_index) {
  // Label matching (between "old" and "new") can guide EquivalenceMap
  // construction; but EquivalenceMap induces Label matching. This apparent
  // "chick and egg" problem is solved by bootstrapping a "coarse"
  // EquivalenceMap using |*image_index|.
  size_t pool_count = old_image_index.PoolCount();
  std::vector<TargetsAffinity> target_affinities(pool_count);
  
  SimilarityMap equivalence_map;
  // Associate targets from "old" to "new" image based on coarse
  // |equivalence_map| and label references accordingly.
  for (size_t i = 0; i < 2; ++i) {
    EncodedView old_view(&old_image_index);
    EncodedView new_view(&new_image_index);
    for (uint8_t pool = 0; pool < pool_count; ++pool) {
      target_affinities[pool].InferFromSimilarities(
          equivalence_map,
          old_image_index.GetTargetPool(PoolTag(pool)).targets(),
          new_image_index.GetTargetPool(PoolTag(pool)).targets());
      
      std::vector<size_t> old_labels;
      std::vector<size_t> new_labels;
      size_t label_bound = target_affinities[pool].AssignLabels(&old_labels, &new_labels);
      old_view.UpdateLabels(PoolTag(pool), std::move(old_labels), label_bound);
      new_view.UpdateLabels(PoolTag(pool), std::move(new_labels), label_bound);
      
    }
    equivalence_map.Build(
      MakeSuffixArray<InducedSuffixSort>(old_view, old_view.Cardinality()),
                        old_view, new_view, target_affinities,
                        kMinEquivalenceSimilarity);
  }

  return equivalence_map;
}

bool GenerateEquivalencesAndExtraData(ConstBufferView new_image,
                                      const SimilarityMap& equivalence_map,
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
                      const SimilarityMap& equivalence_map,
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
                             PoolTag pool_tag,
                             const TargetPool target_pool,
                             const EquivalenceMap& equivalence_map,
                             const SimilarityMap& similarity_map,
                             PatchElementWriter* patch_writer) {
  ReferenceDeltaSink reference_delta_sink;
  // Treat each type separately, for simplicity.
  // TODO(huangs): Investigate whether mixing all types is worthwhile.
  
  for (TypeTag type_tag : old_index.GetTypeTags(pool_tag)) {
    size_t ref_width = old_index.GetTraits(type_tag).width;

    const std::vector<Reference>& src_refs = old_index.GetReferences(type_tag);
    const std::vector<Reference>& dst_refs = new_index.GetReferences(type_tag);
    auto dst_ref = dst_refs.begin();

    // For each equivalence, for each covered |dst_ref| and the matching
    // |src_ref|, emit the delta between the respective target labels. Note: By
    // construction, reference locations (with |ref_width|) lies either
    // completely inside an equivalence or completely outside. We perform
    // "straddle checks" throughout to verify this assertion.
    for (const auto& candidate : similarity_map) {
      const Equivalence equiv = candidate.eq;
      // Increment |dst_ref| until it catches up to |equiv|.
      while (dst_ref != dst_refs.end() && dst_ref->location < equiv.dst_offset)
        ++dst_ref;
      if (dst_ref == dst_refs.end())
        break;
      if (dst_ref->location >= equiv.dst_end())
        continue;
      // Straddle check.
      DCHECK_LE(dst_ref->location + ref_width, equiv.dst_end());

      offset_t src_loc =
          equiv.src_offset + (dst_ref->location - equiv.dst_offset);
      auto src_ref = std::lower_bound(
          src_refs.begin(), src_refs.end(), src_loc,
          [](const Reference& a, offset_t b) { return a.location < b; });
      for (; dst_ref != dst_refs.end() &&
             dst_ref->location + ref_width <= equiv.dst_end();
           ++dst_ref, ++src_ref) {
        // Local offset of |src_ref| should match that of |dst_ref|.
        DCHECK_EQ(src_ref->location - equiv.src_offset,
                  dst_ref->location - equiv.dst_offset);
        offset_t old_offset = old_index.GetTargetPool(pool_tag).GetOffsetForKey(src_ref->target);
        offset_t new_expected_offset = equivalence_map.ProjectOffset(old_offset);
        offset_t new_expected_key = target_pool.FindKeyForOffset(new_expected_offset);
        offset_t new_offset = new_index.GetTargetPool(pool_tag).GetOffsetForKey(dst_ref->target);
        offset_t new_key = target_pool.FindKeyForOffset(new_offset);
        
        reference_delta_sink.PutNext(static_cast<int32_t>(
            new_key - new_expected_key));
        LOG(INFO) << int32_t(new_key - new_expected_key);
      }
      if (dst_ref == dst_refs.end())
        break;  // Done.
      // Straddle check.
      DCHECK_GE(dst_ref->location, equiv.dst_end());
    }
  }
  patch_writer->SetReferenceDeltaSink(std::move(reference_delta_sink));
  return true;
}

bool GenerateExtraTargets(const std::vector<offset_t>& extra_targets,
                          PoolTag pool_tag,
                          PatchElementWriter* patch_writer) {
  TargetSink target_sink;
  for (offset_t target : extra_targets)
    target_sink.PutNext(target);
  patch_writer->SetTargetSink(pool_tag, std::move(target_sink));
  return true;
}

bool GenerateRawElement(const std::vector<offset_t>& old_sa,
                        ConstBufferView old_image,
                        ConstBufferView new_image,
                        PatchElementWriter* patch_writer) {
  ImageIndex old_image_index(old_image);
  ImageIndex new_image_index(new_image);

  SimilarityMap equivalences;
  equivalences.Build(old_sa, EncodedView(&old_image_index), EncodedView(&new_image_index), {},
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
    LOG(ERROR) << "Failed to create Disassembler.";
    return false;
  }
  DCHECK_EQ(old_disasm->GetExeType(), new_disasm->GetExeType());

  // Initialize ImageIndexes.
  ImageIndex old_image_index(old_image);
  ImageIndex new_image_index(new_image);
  if (!old_image_index.InsertReferences(old_disasm.get()) ||
      !new_image_index.InsertReferences(new_disasm.get())) {
    LOG(ERROR) << "Failed to create ImageIndex.";
    return false;
  }

  DCHECK_EQ(old_image_index.PoolCount(), new_image_index.PoolCount());

  SimilarityMap similarity_map = CreateSimilarityMap(
      old_image_index, new_image_index);
  EquivalenceMap equivalence_map(similarity_map);

  for (uint8_t pool = 0; pool < old_image_index.PoolCount(); ++pool) {
    TargetPool projected_old_targets = old_image_index.GetTargetPool(PoolTag(pool));
    projected_old_targets.Project(equivalence_map);
    std::vector<offset_t> extra_target =
        FindExtraTargets(projected_old_targets.targets(), new_image_index.GetTargetPool(PoolTag(pool)).targets());
    projected_old_targets.InsertTargets(extra_target);
    

    if (!GenerateExtraTargets(extra_target, PoolTag(pool), patch_writer) ||
        !GenerateReferencesDelta(old_image_index, new_image_index, PoolTag(pool), projected_old_targets,
                                 equivalence_map, similarity_map,  patch_writer))
      return false;
  }

  return GenerateEquivalencesAndExtraData(new_image, similarity_map,
                                          patch_writer) &&
         GenerateRawDelta(old_image, new_image, similarity_map, new_image_index,
                          patch_writer);
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

  // If images do not match known executable formats, or if detected formats
  // don't match, fall back to generating a raw patch.
  if (!old_element.has_value() || !new_element.has_value() ||
      old_element->exe_type != new_element->exe_type) {
    LOG(WARNING) << "Fall back to raw mode.";
    return GenerateRaw(old_image, new_image, patch_writer);
  }

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

  ImageIndex old_image_index(old_image);
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
