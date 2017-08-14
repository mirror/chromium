// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/ime/ime_text_span.h"

#include <string>
#include <vector>

namespace ui {

ImeTextSpan::ImeTextSpan()
    : type(Type::COMPOSITION),
      start_offset(0),
      end_offset(0),
      underline_color(SK_ColorTRANSPARENT),
      thick(false),
      background_color(SK_ColorTRANSPARENT),
      suggestion_highlight_color(SK_ColorTRANSPARENT),
      suggestions(std::vector<std::string>()) {}

ImeTextSpan::ImeTextSpan(uint32_t s, uint32_t e, SkColor c, bool t)
    : type(Type::COMPOSITION),
      start_offset(s),
      end_offset(e),
      underline_color(c),
      thick(t),
      background_color(SK_ColorTRANSPARENT),
      suggestion_highlight_color(SK_ColorTRANSPARENT),
      suggestions(std::vector<std::string>()) {}

ImeTextSpan::ImeTextSpan(Type ty,
                         uint32_t s,
                         uint32_t e,
                         SkColor uc,
                         bool th,
                         SkColor bc,
                         SkColor shc,
                         const std::vector<std::string>& su)
    : type(ty),
      start_offset(s),
      end_offset(e),
      underline_color(uc),
      thick(th),
      background_color(bc),
      suggestion_highlight_color(shc),
      suggestions(su) {}

ImeTextSpan::ImeTextSpan(const ImeTextSpan& rhs) = default;

ImeTextSpan::~ImeTextSpan() = default;

}  // namespace ui
