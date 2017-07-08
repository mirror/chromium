// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/editing/TextCompositionData.h"

#include "core/editing/CompositionUnderlineVectorBuilder.h"
#include "public/web/WebTextCompositionData.h"

namespace blink {

TextCompositionData::TextCompositionData() {}

TextCompositionData::TextCompositionData(
    const Vector<CompositionUnderline>& composition_underlines)
    : composition_underlines_(composition_underlines) {}

TextCompositionData::TextCompositionData(
    const WebTextCompositionData& web_text_composition_data)
    : TextCompositionData(CompositionUnderlineVectorBuilder::Build(
          web_text_composition_data.composition_underlines)) {}

TextCompositionData::~TextCompositionData() = default;

/*
bool TextCompositionData::operator==(const TextCompositionData& rhs) const {
  return composition_underlines_ == rhs.composition_underlines;
}

bool TextCompositionData::operator!=(const TextCompositionData& rhs) const {
  return !(*this == rhs);
}
*/

}  // namespace blink
