// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_IME_TEXT_COMPOSITION_DATA_H_
#define UI_BASE_IME_TEXT_COMPOSITION_DATA_H_

#include <vector>

#include "ui/base/ime/composition_underline.h"
#include "ui/base/ime/ui_base_ime_export.h"

namespace ui {

// Keep sync with blink::WebTextCompositionData defined in:
// third_party/WebKit/public/web/WebTextCompositionData.h

struct UI_BASE_IME_EXPORT TextCompositionData {
  TextCompositionData();
  TextCompositionData(const std::vector<ui::CompositionUnderline>&);

  ~TextCompositionData();

  TextCompositionData(const TextCompositionData&);

  bool operator==(const TextCompositionData& rhs) const;
  bool operator!=(const TextCompositionData& rhs) const;

  std::vector<CompositionUnderline> composition_underlines;
};

}  // namespace ui

#endif  // UI_BASE_IME_TEXT_COMPOSITION_DATA_H_
