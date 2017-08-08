// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/editing/CompositionUnderline.h"

#include <algorithm>
#include "public/web/WebCompositionUnderline.h"

namespace blink {

CompositionUnderline::CompositionUnderline(
    Type type,
    unsigned start_offset,
    unsigned end_offset,
    const Color& underline_color,
    bool thick,
    const Color& background_color,
    const Color& suggestion_highlight_color,
    const Vector<String>& suggestions)
    : type_(type),
      underline_color_(underline_color),
      thick_(thick),
      background_color_(background_color),
      suggestion_highlight_color_(suggestion_highlight_color),
      suggestions_(suggestions) {
  // Sanitize offsets by ensuring a valid range corresponding to the last
  // possible position.
  // TODO(wkorman): Consider replacing with DCHECK_LT(startOffset, endOffset).
  start_offset_ =
      std::min(start_offset, std::numeric_limits<unsigned>::max() - 1u);
  end_offset_ = std::max(start_offset_ + 1u, end_offset);
}

namespace {

Vector<String> ConvertStdVectorOfStdStringsToVectorOfStrings(
    const std::vector<std::string>& input) {
  Vector<String> output;
  for (const std::string& val : input) {
    output.push_back(String::FromUTF8(val.c_str()));
  }
  return output;
}

CompositionUnderline::Type ConvertWebTypeToType(
    WebCompositionUnderline::Type type) {
  switch (type) {
    case WebCompositionUnderline::Type::COMPOSITION:
      return CompositionUnderline::Type::kComposition;
    case WebCompositionUnderline::Type::SUGGESTION:
      return CompositionUnderline::Type::kSuggestion;
    case WebCompositionUnderline::Type::MISSPELLING_SUGGESTION:
      return CompositionUnderline::Type::kMisspellingSuggestion;
  }
}

}  // namespace

CompositionUnderline::CompositionUnderline(
    const WebCompositionUnderline& underline)
    : CompositionUnderline(ConvertWebTypeToType(underline.type),
                           underline.start_offset,
                           underline.end_offset,
                           Color(underline.underline_color),
                           underline.thick,
                           Color(underline.background_color),
                           Color(underline.suggestion_highlight_color),
                           ConvertStdVectorOfStdStringsToVectorOfStrings(
                               underline.suggestions)) {}
}  // namespace blink
