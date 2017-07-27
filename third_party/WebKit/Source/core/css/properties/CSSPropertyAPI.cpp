// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/properties/CSSPropertyAPI.h"

#include "core/CSSPropertyNames.h"
#include "core/css/CSSValue.h"
#include "core/css/parser/CSSParserContext.h"
#include "core/css/parser/CSSPropertyParser.h"

namespace blink {

bool CSSPropertyAPI::parseShorthand(
    CSSPropertyID property,
    bool important,
    CSSParserTokenRange& range,
    CSSParserContext& context,
    bool use_legacy_parsing,
    HeapVector<CSSProperty, 256>& properties) const {
  switch (property) {
    case CSSPropertyMarker: {
      const CSSValue* marker = CSSPropertyParser::ParseSingleValue(
          CSSPropertyMarkerStart, range, context);
      if (!marker || !range_.AtEnd())
        return false;
      CSSPropertyParser::AddParsedProperty(
          CSSPropertyMarkerStart, CSSPropertyMarker, *marker, important);
      CSSPropertyParser::AddParsedProperty(
          CSSPropertyMarkerMid, CSSPropertyMarker, *marker, important);
      CSSPropertyParser::AddParsedProperty(
          CSSPropertyMarkerEnd, CSSPropertyMarker, *marker, important);
      return true;
    }
    case CSSPropertyBorder:
      return ConsumeBorder(important);
    case CSSPropertyPageBreakAfter:
    case CSSPropertyPageBreakBefore:
    case CSSPropertyPageBreakInside:
    case CSSPropertyWebkitColumnBreakAfter:
    case CSSPropertyWebkitColumnBreakBefore:
    case CSSPropertyWebkitColumnBreakInside:
      return ConsumeLegacyBreakProperty(property, important);
    case CSSPropertyBackgroundRepeat:
    case CSSPropertyWebkitMaskRepeat: {
      CSSValue* result_x = nullptr;
      CSSValue* result_y = nullptr;
      bool implicit = false;
      if (!ConsumeRepeatStyle(range_, result_x, result_y, implicit) ||
          !range_.AtEnd())
        return false;
      CSSPropertyParser::AddParsedProperty(
          property == CSSPropertyBackgroundRepeat
              ? CSSPropertyBackgroundRepeatX
              : CSSPropertyWebkitMaskRepeatX,
          property, *result_x, important, implicit);
      CSSPropertyParser::AddParsedProperty(
          property == CSSPropertyBackgroundRepeat
              ? CSSPropertyBackgroundRepeatY
              : CSSPropertyWebkitMaskRepeatY,
          property, *result_y, important, implicit);
      return true;
    }
    case CSSPropertyBackground:
      return ConsumeBackgroundShorthand(backgroundShorthand(), important);
    case CSSPropertyWebkitMask:
      return ConsumeBackgroundShorthand(webkitMaskShorthand(), important);
    case CSSPropertyGridGap: {
      DCHECK(RuntimeEnabledFeatures::CSSGridLayoutEnabled());
      DCHECK_EQ(shorthandForProperty(CSSPropertyGridGap).length(), 2u);
      CSSValue* row_gap = ConsumeLengthOrPercent(range_, context.Mode(),
                                                 kValueRangeNonNegative);
      CSSValue* column_gap = ConsumeLengthOrPercent(range_, context.Mode(),
                                                    kValueRangeNonNegative);
      if (!row_gap || !range_.AtEnd())
        return false;
      if (!column_gap)
        column_gap = row_gap;
      CSSPropertyParser::AddParsedProperty(
          CSSPropertyGridRowGap, CSSPropertyGridGap, *row_gap, important);
      CSSPropertyParser::AddParsedProperty(
          CSSPropertyGridColumnGap, CSSPropertyGridGap, *column_gap, important);
      return true;
    }
    case CSSPropertyGridArea:
      return ConsumeGridAreaShorthand(important);
    case CSSPropertyGridTemplate:
      return ConsumeGridTemplateShorthand(CSSPropertyGridTemplate, important);
    case CSSPropertyGrid:
      return ConsumeGridShorthand(important);
    case CSSPropertyPlaceContent:
      return ConsumePlaceContentShorthand(important);
    case CSSPropertyPlaceItems:
      return ConsumePlaceItemsShorthand(important);
    case CSSPropertyPlaceSelf:
      return ConsumePlaceSelfShorthand(important);
    default:
      return false;
  }
}

}  // namespace blink
