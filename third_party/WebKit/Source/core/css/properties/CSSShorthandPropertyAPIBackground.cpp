// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/properties/CSSShorthandPropertyAPIBackground.h"

#include "core/StylePropertyShorthand.h"
#include "core/css/CSSInitialValue.h"
#include "core/css/parser/CSSParserContext.h"
#include "core/css/parser/CSSPropertyParserHelpers.h"
#include "core/css/properties/CSSPropertyBackgroundUtils.h"
#include "core/css/properties/CSSPropertyPositionUtils.h"

namespace blink {

namespace {

CSSValue* ConsumeBackgroundComponent(CSSPropertyID resolved_property,
                                     CSSParserTokenRange& range,
                                     const CSSParserContext& context) {
  switch (resolved_property) {
    case CSSPropertyBackgroundClip:
      return CSSPropertyBackgroundUtils::ConsumeBackgroundBox(range);
    case CSSPropertyBackgroundAttachment:
      return CSSPropertyBackgroundUtils::ConsumeBackgroundAttachment(range);
    case CSSPropertyBackgroundOrigin:
      return CSSPropertyBackgroundUtils::ConsumeBackgroundBox(range);
    case CSSPropertyBackgroundImage:
      return CSSPropertyParserHelpers::ConsumeImageOrNone(range, &context);
    case CSSPropertyBackgroundPositionX:
      return CSSPropertyPositionUtils::ConsumePositionLonghand<CSSValueLeft,
                                                               CSSValueRight>(
          range, context.Mode());
    case CSSPropertyBackgroundPositionY:
      return CSSPropertyPositionUtils::ConsumePositionLonghand<CSSValueTop,
                                                               CSSValueBottom>(
          range, context.Mode());
    case CSSPropertyBackgroundSize:
      return CSSPropertyBackgroundUtils::ConsumeBackgroundSize(
          range, context.Mode(), false /* use_legacy_parsing */);
    case CSSPropertyBackgroundColor:
      return CSSPropertyParserHelpers::ConsumeColor(range, context.Mode());
    default:
      break;
  };
  return nullptr;
}

}  // namespace

// Note: this assumes y properties (e.g. background-position-y) follow the x
// properties in the shorthand array.
bool CSSShorthandPropertyAPIBackground::ParseShorthand(
    bool important,
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    bool use_legacy_parsing,
    HeapVector<CSSProperty, 256>& properties) {
  const StylePropertyShorthand& shorthand = backgroundShorthand();
  const unsigned longhand_count = shorthand.length();
  CSSValue* longhands[10] = {0};
  DCHECK_EQ(longhand_count, 10u);

  bool implicit = false;
  do {
    bool parsed_longhand[10] = {false};
    CSSValue* origin_value = nullptr;
    do {
      bool found_property = false;
      for (size_t i = 0; i < longhand_count; ++i) {
        if (parsed_longhand[i])
          continue;

        CSSValue* value = nullptr;
        CSSValue* value_y = nullptr;
        CSSPropertyID property = shorthand.properties()[i];
        if (property == CSSPropertyBackgroundRepeatX) {
          CSSPropertyBackgroundUtils::ConsumeRepeatStyleComponent(
              range, value, value_y, implicit);
        } else if (property == CSSPropertyBackgroundPositionX) {
          if (!CSSPropertyParserHelpers::ConsumePosition(
                  range, context,
                  CSSPropertyParserHelpers::UnitlessQuirk::kForbid,
                  WebFeature::kThreeValuedPositionBackground, value, value_y))
            continue;
        } else if (property == CSSPropertyBackgroundSize) {
          if (!CSSPropertyBackgroundUtils::ConsumeSizeProperty(
                  range, context.Mode(), parsed_longhand[i - 1], value)) {
            return false;
          }
          if (!value)
            continue;
        } else if (property == CSSPropertyBackgroundPositionY ||
                   property == CSSPropertyBackgroundRepeatY) {
          continue;
        } else {
          value = ConsumeBackgroundComponent(property, range, context);
        }
        if (value) {
          if (property == CSSPropertyBackgroundOrigin)
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
      if (property == CSSPropertyBackgroundColor && !range.AtEnd()) {
        if (parsed_longhand[i])
          return false;  // Colors are only allowed in the last layer.
        continue;
      }
      if (property == CSSPropertyBackgroundClip && !parsed_longhand[i] &&
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
    if (property == CSSPropertyBackgroundSize && longhands[i] &&
        context.UseLegacyBackgroundSizeShorthandBehavior())
      continue;
    CSSPropertyParserHelpers::AddProperty(
        property, shorthand.id(), *longhands[i], important,
        implicit ? CSSPropertyParserHelpers::IsImplicitProperty::kImplicit
                 : CSSPropertyParserHelpers::IsImplicitProperty::kNotImplicit,
        properties);
  }

  return true;
}

}  // namespace blink
