// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/input/ime_text_span_conversions.h"

#include "base/logging.h"

namespace content {

blink::WebImeTextSpan::Type ConvertUiImeTextSpanTypeToWebType(
    ui::ImeTextSpan::Type type) {
  switch (type) {
    case ui::ImeTextSpan::Type::kComposition:
      return blink::WebImeTextSpan::Type::kComposition;
    case ui::ImeTextSpan::Type::kSuggestion:
      return blink::WebImeTextSpan::Type::kSuggestion;
    case ui::ImeTextSpan::Type::kMisspellingSuggestion:
      return blink::WebImeTextSpan::Type::kMisspellingSuggestion;
  }

  NOTREACHED();
  return blink::WebImeTextSpan::Type::kComposition;
}

ui::ImeTextSpan::Type ConvertWebImeTextSpanTypeToUiType(
    blink::WebImeTextSpan::Type type) {
  switch (type) {
    case blink::WebImeTextSpan::Type::kComposition:
      return ui::ImeTextSpan::Type::kComposition;
    case blink::WebImeTextSpan::Type::kSuggestion:
      return ui::ImeTextSpan::Type::kSuggestion;
    case blink::WebImeTextSpan::Type::kMisspellingSuggestion:
      return ui::ImeTextSpan::Type::kMisspellingSuggestion;
  }

  NOTREACHED();
  return ui::ImeTextSpan::Type::kComposition;
}

blink::WebImeTextSpan::Thickness ConvertUiImeTextSpanThicknessToWebThickness(
    ui::ImeTextSpan::Thickness thickness) {
  switch (thickness) {
    case ui::ImeTextSpan::Thickness::kNone:
      return blink::WebImeTextSpan::Thickness::kNone;
    case ui::ImeTextSpan::Thickness::kThin:
      return blink::WebImeTextSpan::Thickness::kThin;
    case ui::ImeTextSpan::Thickness::kThick:
      return blink::WebImeTextSpan::Thickness::kThick;
  }

  NOTREACHED();
  return blink::WebImeTextSpan::Thickness::kThin;
}

ui::ImeTextSpan::Thickness ConvertWebImeTextSpanThicknessToUiThickness(
    blink::WebImeTextSpan::Thickness thickness) {
  switch (thickness) {
    case blink::WebImeTextSpan::Thickness::kNone:
      return ui::ImeTextSpan::Thickness::kNone;
    case blink::WebImeTextSpan::Thickness::kThin:
      return ui::ImeTextSpan::Thickness::kThin;
    case blink::WebImeTextSpan::Thickness::kThick:
      return ui::ImeTextSpan::Thickness::kThick;
  }

  NOTREACHED();
  return ui::ImeTextSpan::Thickness::kThin;
}

}  // namespace content
