// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_IME_COMPOSITION_UNDERLINE_H_
#define UI_BASE_IME_COMPOSITION_UNDERLINE_H_

#include <stdint.h>

#include <string>
#include <vector>

#include "third_party/skia/include/core/SkColor.h"
#include "ui/base/ime/ui_base_ime_export.h"

namespace ui {

// Intentionally keep sync with blink::WebCompositionUnderline defined in:
// third_party/WebKit/public/web/WebCompositionUnderline.h

struct UI_BASE_IME_EXPORT CompositionUnderline {
  enum class Type {
    // Creates a composition marker
    COMPOSITION,
    // Creates a suggestion marker that isn't cleared after the user picks a
    // replacement
    SUGGESTION,
    // Creates a suggestion marker that is cleared after the user picks a
    // replacement
    MISSPELLING_SUGGESTION
  };

  // The default constructor is used by generated Mojo code.
  CompositionUnderline();
  // TODO(huangs): remove this constructor.
  CompositionUnderline(uint32_t s, uint32_t e, SkColor c, bool t);
  CompositionUnderline(
      Type ty,
      uint32_t s,
      uint32_t e,
      SkColor uc,
      bool th,
      SkColor bc,
      SkColor shc = SK_ColorTRANSPARENT,
      const std::vector<std::string>& su = std::vector<std::string>());

  CompositionUnderline(const CompositionUnderline& rhs);

  ~CompositionUnderline();

  bool operator<(const CompositionUnderline& rhs) const {
    return std::tie(start_offset, end_offset) <
           std::tie(rhs.start_offset, rhs.end_offset);
  }

  bool operator==(const CompositionUnderline& rhs) const {
    return (this->type == rhs.type) &&
           (this->start_offset == rhs.start_offset) &&
           (this->end_offset == rhs.end_offset) &&
           (this->underline_color == rhs.underline_color) &&
           (this->thick == rhs.thick) &&
           (this->background_color == rhs.background_color) &&
           (this->suggestion_highlight_color ==
            rhs.suggestion_highlight_color) &&
           (this->suggestions == rhs.suggestions);
  }

  bool operator!=(const CompositionUnderline& rhs) const {
    return !(*this == rhs);
  }

  Type type;
  uint32_t start_offset;
  uint32_t end_offset;
  SkColor underline_color;
  bool thick;
  SkColor background_color;
  SkColor suggestion_highlight_color;
  std::vector<std::string> suggestions;
};

typedef std::vector<CompositionUnderline> CompositionUnderlines;

}  // namespace ui

#endif  // UI_BASE_IME_COMPOSITION_UNDERLINE_H_
