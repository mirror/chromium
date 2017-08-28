// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ImeFormattingMarker_h
#define ImeFormattingMarker_h

#include "core/editing/markers/StyleableMarker.h"

namespace blink {

// A subclass of DocumentMarker used to store formatting information added by an
// IME (most likely to assist with text composition). We store what color to
// display the underline (possibly transparent), whether the underline should
// be thick or not, and what background color should be used under the marked
// text (also possibly transparent).
class CORE_EXPORT ImeFormattingMarker final : public StyleableMarker {
 public:
  ImeFormattingMarker(unsigned start_offset,
                      unsigned end_offset,
                      Color underline_color,
                      Thickness,
                      Color background_color);

  // DocumentMarker implementations
  MarkerType GetType() const final;

 private:
  DISALLOW_COPY_AND_ASSIGN(ImeFormattingMarker);
};

DEFINE_TYPE_CASTS(ImeFormattingMarker,
                  DocumentMarker,
                  marker,
                  marker->GetType() == DocumentMarker::kImeFormatting,
                  marker.GetType() == DocumentMarker::kImeFormatting);

}  // namespace blink

#endif
