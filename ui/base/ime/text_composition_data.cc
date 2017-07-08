// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/ime/text_composition_data.h"

namespace ui {

TextCompositionData::TextCompositionData() = default;

TextCompositionData::TextCompositionData(
    const std::vector<ui::CompositionUnderline>& cu)
    : composition_underlines(cu) {}

TextCompositionData::~TextCompositionData() = default;

TextCompositionData::TextCompositionData(const TextCompositionData&) = default;

bool TextCompositionData::operator==(const TextCompositionData& rhs) const {
  return composition_underlines == rhs.composition_underlines;
}

bool TextCompositionData::operator!=(const TextCompositionData& rhs) const {
  return !(*this == rhs);
}

}  // namespace ui
