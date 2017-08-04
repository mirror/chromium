// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/editing/markers/StyleableMarker.h"

namespace blink {

StyleableMarker::StyleableMarker(unsigned start_offset,
                                 unsigned end_offset,
                                 bool use_text_color,
                                 Color underline_color,
                                 Thickness thickness,
                                 Color background_color)
    : DocumentMarker(start_offset, end_offset),
      use_text_color_(use_text_color),
      underline_color_(underline_color),
      background_color_(background_color),
      thickness_(thickness) {}

bool StyleableMarker::UseTextColor() const {
  return use_text_color_;
}

Color StyleableMarker::UnderlineColor() const {
  return underline_color_;
}

bool StyleableMarker::IsThick() const {
  return thickness_ == Thickness::kThick;
}

Color StyleableMarker::BackgroundColor() const {
  return background_color_;
}

bool IsStyleableMarker(const DocumentMarker& marker) {
  DocumentMarker::MarkerType type = marker.GetType();
  return type == DocumentMarker::kComposition ||
         type == DocumentMarker::kActiveSuggestion;
}

}  // namespace blink
