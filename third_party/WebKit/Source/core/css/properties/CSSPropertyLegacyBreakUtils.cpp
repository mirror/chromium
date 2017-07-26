// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/properties/CSSPropertyLegacyBreakUtils.h"

#include "core/css/CSSValueList.h"
#include "core/css/parser/CSSPropertyParserHelpers.h"
#include "platform/RuntimeEnabledFeatures.h"

namespace blink {

bool CSSPropertyLegacyBreakUtils::ConsumeFromPageBreakBetween(
    CSSParserTokenRange& range,
    CSSValueID& value) {
  CSSIdentifierValue* keyword = CSSPropertyParserHelpers::ConsumeIdent(range);
  if (!keyword)
    return false;
  if (!range.AtEnd())
    return false;
  value = keyword->GetValueID();

  if (value == CSSValueAlways) {
    value = CSSValuePage;
    return true;
  }
  return value == CSSValueAuto || value == CSSValueAvoid ||
         value == CSSValueLeft || value == CSSValueRight;
}

bool CSSPropertyLegacyBreakUtils::ConsumeFromColumnBreakBetween(
    CSSParserTokenRange& range,
    CSSValueID& value) {
  CSSIdentifierValue* keyword = CSSPropertyParserHelpers::ConsumeIdent(range);
  if (!keyword)
    return false;
  if (!range.AtEnd())
    return false;
  value = keyword->GetValueID();

  if (value == CSSValueAlways) {
    value = CSSValueColumn;
    return true;
  }
  return value == CSSValueAuto || value == CSSValueAvoid;
}

bool CSSPropertyLegacyBreakUtils::ConsumeFromColumnOrPageBreakInside(
    CSSParserTokenRange& range,
    CSSValueID& value) {
  CSSIdentifierValue* keyword = CSSPropertyParserHelpers::ConsumeIdent(range);
  if (!keyword)
    return false;
  if (!range.AtEnd())
    return false;
  value = keyword->GetValueID();

  return value == CSSValueAuto || value == CSSValueAvoid;
}

}  // namespace blink
