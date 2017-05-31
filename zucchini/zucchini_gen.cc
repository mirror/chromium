// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

#include "zucchini/disassembler.h"
#include "zucchini/encoded_view.h"
#include "zucchini/ensemble.h"
#include "zucchini/equivalence_map.h"
#include "zucchini/label_manager.h"
#include "zucchini/patch.h"
#include "zucchini/program_detector.h"
#include "zucchini/ranges/algorithm.h"
#include "zucchini/ranges/utility.h"
#include "zucchini/ranges/view.h"
#include "zucchini/reference_bytes_mixer.h"
#include "zucchini/reference_holder.h"
#include "zucchini/region.h"
#include "zucchini/sink_patch.h"
#include "zucchini/suffix_array.h"
#include "zucchini/zucchini.h"

namespace zucchini {

namespace {

/******** Helper Function Declarations ********/

// For patch generation: Projects labels in |old_label_manager| to
// |new_label_manager| using |equivalences|. Labels that cannot be projected are
// assigned |kUnusedIndex|.
void MakeNewLabelsFromEquivalenceMap(
    const OrderedLabelManager& old_label_manager,
    const EquivalenceMap& equivalences,
    UnorderedLabelManager* new_label_manager);

// Assigns labels to targets of |old_references| using |old_label_manager| if
// the label is projected in |new_label_manager|.
void AssignLabelsIfReused(const OrderedLabelManager& old_label_manager,
                          const UnorderedLabelManager& new_label_manager,
                          ReferenceHolder::Range old_references);

// Finds new labels from targets of references in |new_references| if their
// location is found in an equivalence of |equivalences|. These new labels are
// inserted to |new_label_manager|.
void FindExtraLabels(const EquivalenceMap& equivalences,
                     const ReferenceHolder::Range& new_references,
                     OrderedLabelManager* new_label_manager);

// Iterates over |local_equivalence_map|. Computes the global equivalences
// (using |src_base| and |dst_base|). Writes the resulting equivalences to
// |patch|. Writes data in |local_dst| but outside |local_equivalence_map| to
// the "extra data" stream in |patch|. "New" image regions in
// |local_equivalence_map| must be contained in |local_dst|.
bool GenerateEquivalencesAndExtraData(
    Region local_dst,
    const EquivalenceMap& local_equivalence_map,
    offset_t src_base,
    offset_t dst_base,
    SinkPatch* patch);

// Writes raw delta between |src| and |local_dst| matched by
// |local_equivalence_map| to |patch|, using |new_references| and
// |reference_bytes_mixer| to modify reference bytes to handle special cases and
// to reduce delta. "New" image regions in |local_equivalence_map| must be
// contained in |local_dst|.
bool GenerateRawDelta(Region src,
                      Region local_dst,
                      const EquivalenceMap& local_equivalence_map,
                      const ReferenceHolder& new_references,
                      const ReferenceBytesMixer& reference_bytes_mixer,
                      SinkPatch* patch);

// Writes reference delta between |src_refs| and |dst_refs| to |patch|.
bool GenerateReferencesDelta(const ReferenceHolder& src_refs,
                             const ReferenceHolder& dst_refs,
                             const EquivalenceMap& equivalences,
                             SinkPatch* patch);

// Given a |match|, generates patch data and writes them to |patch|.
bool GenerateForMatch(const EnsembleMatcher::Match& match, SinkPatch* patch);

// Writes ensemble patch data to |patch| using |ensemble_matcher| data.
bool GenerateEnsemblePatch(Region old_image,
                           Region new_image,
                           const EnsembleMatcher& ensemble_matcher,
                           SinkPatch* patch);

/******** Helper Function Definitions ********/

void MakeNewLabelsFromEquivalenceMap(
    const OrderedLabelManager& old_label_manager,
    const EquivalenceMap& equivalences,
    UnorderedLabelManager* new_label_manager) {
  std::vector<offset_t> new_labels(old_label_manager.size(), kUnusedIndex);
  offset_t idx = 0;
  EquivalenceMap::ForwardMapper forward_map = equivalences.GetForwardMapper();
  for (offset_t l : old_label_manager.Labels()) {
    EquivalenceMap::ForwardMapper::iterator e = forward_map(l);
    if (e != equivalences.end()) {
      auto best = e;
      auto it = e + 1;
      while (it != equivalences.end() && l >= it->src) {
        if ((it->length > best->length) ||
            (it->length == best->length && it->dst < best->dst)) {
          best = it;
        }
        ++it;
      }
      offset_t dst = l - best->src + best->dst;
      new_labels[idx] = dst;
    }
    ++idx;
  }
  new_label_manager->Init(std::move(new_labels));
}

void AssignLabelsIfReused(const OrderedLabelManager& old_label_manager,
                          const UnorderedLabelManager& new_label_manager,
                          ReferenceHolder::Range old_references) {
  namespace rv = ranges::view;

  old_label_manager.Assign(Targets(old_references));

  old_label_manager.Unassign(Targets(
      rv::RemoveIf(old_references, [&new_label_manager](const Reference& ref) {
        return new_label_manager.at(UnmarkIndex(ref.target)) != kUnusedIndex;
      })));
}

void FindExtraLabels(const EquivalenceMap& equivalences,
                     const ReferenceHolder::Range& new_references,
                     OrderedLabelManager* new_label_manager) {
  namespace rv = ranges::view;

  EquivalenceMap::BackwardMapper backward_map =
      equivalences.GetBackwardMapper();

  new_label_manager->Allocate(Targets(rv::RemoveIf(
      new_references, [&backward_map, &equivalences](const Reference& ref) {
        return backward_map(ref.location) == equivalences.end();
      })));
}

bool GenerateEquivalencesAndExtraData(
    Region local_dst,
    const EquivalenceMap& local_equivalence_map,
    offset_t src_base,
    offset_t dst_base,
    SinkPatch* patch) {
  // Store data in gaps in |local_dst| before / between / after
  // |local_equivalence_map| blocks as "extra data".
  SinkPatch::ExtraDataReceptor* extra_data_rec = patch->ExtraData();
  SinkPatch::EquivalenceReceptor* equivalences_rec = patch->Equivalences();

  Region::iterator dst_it = local_dst.begin();
  for (Equivalence e : local_equivalence_map) {
    (*equivalences_rec)(e.DisplaceBy(src_base, dst_base));
    Region::iterator next_dst = local_dst.begin() + e.dst;
    (*extra_data_rec)(Region(dst_it, next_dst));
    dst_it = next_dst + e.length;
  }
  if (dst_it > local_dst.end())
    return false;
  (*extra_data_rec)(Region(dst_it, local_dst.end()));
  return true;
}

bool GenerateRawDelta(Region src,
                      Region local_dst,
                      const EquivalenceMap& local_equivalence_map,
                      const ReferenceHolder& new_references,
                      const ReferenceBytesMixer& reference_bytes_mixer,
                      SinkPatch* patch) {
  // Upper bound for reference instructions across all platforms.
  constexpr int kMaxReferenceInstructionSize = 8;

  ReferenceHolder::ConstLocSortedRange refs =
      new_references.Get(SortedByLocation);
  ReferenceHolder::ConstLocSortedRange::iterator ref = begin(refs);
  SinkPatch::RawDeltaReceptor* raw_delta_rec = patch->RawDelta();

  uint8_t mixed_ref_bytes[kMaxReferenceInstructionSize];
  Region mixed_ref_region(mixed_ref_bytes, kMaxReferenceInstructionSize);

  auto byte_diff = [](uint8_t src_byte, uint8_t dst_byte) -> int8_t {
    return dst_byte - src_byte;
  };

  // Visit |local_equivalence_map| blocks in |local_dst| order, in lockstep
  // with |new_references|. Find and emit all bytewise differences.
  for (Equivalence e : local_equivalence_map) {
    copy_offset_t base_copy_offset = raw_delta_rec->GetBaseCopyOffset();
    while (ref != end(refs) &&
           ref->location + new_references.Width(ref->type) <= e.dst) {
      ++ref;
    }
    // Error if |ref| straddles across |e.dst|.
    assert(ref == end(refs) || ref->location >= e.dst);

    // For each bytewise delta from |src| to |local_dst|, compute "copy offset"
    // and pass it along with delta to the receptor.
    uint32_t i = 0;
    while (i < e.length) {
      if (ref != end(refs) && ref->location == e.dst + i) {
        // Reference delta has its own flow. On some architectures (e.g., x86)
        // this does not involve raw delta, so we skip. On other architectures
        // (e.g., ARM) references are mixed with other bits that may change, so
        // we need to "mix" data and store some changed bits into raw delta.
        int num_bytes = reference_bytes_mixer.NumBytes(ref->type);
        if (num_bytes) {
          reference_bytes_mixer.Mix(ref->type, src.begin(), e.src + i,
                                    local_dst.begin(), e.dst + i,
                                    mixed_ref_region.begin());
          for (int j = 0; j < num_bytes; ++j) {
            int8_t diff = byte_diff(src[e.src + i + j], mixed_ref_bytes[j]);
            if (diff)
              (*raw_delta_rec)({base_copy_offset + i + j, diff});
          }
        }
        i += new_references.Width(ref->type);
        ++ref;

      } else {
        int8_t diff = byte_diff(src[e.src + i], local_dst[e.dst + i]);
        if (diff)
          (*raw_delta_rec)({base_copy_offset + i, diff});
        ++i;
      }
    }
    raw_delta_rec->SetBaseCopyOffset(base_copy_offset + e.length);
  }
  return true;
}

// |equivalences| is assumed to be sorted by destination.
bool GenerateReferencesDelta(const ReferenceHolder& src_refs,
                             const ReferenceHolder& dst_refs,
                             const EquivalenceMap& equivalences,
                             SinkPatch* patch) {
  namespace rv = ranges::view;

  SinkPatch::ReferenceDeltaReceptor* reference_delta_rec =
      patch->ReferenceDelta();
  for (uint8_t type = 0; type < src_refs.TypeCount(); ++type) {
    // |e| visits ordered, non-overlapping destination ranges of equivalences.
    // |dst_ref| visits ordered references in the destination.
    // We visit all intersecting |e| and |dst_ref| in lockstep.
    EquivalenceMap::const_iterator e = equivalences.begin();
    // |src_ref| is the counterpart to |dst_ref| in source. On visiting a new
    // |e|, we find it by binary search (lower_bound()). Once the initial match
    // is found, we can linearly advance. Here we assign uninitialized state.
    auto uninit_src_ref = end(src_refs.Get(type));
    auto src_ref = uninit_src_ref;

    for (auto dst_ref : dst_refs.Get(type)) {
      // Increment |e| until it catches up to |dst_ref|.
      while (e != equivalences.end() &&
             e->dst + e->length <= dst_ref.location) {
        ++e;
        src_ref = uninit_src_ref;
      }
      if (e == equivalences.end())
        continue;

      // If |dst_ref| lags |e|, skip so it catches up.
      if (e->dst > dst_ref.location) {
        // |dst_ref| should not straddle start of |e|.
        assert(e->dst >= dst_ref.location + dst_refs.Width(type));
        continue;
      }

      // Found: Now |dst_ref| and |e| intersect.
      // |dst_ref| should not straddle end of |e|.
      assert(dst_ref.location + dst_refs.Width(type) <= e->dst + e->length);

      offset_t src_loc = e->src + (dst_ref.location - e->dst);
      if (src_ref == uninit_src_ref) {
        src_ref = ranges::lower_bound(src_refs.Get(type), src_loc,
                                      ranges::less(), &Reference::location);
      } else {
        while (src_ref != end(src_refs.Get(type)) &&
               src_ref->location < src_loc) {
          ++src_ref;
        }
      }
      assert(src_ref != uninit_src_ref);
      assert(src_loc == src_ref->location);
      offset_t dst_target = dst_ref.target;
      offset_t src_target = src_ref->target;
      assert(IsMarked(dst_target));
      assert(IsMarked(src_target));

      (*reference_delta_rec)(static_cast<int32_t>(UnmarkIndex(dst_target) -
                                                  UnmarkIndex(src_target)));
    }
  }
  return true;
}

bool GenerateForMatch(const EnsembleMatcher::Match& match, SinkPatch* patch) {
  // Initialize elements and disassemblers.
  const Ensemble::Element& old_element = match.old_element;
  const Ensemble::Element& new_element = match.new_element;
  Region old_sub_image = old_element.sub_image;
  Region new_sub_image = new_element.sub_image;
  ExecutableType exe_type = new_element.exe_type;
  assert(old_element.exe_type == exe_type);

  bool is_ensemble = new_sub_image.size() < new_element.base_image.size();
  if (is_ensemble) {
    std::cout << "--- Element [" << new_element.start_offset() << ","
              << new_element.end_offset() << ") <- ["
              << old_element.start_offset() << "," << old_element.end_offset()
              << "), type = " << exe_type << std::endl;
  }

  auto maker =
      exe_type == EXE_TYPE_NO_OP ? MakeNoOpDisassembler : MakeDisassembler;
  std::unique_ptr<Disassembler> old_disasm = maker(old_sub_image);
  std::unique_ptr<Disassembler> new_disasm = maker(new_sub_image);
  if (old_disasm->GetExeType() != new_disasm->GetExeType())
    return false;

  // Initialize views.
  ReferenceHolder old_refs;
  ReferenceHolder new_refs;
  old_refs.Insert(old_disasm->References());
  new_refs.Insert(new_disasm->References());

  EncodedView old_view(old_sub_image, old_refs);
  EncodedView new_view(new_sub_image, new_refs);

  // First iteration.
  std::unique_ptr<SuffixArray<const EncodedView&>> old_sa;

  old_sa = MakeSuffixArray<Sais, const EncodedView&>(old_view,
                                                     old_view.Cardinality());
  EquivalenceMap equivalences;
  equivalences.Build(old_view, *old_sa, new_view, kLargeEquivalenceScore);
  old_sa.reset(nullptr);

  std::vector<OrderedLabelManager> old_label_manager(old_refs.PoolCount());
  for (uint8_t type = 0; type < old_refs.TypeCount(); ++type) {
    old_label_manager[old_refs.Pool(type)].Allocate(
        Targets(old_refs.Get(type)));
  }

  // Label projection.
  std::vector<UnorderedLabelManager> new_label_manager(new_refs.PoolCount());
  for (uint8_t pool = 0; pool < new_label_manager.size(); ++pool) {
    std::cout << "Pool " << int(pool)
              << " old: " << old_label_manager[pool].size() << std::endl;
    MakeNewLabelsFromEquivalenceMap(old_label_manager[pool], equivalences,
                                    &new_label_manager[pool]);
    old_view.SetLabelCount(pool, old_label_manager[pool].size());
    new_view.SetLabelCount(pool, new_label_manager[pool].size());
  }
  for (uint8_t type = 0; type < old_refs.TypeCount(); ++type) {
    AssignLabelsIfReused(old_label_manager[old_refs.Pool(type)],
                         new_label_manager[new_refs.Pool(type)],
                         old_refs.Get(type));
    new_label_manager[new_refs.Pool(type)].Assign(Targets(new_refs.Get(type)));
  }

  // Second iteration.
  // Rebuild the suffix array, since |old_view.Cardinality()| can change.
  old_sa = MakeSuffixArray<Sais, const EncodedView&>(old_view,
                                                     old_view.Cardinality());
  equivalences.Build(old_view, *old_sa, new_view, kMinEquivalenceScore);
  old_sa.reset(nullptr);

  // Label projection.
  for (uint8_t type = 0; type < old_refs.TypeCount(); ++type) {
    old_label_manager[old_refs.Pool(type)].Assign(Targets(old_refs.Get(type)));
    new_label_manager[new_refs.Pool(type)].Unassign(
        Targets(new_refs.Get(type)));
  }
  for (uint8_t pool = 0; pool < new_label_manager.size(); ++pool) {
    MakeNewLabelsFromEquivalenceMap(old_label_manager[pool], equivalences,
                                    &new_label_manager[pool]);
  }
  for (uint8_t type = 0; type < old_refs.TypeCount(); ++type) {
    new_label_manager[new_refs.Pool(type)].Assign(Targets(new_refs.Get(type)));
  }

  // Write to patch.
  if (!patch->StartElementWrite(old_element, new_element,
                                new_disasm->GetMaxPoolCount())) {
    return false;
  }

  size_t pool_count = new_refs.PoolCount();
  equivalences.SortByDestination();
  std::vector<OrderedLabelManager> extra_label_manager(pool_count);
  for (uint8_t type = 0; type < old_refs.TypeCount(); ++type) {
    FindExtraLabels(equivalences, new_refs.Get(type),
                    &extra_label_manager[new_refs.Pool(type)]);
  }
  for (uint8_t pool = 0; pool < pool_count; ++pool) {
    std::cout << "Pool " << int(pool)
              << " extra: " << extra_label_manager[pool].size() << std::endl;
    new_label_manager[pool].Digest(extra_label_manager[pool].Labels());
    SinkPatch::LabelReceptor* label_rec =
        patch->LocalLabels(pool, extra_label_manager[pool].Labels().size());
    ranges::for_each(extra_label_manager[pool].Labels(), *label_rec);
  }
  for (uint8_t type = 0; type < old_refs.TypeCount(); ++type) {
    new_label_manager[new_refs.Pool(type)].Assign(Targets(new_refs.Get(type)));
  }

  if (!GenerateEquivalencesAndExtraData(new_sub_image, equivalences,
                                        old_element.start_offset(),
                                        new_element.start_offset(), patch)) {
    return false;
  }
  std::unique_ptr<ReferenceBytesMixer> reference_bytes_mixer =
      ReferenceBytesMixer::Create(*old_disasm, *new_disasm);
  if (!GenerateRawDelta(old_sub_image, new_sub_image, equivalences, new_refs,
                        *reference_bytes_mixer, patch)) {
    return false;
  }
  if (!GenerateReferencesDelta(old_refs, new_refs, equivalences, patch))
    return false;
  return patch->EndElementWrite();
}

bool GenerateEnsemblePatch(Region old_image,
                           Region new_image,
                           const EnsembleMatcher& ensemble_matcher,
                           SinkPatch* patch) {
  size_t num_elements = ensemble_matcher.GetMatches().size();
  assert(num_elements > 0);
  patch->WriteNumElements(num_elements);

  std::cout << "---------- Match separators ----------" << std::endl;

  // Compute equivalence map for "separators", i.e., data in "new" image that
  // lie outside of matched file pairs. This requires computing suffix array of
  // the entire "old" image, which takes a lot of memory. To enable this memory
  // to be freed earlier (before more suffix array computation for matched files
  // pairs), we compute and store separator equivalence maps in one fell swoop,
  // and process them later.
  std::vector<EquivalenceMap> sep_equivs(num_elements + 1);

  EncodedView old_view_raw(old_image, ReferenceHolder::GetEmptyRefs());
  std::unique_ptr<SuffixArray<const EncodedView&>> old_sa_raw;
  old_sa_raw = MakeSuffixArray<Sais, const EncodedView&>(old_view_raw, 256);

  for (size_t i = 0; i <= num_elements; ++i) {
    Region sep = ensemble_matcher.GetSeparators()[i];
    if (!sep.empty()) {
      offset_t sep_offset = sep.begin() - new_image.begin();
      std::cout << "Separator: " << sep_offset << " with size " << sep.size()
                << std::endl;

      EncodedView sep_view(sep, ReferenceHolder::GetEmptyRefs());
      sep_equivs[i].Build(old_view_raw, *old_sa_raw, sep_view,
                          kMinEquivalenceScore);
      sep_equivs[i].SortByDestination();
    }
  }
  old_sa_raw.reset(nullptr);

  std::cout << "---------- Emit streams ----------" << std::endl;

  ReferenceBytesMixer no_op_bytes_mixer;
  // Walk through |new_image| and emit all streams in proper order.
  for (size_t i = 0; i <= num_elements; ++i) {
    // Process separators, including possible one at end.
    Region sep = ensemble_matcher.GetSeparators()[i];
    if (!sep.empty()) {
      assert(sep_equivs[i].size());
      offset_t sep_offset =
          static_cast<offset_t>(sep.begin() - new_image.begin());
      std::cout << "--- Sep     [" << sep_offset << ","
                << sep_offset + sep.size() << ")" << std::endl;

      if (!GenerateEquivalencesAndExtraData(sep, sep_equivs[i], 0, sep_offset,
                                            patch)) {
        return false;
      }
      if (!GenerateRawDelta(old_image, sep, sep_equivs[i],
                            ReferenceHolder::GetEmptyRefs(), no_op_bytes_mixer,
                            patch)) {
        return false;
      }
    }
    if (i < num_elements) {
      // Process matched elements.
      if (!GenerateForMatch(ensemble_matcher.GetMatches()[i], patch))
        return false;
    }
  }

  std::cout << "----------------------" << std::endl;

  return true;
}

}  // namespace

bool Generate(const PatchOptions& opts,
              Region old_image,
              Region new_image,
              SinkStreamSet* patch_stream) {
  // Identify |patch_type| and compute |ensemble_matcher|.
  PatchType patch_type = PATCH_TYPE_RAW;
  EnsembleMatcher ensemble_matcher;
  auto generate_raw_patch_because = [&](const char* reason) {
    std::cout << reason << ": Generating raw patch." << std::endl;
    ensemble_matcher.RunRawMatch(old_image, new_image);
  };

  if (opts.force_raw) {
    generate_raw_patch_because("Raw mode enforced");
  } else if (!ensemble_matcher.RunMatch(old_image, new_image,
                                        opts.imposed_matches)) {
    // Forgive matcher failure; fall back to raw patch.
    generate_raw_patch_because(opts.imposed_matches.empty()
                                   ? "EnsembleMatcher failed"
                                   : "Imposed matches are invalid");
  } else {
    const std::vector<EnsembleMatcher::Match>& matches =
        ensemble_matcher.GetMatches();
    if (!opts.imposed_matches.empty())
      std::cout << "Imposed ";
    std::cout << "Matching: Found " << matches.size()
              << " nontrivial matches and "
              << ensemble_matcher.GetNumIdentical() << " identical matches."
              << std::endl;
    size_t num_elements = matches.size();
    if (!num_elements) {
      generate_raw_patch_because("No nontrival matches");
    } else if (num_elements == 1 &&
               matches[0].old_element.sub_image.size() == old_image.size() &&
               matches[0].new_element.sub_image.size() == new_image.size()) {
      // If |old_image| matches |new_image| entirely then we have single patch.
      std::cout << "Old and new files are executables: "
                << "Generating single-file patch." << std::endl;
      patch_type = PATCH_TYPE_SINGLE;
    } else {
      std::cout << "Generating ensemble patch." << std::endl;
      patch_type = PATCH_TYPE_ENSEMBLE;
    }
  }

  // Patch generation.
  SinkPatch patch(patch_stream, kMinEquivalenceScore + kBaseEquivalenceCost);
  patch.WritePatchType(patch_type);
  if (patch_type == PATCH_TYPE_RAW || patch_type == PATCH_TYPE_SINGLE) {
    assert(ensemble_matcher.GetMatches().size() == 1);
    return GenerateForMatch(ensemble_matcher.GetMatches()[0], &patch);
  } else {
    assert(patch_type == PATCH_TYPE_ENSEMBLE);
    return GenerateEnsemblePatch(old_image, new_image, ensemble_matcher,
                                 &patch);
  }
}

}  // namespace zucchini
