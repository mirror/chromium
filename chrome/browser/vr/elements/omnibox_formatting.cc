// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/omnibox_formatting.h"

#include "ui/gfx/font.h"
#include "ui/gfx/range/range.h"
#include "ui/gfx/text_constants.h"

namespace vr {

TextFormatting ConvertClassification(
    const ACMatchClassifications& classifications,
    size_t text_length) {
  TextFormatting formatting;
  formatting.push_back(
      TextFormattingAttribute(SUGGESTION_TEXT, gfx::Range(0, text_length)));

  for (size_t i = 0; i < classifications.size(); ++i) {
    const size_t text_start = classifications[i].offset;
    if (text_start >= text_length)
      break;

    const size_t text_end =
        (i < (classifications.size() - 1))
            ? std::min(classifications[i + 1].offset, text_length)
            : text_length;
    const gfx::Range current_range(text_start, text_end);

    if (classifications[i].style & ACMatchClassification::MATCH)
      formatting.push_back(
          TextFormattingAttribute(gfx::Font::Weight::BOLD, current_range));

    ColorId color_id = SUGGESTION_TEXT;
    if (classifications[i].style & ACMatchClassification::URL) {
      color_id = SUGGESTION_URL_TEXT;
      formatting.push_back(TextFormattingAttribute(
          gfx::DirectionalityMode::DIRECTIONALITY_AS_URL));
    } else if (classifications[i].style & ACMatchClassification::DIM) {
      color_id = SUGGESTION_DIM_TEXT;
    } else if (classifications[i].style & ACMatchClassification::INVISIBLE) {
      color_id = TRANSPARENT;
    }
    formatting.push_back(TextFormattingAttribute(color_id, current_range));
  }
  return formatting;
}

}  // namespace vr
