// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/properties/CSSPropertyAnimationUtils.h"
#include "core/StylePropertyShorthand.h"
#include "core/css/CSSInitialValue.h"
#include "core/css/parser/CSSPropertyParserHelpers.h"
#include "core/css/properties/CSSPropertyAnimationIterationCountUtils.h"
#include "core/css/properties/CSSPropertyAnimationNameUtils.h"
#include "core/css/properties/CSSPropertyAnimationTimingFunctionUtils.h"
#include "core/css/properties/CSSPropertyTransitionPropertyUtils.h"

namespace blink {

namespace {

CSSValue* ConsumeAnimationValue(CSSPropertyID property,
                                CSSParserTokenRange& range,
                                const CSSParserContext& context,
                                bool use_legacy_parsing) {
  switch (property) {
    case CSSPropertyAnimationDelay:
      return CSSPropertyParserHelpers::ConsumeTime(range, kValueRangeAll);
    case CSSPropertyAnimationDirection:
      return CSSPropertyParserHelpers::ConsumeIdent<
          CSSValueNormal, CSSValueAlternate, CSSValueReverse,
          CSSValueAlternateReverse>(range);
    case CSSPropertyAnimationDuration:
      return CSSPropertyParserHelpers::ConsumeTime(range,
                                                   kValueRangeNonNegative);
    case CSSPropertyAnimationFillMode:
      return CSSPropertyParserHelpers::ConsumeIdent<
          CSSValueNone, CSSValueForwards, CSSValueBackwards, CSSValueBoth>(
          range);
    case CSSPropertyAnimationIterationCount:
      return CSSPropertyAnimationIterationCountUtils::
          ConsumeAnimationIterationCount(range);
    case CSSPropertyAnimationName:
      return CSSPropertyAnimationNameUtils::ConsumeAnimationName(
          range, &context, use_legacy_parsing);
    case CSSPropertyAnimationPlayState:
      return CSSPropertyParserHelpers::ConsumeIdent<CSSValueRunning,
                                                    CSSValuePaused>(range);
    case CSSPropertyAnimationTimingFunction:
      return CSSPropertyAnimationTimingFunctionUtils::
          ConsumeAnimationTimingFunction(range);
    default:
      NOTREACHED();
      return nullptr;
  }
}

CSSValue* ConsumeTransitionValue(CSSPropertyID property,
                                 CSSParserTokenRange& range) {
  switch (property) {
    case CSSPropertyTransitionDelay:
      return CSSPropertyParserHelpers::ConsumeTime(range, kValueRangeAll);
    case CSSPropertyTransitionDuration:
      return CSSPropertyParserHelpers::ConsumeTime(range,
                                                   kValueRangeNonNegative);
    case CSSPropertyTransitionProperty:
      return CSSPropertyTransitionPropertyUtils::ConsumeTransitionProperty(
          range);
    case CSSPropertyTransitionTimingFunction:
      return CSSPropertyAnimationTimingFunctionUtils::
          ConsumeAnimationTimingFunction(range);
    default:
      NOTREACHED();
      return nullptr;
  }
}

}  // namespace

bool CSSPropertyAnimationUtils::ConsumeShorthand(
    const StylePropertyShorthand& shorthand,
    bool is_transition,
    bool use_legacy_parsing,
    bool important,
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    HeapVector<CSSProperty, 256>& properties) {
  const unsigned longhand_count = shorthand.length();
  CSSValueList* longhands[8];
  DCHECK_LE(longhand_count, 8u);
  for (size_t i = 0; i < longhand_count; ++i)
    longhands[i] = CSSValueList::CreateCommaSeparated();

  do {
    bool parsed_longhand[8] = {false};
    do {
      bool found_property = false;
      for (size_t i = 0; i < longhand_count; ++i) {
        if (parsed_longhand[i])
          continue;

        CSSValue* value =
            is_transition
                ? ConsumeTransitionValue(shorthand.properties()[i], range)
                : ConsumeAnimationValue(shorthand.properties()[i], range,
                                        context, use_legacy_parsing);
        if (value) {
          parsed_longhand[i] = true;
          found_property = true;
          longhands[i]->Append(*value);
          break;
        }
      }
      if (!found_property)
        return false;
    } while (!range.AtEnd() && range.Peek().GetType() != kCommaToken);

    // TODO(timloh): This will make invalid longhands, see crbug.com/386459
    for (size_t i = 0; i < longhand_count; ++i) {
      if (!parsed_longhand[i])
        longhands[i]->Append(*CSSInitialValue::Create());
      parsed_longhand[i] = false;
    }
  } while (CSSPropertyParserHelpers::ConsumeCommaIncludingWhitespace(range));

  if (is_transition) {
    for (size_t i = 0; i < longhand_count; ++i) {
      if (shorthand.properties()[i] == CSSPropertyTransitionProperty &&
          !CSSPropertyTransitionPropertyUtils::IsValidPropertyList(
              *longhands[i]))
        return false;
    }
  }

  for (size_t i = 0; i < longhand_count; ++i) {
    CSSPropertyParserHelpers::AddProperty(
        shorthand.properties()[i], shorthand.id(), *longhands[i], important,
        CSSPropertyParserHelpers::IsImplicitProperty::kNotImplicit, properties);
  }
  return range.AtEnd();
}

}  // namespace blink
