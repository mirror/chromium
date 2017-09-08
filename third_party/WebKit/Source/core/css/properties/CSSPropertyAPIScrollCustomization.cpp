// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/properties/CSSPropertyAPIScrollCustomization.h"

#include "core/css/CSSValueList.h"
#include "core/css/parser/CSSPropertyParserHelpers.h"
#include "platform/RuntimeEnabledFeatures.h"

class CSSParserLocalContext;
namespace blink {

namespace {

static bool ConsumePan(CSSParserTokenRange& range,
                       CSSValue*& pan_x,
                       CSSValue*& pan_y) {
  CSSValueID id = range.Peek().Id();
  if ((id == CSSValuePanX || id == CSSValuePanRight || id == CSSValuePanLeft) &&
      !pan_x) {
    pan_x = CSSPropertyParserHelpers::ConsumeIdent(range);
  } else if ((id == CSSValuePanY || id == CSSValuePanDown ||
              id == CSSValuePanUp) &&
             !pan_y) {
    pan_y = CSSPropertyParserHelpers::ConsumeIdent(range);
  } else {
    return false;
  }
  return true;
}

}  // namespace

const CSSValue* CSSPropertyAPIScrollCustomization::ParseSingleValue(
    CSSPropertyID,
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    const CSSParserLocalContext&) const {
  CSSValueList* list = CSSValueList::CreateSpaceSeparated();
  CSSValueID id = range.Peek().Id();
  if (id == CSSValueAuto || id == CSSValueNone) {
    list->Append(*CSSPropertyParserHelpers::ConsumeIdent(range));
    return list;
  }

  CSSValue* pan_x = nullptr;
  CSSValue* pan_y = nullptr;
  if (!ConsumePan(range, pan_x, pan_y))
    return nullptr;
  if (!range.AtEnd() && !ConsumePan(range, pan_x, pan_y))
    return nullptr;

  if (pan_x)
    list->Append(*pan_x);
  if (pan_y)
    list->Append(*pan_y);
  return list;
}

}  // namespace blink
