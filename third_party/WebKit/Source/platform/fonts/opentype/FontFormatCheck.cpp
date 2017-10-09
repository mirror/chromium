// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/fonts/opentype/FontFormatCheck.h"

#include "SkTypeface.h"
#include "platform/wtf/Vector.h"

namespace blink {

FontFormatCheck::FontFormatCheck(sk_sp<SkTypeface> typeface) {
  const size_t table_count = typeface->countTables();
  table_tags_.resize(table_count);
  if (!typeface->getTableTags(table_tags_.data()))
    table_tags_.resize(0);
}

bool FontFormatCheck::IsVariableFont() {
  return table_tags_.size() && table_tags_.Contains(SkFontTableTag(
                                   SkSetFourByteTag('f', 'v', 'a', 'r')));
}

bool FontFormatCheck::IsCbdtCblcColorFont() {
  return table_tags_.size() &&
         table_tags_.Contains(
             SkFontTableTag(SkSetFourByteTag('C', 'B', 'D', 'T'))) &&
         table_tags_.Contains(
             SkFontTableTag(SkSetFourByteTag('C', 'B', 'L', 'C')));
}
bool FontFormatCheck::IsCff2OutlineFont() {
  return table_tags_.size() && table_tags_.Contains(SkFontTableTag(
                                   SkSetFourByteTag('C', 'F', 'F', '2')));
}

}  // namespace blink
