// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_ZUCCHINI_ZUCCHINI_GEN_H_
#define CHROME_INSTALLER_ZUCCHINI_ZUCCHINI_GEN_H_

#include <vector>

#include "base/optional.h"
#include "chrome/installer/zucchini/buffer_view.h"
#include "chrome/installer/zucchini/image_utils.h"
#include "chrome/installer/zucchini/zucchini.h"

namespace zucchini {

class Disassembler;
class EquivalenceMap;
class SimilarityMap;
class ImageIndex;
class PatchElementWriter;

// Extracts all unmarked targets of references in |new_references| whose
// location is found in an equivalence of |equivalences|, and returns these
// targets in a new vector. |new_references| must be sorted in ascending order.
std::vector<offset_t> FindExtraTargets(
    const std::vector<offset_t> old_targets,
    const std::vector<offset_t> new_targets,
    const EquivalenceMap& equivalence_map);

// Creates an EquivalenceMap from "old" image to "new" image and returns the
// result. |*_image_index|:
// - Provide "old" and "new" raw image data and references.
// - Mediate Label matching, which link references between "old" and "new", and
//   guide EquivalenceMap construction.
// |*_image_index| is assumed to hold targets as *unmarked* offsets. These are
// also temporarily modified during Label matching -- that's why they're passed
// by pointer. Meanwhile, |old_label_manager| contains labels for
// |old_image_index|.
SimilarityMap CreateSimilarityMap(
    const ImageIndex& old_image_index,
    const ImageIndex& new_image_index);

// Writes equivalences from |equivalence_map|, and extra data from |new_image|
// found in gaps between equivalences to |patch_writer|.
bool GenerateEquivalencesAndExtraData(ConstBufferView new_image,
                                      const EquivalenceMap& equivalence_map,
                                      PatchElementWriter* patch_writer);

// Writes raw delta between |old_image| and |new_image| matched by
// |equivalence_map| to |patch_writer|, using |new_image_index| to ignore
// reference bytes.
bool GenerateRawDelta(ConstBufferView old_image,
                      ConstBufferView new_image,
                      const EquivalenceMap& equivalence_map,
                      const ImageIndex& new_image_index,
                      PatchElementWriter* patch_writer);

// Writes reference delta between references from |old_index| and from
// |new_index| to |patch_writer|.
bool GenerateReferencesDelta(const ImageIndex& old_index,
                             const ImageIndex& new_index,
                             const EquivalenceMap& equivalence_map,
                             PatchElementWriter* patch_writer);

// Writes |extra_targets| associated with |pool_tag| to |patch_writer|.
bool GenerateExtraTargets(const std::vector<offset_t>& extra_targets,
                          PoolTag pool_tag,
                          PatchElementWriter* patch_writer);

// Generates raw patch element data between |old_image| and |new_image|, and
// writes them to |patch_writer|. |old_sa| is the suffix array for |old_image|.
bool GenerateRawElement(const std::vector<offset_t>& old_sa,
                        ConstBufferView old_image,
                        ConstBufferView new_image,
                        PatchElementWriter* patch_writer);

// Generates patch element of type |exe_type| from |old_image| to |new_image|,
// and writes it to |patch_writer|.
bool GenerateExecutableElement(ExecutableType exe_type,
                               ConstBufferView old_image,
                               ConstBufferView new_image,
                               PatchElementWriter* patch_writer);

}  // namespace zucchini

#endif  // CHROME_INSTALLER_ZUCCHINI_ZUCCHINI_GEN_H_
