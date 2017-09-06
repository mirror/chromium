// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/properties/CSSPropertyAPIFontVariantEastAsian.h"

#include "core/CSSValueKeywords.h"
#include "core/css/parser/CSSPropertyParserHelpers.h"

namespace blink {

const CSSValue* CSSPropertyAPIFontVariantEastAsian::ParseSingleValue(
    CSSPropertyID,
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    const CSSParserLocalContext&) const {
  if (range.Peek().Id() == CSSValueNormal)
    return CSSPropertyParserHelpers::ConsumeIdent(range);

  CSSValueList* list = CSSValueList::CreateSpaceSeparated();
  bool has_form = false;
  bool has_width = false;
  bool has_ruby = false;
  do {
    CSSValueID id = range.Peek().Id();
    switch (id) {
      case CSSValueJis78:
      case CSSValueJis83:
      case CSSValueJis90:
      case CSSValueJis04:
      case CSSValueSimplified:
      case CSSValueTraditional:
        if (has_form)
          return nullptr;
        has_form = true;
        break;
      case CSSValueFullWidth:
      case CSSValueProportionalWidth:
        if (has_width)
          return nullptr;
        has_width = true;
        break;
      case CSSValueRuby:
        if (has_ruby)
          return nullptr;
        has_ruby = true;
        break;
      default:
        return nullptr;
    }
    list->Append(*CSSPropertyParserHelpers::ConsumeIdent(range));
  } while (!range.AtEnd());
  return list;
}

}  // namespace blink
