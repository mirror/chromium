// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_ZUCCHINI_ZUCCHINI_APPLY_H_
#define CHROME_INSTALLER_ZUCCHINI_ZUCCHINI_APPLY_H_

#include <vector>

#include "chrome/installer/zucchini/image_utils.h"
#include "chrome/installer/zucchini/patch_reader.h"
#include "chrome/installer/zucchini/zucchini.h"

namespace zucchini {

// Projects targets in |old_targets| to a list of new targets using equivalences
// in |patch_reader|. Targets that cannot be projected have offset assigned as
// |kUnusedIndex|. Returns the list of new targets in a new vector.
std::vector<offset_t> MakeNewTargetsFromPatch(
    const std::vector<offset_t>& old_targets,
    const EquivalenceSource& equivalence_source);

// Forms preliminary |new_image| from |old_image| and |patch_reader|. The result
// can contain incorrect references.

// Reads equivalences from |patch_reader| to form preliminary |new_image|,
// copying regions from |old_image| and writing extra data from |patch_reader|.
bool ApplyEquivalenceAndExtraData(ConstBufferView old_image,
                                  const PatchElementReader& patch_reader,
                                  MutableBufferView new_image);

// Reads raw delta from |patch_reader| and apply corrections to new_image.
bool ApplyRawDelta(const PatchElementReader& patch_reader,
                   MutableBufferView new_image);

// Creates Disassemblers from |old_element| and |new_element|. Corrects
// references in the "new" Disassembler by projecting references of "old"
// Disassembler using data from |patch|.
bool ApplyReferencesCorrection(ExecutableType exe_type,
                               ConstBufferView old_image,
                               const PatchElementReader& patch_reader,
                               MutableBufferView new_image);

// Applies patch element with type |exe_type| from |patch_reader| on |old_image|
// to produce |new_image|.
bool ApplyElement(ExecutableType exe_type,
                  ConstBufferView old_image,
                  const PatchElementReader& patch_reader,
                  MutableBufferView new_image);

}  // namespace zucchini

#endif  // CHROME_INSTALLER_ZUCCHINI_ZUCCHINI_APPLY_H_
