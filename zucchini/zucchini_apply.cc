// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <map>
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
#include "zucchini/ranges/view.h"
#include "zucchini/region.h"
#include "zucchini/source_patch.h"
#include "zucchini/zucchini.h"

namespace zucchini {

namespace {

/******** Helper Function Declarations ********/

// Form preliminary |new_image| from |old_image| and |patch|. The result can
// contain incorrect references.
bool ApplyRawPatch(Region old_image, SourcePatch* patch, Region new_image);

// Creates a new disassembler from |element|. On success, returns true and
// assigns the new disassembler to |disasm|. Otherwise (specifically, created
// disassembler has unexpected type) returns false.
bool CreateDisassemblerFromElement(const Ensemble::Element& element,
                                   std::unique_ptr<Disassembler>* disasm);

// For patch application: Projects labels in |old_label_manager| to
// |new_label_manager| using the equivalences found in |patch|, and inserts
// additional labels found in |patch|. Labels that cannot be projected are
// assigned |kUnusedIndex|.
void MakeNewLabelsFromPatch(const OrderedLabelManager& old_label_manager,
                            SourcePatch* patch,
                            offset_t old_base,
                            offset_t new_base,
                            UnorderedLabelManager* new_label_manager);

// Creates disassemblers from |old_element| and |new_element|. Corrects
// references in the "new" disassembler by projecting references of "old"
// disassembler using data from |patch|.
bool ApplyReferencesCorrection(const Ensemble::Element& old_element,
                               SourcePatch* patch,
                               const Ensemble::Element& new_element);

/******** Helper Function Definitions ********/

bool ApplyRawPatch(Region old_image, SourcePatch* patch, Region new_image) {
  // First pass: Visit each equivalence (sorted by |new_image| offsets).
  // Alternatingly insert extra data and copy |old_image| data.
  {
    SourcePatch::EquivalenceGenerator equiv_gen = patch->GlobalEquivalenceGen();
    SourcePatch::ExtraDataGenerator* extra_data_gen =
        patch->GlobalExtraDataGen();
    Region::iterator dst_it = new_image.begin();
    for (Equivalence e; equiv_gen(&e);) {
      Region::iterator next_dst_it = new_image.begin() + e.dst;
      if (!(*extra_data_gen)(Region(dst_it, next_dst_it)))
        return false;
      dst_it = std::copy_n(old_image.begin() + e.src, e.length, next_dst_it);
    }
    if (!(*extra_data_gen)(Region(dst_it, new_image.end())))
      return false;
  }

  // Second pass: Correct bytewise differences in portions of |new_image| that
  // were copied from |old_image|. Note that we obtain "copy offsets" from the
  // generator, and need to translate them to file offsets.
  {
    SourcePatch::EquivalenceGenerator equiv_gen = patch->GlobalEquivalenceGen();
    SourcePatch::RawDeltaGenerator* raw_delta_gen = patch->GlobalRawDeltaGen();
    // Traverse |equiv_gen| and |raw_delta_gen| in lockstep.
    Equivalence e;
    RawDeltaUnit d;
    if (!(*raw_delta_gen)(&d))  // No raw delta: Early exit.
      return true;
    if (!equiv_gen(&e))  // Raw delta without equivalence: Something's wrong.
      return false;
    copy_offset_t base_copy_offset = 0;
    for (;;) {
      if (d.copy_offset < base_copy_offset + e.length) {
        offset_t offset = e.dst + (d.copy_offset - base_copy_offset);
        *(new_image.begin() + offset) += d.diff;  // Invert byte_diff().
        if (!(*raw_delta_gen)(&d))
          break;  // Can skip remaining equivalences.
      } else {
        base_copy_offset += e.length;
        if (!equiv_gen(&e))
          return false;  // Ran out of equivalence first: Something's wrong.
      }
    }
  }
  return true;
}

bool CreateDisassemblerFromElement(const Ensemble::Element& element,
                                   std::unique_ptr<Disassembler>* disasm) {
  ExecutableType exe_type = element.exe_type;
  Region sub_image = element.sub_image;
  auto maker =
      exe_type == EXE_TYPE_NO_OP ? MakeNoOpDisassembler : MakeDisassembler;
  std::unique_ptr<Disassembler> temp_disasm = maker(sub_image);
  if (temp_disasm->GetExeType() != exe_type) {
    std::cout << "Disassembler has unexpected type: Expect " << exe_type
              << ", got " << temp_disasm->GetExeType() << std::endl;
    return false;
  }
  *disasm = std::move(temp_disasm);
  return true;
}

void MakeNewLabelsFromPatch(const OrderedLabelManager& old_label_manager,
                            SourcePatch* patch,
                            offset_t old_base,
                            offset_t new_base,
                            UnorderedLabelManager* new_label_manager) {
  // |old_labels| is sorted. This enables binary search usage below.
  const std::vector<offset_t>& old_labels = old_label_manager.Labels();
  std::vector<offset_t> new_labels(old_label_manager.size(), kUnusedIndex);

  // First pass: For each equivalence, attempt to claim labels in |new_labels|
  // that matches (by index) each label in the "old" interval of the
  // equivalence. "Old" interval collisions may occur.
  {
    SourcePatch::EquivalenceGenerator local_equiv_gen =
        patch->LocalEquivalenceGen();
    for (Equivalence e; local_equiv_gen(&e);) {
      offset_t src = e.src - old_base;
      auto it = ranges::lower_bound(old_labels, src);
      offset_t idx = it - old_labels.begin();
      for (; it != old_labels.end() && *it < src + e.length; ++it, ++idx) {
        // To resolve collisions, we temporarily assign label as marked maximal
        // length of equivalence. As a result, longer equivalences are favored.
        // In case of a tie, first claimant wins all.
        if (new_labels[idx] == kUnusedIndex ||
            UnmarkIndex(new_labels[idx]) < e.length) {
          new_labels[idx] = MarkIndex(e.length);
        }
      }
    }
  }

  // Second pass: Assign each claimed label in |new_labels|.
  {
    SourcePatch::EquivalenceGenerator local_equiv_gen =
        patch->LocalEquivalenceGen();
    for (Equivalence e; local_equiv_gen(&e);) {
      offset_t src = e.src - old_base;
      offset_t dst = e.dst - new_base;
      auto it = ranges::lower_bound(old_labels, src);
      offset_t idx = it - old_labels.begin();
      for (; it != old_labels.end() && *it < src + e.length; ++it, ++idx) {
        if (IsMarked(new_labels[idx]) &&
            UnmarkIndex(new_labels[idx]) == e.length) {
          new_labels[idx] = *it - src + dst;
        }
      }
    }
  }

  new_label_manager->Init(std::move(new_labels));
}

bool ApplyReferencesCorrection(const Ensemble::Element& old_element,
                               SourcePatch* patch,
                               const Ensemble::Element& new_element) {
  std::unique_ptr<Disassembler> old_disasm;
  // The "new" image can have incorrect (and even invalid) references, but
  // |new_disasm| obtained here has enough data to make corrections.
  std::unique_ptr<Disassembler> new_disasm;
  if (!CreateDisassemblerFromElement(old_element, &old_disasm) ||
      !CreateDisassemblerFromElement(new_element, &new_disasm)) {
    return false;
  }
  offset_t old_base = old_element.start_offset();
  offset_t new_base = new_element.start_offset();

  SourcePatch::ReferenceDeltaGenerator* ref_delta_gen =
      patch->LocalReferenceDeltaGen();

  std::map<uint8_t, std::vector<ReferenceGroup>> pool_groups;
  GroupRange groups = old_disasm->References();  // |groups| gets consumed.
  for (ReferenceGroup group : groups)
    pool_groups[group.Pool()].push_back(group);

  for (const auto& pool_and_sub_groups : pool_groups) {
    uint8_t pool = pool_and_sub_groups.first;
    const std::vector<ReferenceGroup>& sub_groups = pool_and_sub_groups.second;

    // Load all old labels for the pool.
    OrderedLabelManager old_labels;
    for (ReferenceGroup group : sub_groups)
      old_labels.Allocate(group, Target());

    // Generate estimated new labels for the pool
    UnorderedLabelManager new_labels;
    MakeNewLabelsFromPatch(old_labels, patch, old_base, new_base, &new_labels);
    SourcePatch::LabelGenerator* label_gen = patch->LocalLabelGen(pool);
    if (!label_gen)
      return false;
    new_labels.Digest(ranges::view::MakePtrIterable<offset_t>(label_gen));

    // Correct all new labels, and write results to |new_disasm|.
    for (ReferenceGroup group : sub_groups) {
      ReferenceReceptor ref_rec = new_disasm->Receive(group.Type());
      SourcePatch::EquivalenceGenerator local_equiv_gen =
          patch->LocalEquivalenceGen();
      for (Equivalence e; local_equiv_gen(&e);) {
        offset_t src = e.src - old_base;
        offset_t dst = e.dst - new_base;
        ReferenceGenerator ref_gen = group.Find(src, src + e.length);
        for (Reference ref; ref_gen(&ref);) {
          assert(ref.location >= src);
          assert(ref.location < src + e.length);
          old_labels.Assign(&ref.target);
          int32_t d;
          (*ref_delta_gen)(&d);
          ref.target += d;
          new_labels.Unassign(&ref.target);
          ref.location = ref.location - src + dst;
          ref_rec(ref);
        }
      }
    }
  }
  return true;
}

}  // namespace

bool Apply(Region old_image, SourceStreamSet* patch_stream, Region new_image) {
  SourcePatch patch(patch_stream, kMinEquivalenceScore + kBaseEquivalenceCost);
  if (!patch.CheckOK())
    return false;

  // Globally copy equivalences and insert extra data.
  ApplyRawPatch(old_image, &patch, new_image);

  // Process each element.
  size_t num_elements = patch.GetNumElements();
  for (size_t file_idx = 0; file_idx < num_elements; ++file_idx) {
    std::cout << "*** Element " << file_idx << " ***" << std::endl;
    EnsembleMatcher::Match match;
    if (!patch.LoadNextElement(old_image, new_image, &match))
      return patch.OutputErrorAndReturnFalse();

    if (!ApplyReferencesCorrection(match.old_element, &patch,
                                   match.new_element)) {
      return false;
    }
  }
  return patch.CheckOK();
}

}  // namespace zucchini
