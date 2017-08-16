// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/properties/CSSShorthandPropertyAPIWebkitMask.h"

#include "core/StylePropertyShorthand.h"
#include "core/css/CSSInitialValue.h"
#include "core/css/parser/CSSParserContext.h"
#include "core/css/parser/CSSPropertyParserHelpers.h"
#include "core/css/properties/CSSPropertyBackgroundUtils.h"
#include "core/css/properties/CSSPropertyPositionUtils.h"

namespace blink {

namespace {

CSSValue* ConsumeWebkitMaskComponent(CSSPropertyID resolved_property,
                                     CSSParserTokenRange& range,
                                     const CSSParserContext& context) {
  switch (resolved_property) {
    case CSSPropertyWebkitMaskClip:
      return CSSPropertyBackgroundUtils::ConsumePrefixedBackgroundBox(
          range, &context, true /* allow_text_value */);
    case CSSPropertyWebkitMaskOrigin:
      return CSSPropertyBackgroundUtils::ConsumePrefixedBackgroundBox(
          range, &context, false /* allow_text_value */);
    case CSSPropertyWebkitMaskImage:
      return CSSPropertyParserHelpers::ConsumeImageOrNone(range, &context);
    case CSSPropertyWebkitMaskPositionX:
      return CSSPropertyPositionUtils::ConsumePositionLonghand<CSSValueLeft,
                                                               CSSValueRight>(
          range, context.Mode());
    case CSSPropertyWebkitMaskPositionY:
      return CSSPropertyPositionUtils::ConsumePositionLonghand<CSSValueTop,
                                                               CSSValueBottom>(
          range, context.Mode());
    case CSSPropertyWebkitMaskSize:
      return CSSPropertyBackgroundUtils::ConsumeBackgroundSize(
          range, context.Mode(), false /* use_legacy_parsing */);
    default:
      break;
  };
  return nullptr;
}

}  // namespace

// Note: this assumes y properties (e.g. -webkit-mask-position-y) follow the x
// properties in the shorthand array.
bool CSSShorthandPropertyAPIWebkitMask::ParseShorthand(
    bool important,
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    bool use_legacy_parsing,
    HeapVector<CSSProperty, 256>& properties) {
  const StylePropertyShorthand shorthand = webkitMaskShorthand();
  const unsigned longhand_count = shorthand.length();
  CSSValue* longhands[8] = {0};
  DCHECK_EQ(longhand_count, 8u);

  bool implicit = false;
  do {
    bool parsed_longhand[8] = {false};
    CSSValue* origin_value = nullptr;
    do {
      bool found_property = false;
      for (size_t i = 0; i < longhand_count; ++i) {
        if (parsed_longhand[i])
          continue;

        CSSValue* value = nullptr;
        CSSValue* value_y = nullptr;
        CSSPropertyID property = shorthand.properties()[i];
        if (property == CSSPropertyWebkitMaskRepeatX) {
          CSSPropertyBackgroundUtils::ConsumeRepeatStyleComponent(
              range, value, value_y, implicit);
        } else if (property == CSSPropertyWebkitMaskPositionX) {
          if (!CSSPropertyParserHelpers::ConsumePosition(
                  range, context,
                  CSSPropertyParserHelpers::UnitlessQuirk::kForbid,
                  WebFeature::kThreeValuedPositionBackground, value, value_y))
            continue;
        } else if (property == CSSPropertyWebkitMaskSize) {
          if (!CSSPropertyBackgroundUtils::ConsumeSizeProperty(
                  range, context.Mode(), parsed_longhand[i - 1], value)) {
            return false;
          }
          if (!value)
            continue;
        } else if (property == CSSPropertyWebkitMaskPositionY ||
                   property == CSSPropertyWebkitMaskRepeatY) {
          continue;
        } else {
          value = ConsumeWebkitMaskComponent(property, range, context);
        }
        if (value) {
          if (property == CSSPropertyWebkitMaskOrigin)
            origin_value = value;
          found_property = true;
          CSSPropertyBackgroundUtils::BackgroundValuePostProcessing(
              value, value_y, longhands, parsed_longhand, longhand_count, i);
        }
      }
      if (!found_property)
        return false;
    } while (!range.AtEnd() && range.Peek().GetType() != kCommaToken);

    // TODO(timloh): This will make invalid longhands, see crbug.com/386459
    for (size_t i = 0; i < longhand_count; ++i) {
      CSSPropertyID property = shorthand.properties()[i];
      if (property == CSSPropertyWebkitMaskClip && !parsed_longhand[i] &&
          origin_value) {
        CSSPropertyBackgroundUtils::AddBackgroundValue(longhands[i],
                                                       origin_value);
        continue;
      }
      if (!parsed_longhand[i]) {
        CSSPropertyBackgroundUtils::AddBackgroundValue(
            longhands[i], CSSInitialValue::Create());
      }
    }
  } while (CSSPropertyParserHelpers::ConsumeCommaIncludingWhitespace(range));
  if (!range.AtEnd())
    return false;

  for (size_t i = 0; i < longhand_count; ++i) {
    CSSPropertyID property = shorthand.properties()[i];
    CSSPropertyParserHelpers::AddProperty(
        property, shorthand.id(), *longhands[i], important,
        implicit ? CSSPropertyParserHelpers::IsImplicitProperty::kImplicit
                 : CSSPropertyParserHelpers::IsImplicitProperty::kNotImplicit,
        properties);
  }
  return true;
}

}  // namespace blink
