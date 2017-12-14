// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/ensemble_matcher.h"

#include <limits>
#include <utility>

#include "base/logging.h"
#include "base/strings/stringprintf.h"

namespace zucchini {

/******** EnsembleMatcher ********/

EnsembleMatcher::EnsembleMatcher() = default;

EnsembleMatcher::~EnsembleMatcher() = default;

void EnsembleMatcher::Trim() {
  // Trim rule: If > 1 DEX files are found then ignore all DEX. This is done
  // because we do not yet support MultiDex, under which contents can move
  // across file boundary between "old" and "new" archives. When this occurs,
  // forcing matches of DEX files and patching them separately can result in
  // larger patches than naive patching.
  auto is_match_dex = [](const ElementMatch& match) {
    return match.old_element.exe_type == kExeTypeDex;
  };
  auto num_dex = std::count_if(matches_.begin(), matches_.end(), is_match_dex);
  if (num_dex > 1) {
    LOG(WARNING) << "Found " << num_dex << " DEX: Ignoring all." << std::endl;
    matches_.erase(
        std::remove_if(matches_.begin(), matches_.end(), is_match_dex),
        matches_.end());
  }
}

void EnsembleMatcher::GenerateSeparators(ConstBufferView new_image) {
  ConstBufferView::iterator it = new_image.begin();
  for (ElementMatch& match : matches_) {
    ConstBufferView new_sub_image(new_image[match.new_element.region()]);
    separators_.push_back(
        ConstBufferView::FromRange(it, new_sub_image.begin()));
    it = new_sub_image.end();
  }
  separators_.push_back(ConstBufferView::FromRange(it, new_image.end()));
}

}  // namespace zucchini
