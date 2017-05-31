// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ZUCCHINI_ZUCCHINI_H_
#define ZUCCHINI_ZUCCHINI_H_

#include <string>
#include <vector>

#include "zucchini/region.h"
#include "zucchini/stream.h"

namespace zucchini {

struct PatchOptions {
  // Whether to ignore structure and treat input files as raw binary data.
  bool force_raw = false;

  // Imposed matches, represented as a string. See EnsembleMatcher for details.
  // This is used by Zucchini-gen and Zucchini-match.
  std::string imposed_matches;
};

// The functions below return true on success, and false on failure.

// Generates patch from |old_image| to |new_image|, and writes it to
// |patch_stream|.
bool Generate(const PatchOptions& opts,
              Region old_image,
              Region new_image,
              SinkStreamSet* patch_stream);

// Applies |patch_stream| on |old_image| to build |new_image|.
bool Apply(Region old_image, SourceStreamSet* patch_stream, Region new_image);

// Prints stats on references found in |image|. If |do_dump| is true, then
// prints all references (locations and targets).
bool ReadRefs(Region image, bool do_dump);

// Prints regions and types of all detected executables in |image|. Appends
// detected subregions to |sub_image_list|.
bool DetectAll(Region image, std::vector<Region>* sub_image_list);

// Prints all matched regions from |old_image| to |new_image|.
bool MatchAll(const PatchOptions& opts, Region old_image, Region new_imager);

}  // namespace zucchini

#endif  // ZUCCHINI_ZUCCHINI_H_
