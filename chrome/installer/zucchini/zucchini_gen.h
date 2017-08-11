// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_ZUCCHINI_ZUCCHINI_GEN_H_
#define CHROME_INSTALLER_ZUCCHINI_ZUCCHINI_GEN_H_

#include <vector>

#include "chrome/installer/zucchini/equivalence_map.h"
#include "chrome/installer/zucchini/image_utils.h"
#include "chrome/installer/zucchini/zucchini.h"

namespace zucchini {

// Projects Labels in |old_label_manager| to a list of new Labels using
// |equivalences|. Labels that cannot be projected have offset assigned to
// |kUnusedIndex|. Returns the list of new Labels.
std::vector<offset_t> MakeNewLabelsFromEquivalenceMap(
    const std::vector<offset_t>& old_offsets,
    const std::vector<Equivalence>& equivalences);

// Extracts all targets of references in |new_references| whose location is
// found in an equivalence of |equivalences|, and returns these targets in a
// new vector.
std::vector<offset_t> FindExtraLabels(
    const std::vector<Reference>& new_references,
    const EquivalenceMap& equivalences);

}  // namespace zucchini

#endif  // CHROME_INSTALLER_ZUCCHINI_ZUCCHINI_GEN_H_
