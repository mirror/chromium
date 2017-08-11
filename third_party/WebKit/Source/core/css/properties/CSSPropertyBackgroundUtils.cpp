// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/properties/CSSPropertyBackgroundUtils.h"

#include "core/CSSValueKeywords.h"
#include "core/css/CSSTimingFunctionValue.h"
#include "core/css/CSSValueList.h"
#include "core/css/CSSValuePair.h"
#include "core/css/parser/CSSParserTokenRange.h"
#include "core/css/parser/CSSPropertyParserHelpers.h"
#include "platform/RuntimeEnabledFeatures.h"

namespace blink {

void CSSPropertyBackgroundUtils::AddBackgroundValue(CSSValue*& list,
                                                    CSSValue* value) {
  if (list) {
    if (!list->IsBaseValueList()) {
      CSSValue* first_value = list;
      list = CSSValueList::CreateCommaSeparated();
      ToCSSValueList(list)->Append(*first_value);
    }
    ToCSSValueList(list)->Append(*value);
  } else {
    // To conserve memory we don't actually wrap a single value in a list.
    list = value;
  }
}

CSSValue* CSSPropertyBackgroundUtils::ConsumeBackgroundAttachment(
    CSSParserTokenRange& range) {
  return CSSPropertyParserHelpers::ConsumeIdent<CSSValueScroll, CSSValueFixed,
                                                CSSValueLocal>(range);
}

CSSValue* CSSPropertyBackgroundUtils::ConsumeBackgroundBlendMode(
    CSSParserTokenRange& range) {
  CSSValueID id = range.Peek().Id();
  if (id == CSSValueNormal || id == CSSValueOverlay ||
      (id >= CSSValueMultiply && id <= CSSValueLuminosity))
    return CSSPropertyParserHelpers::ConsumeIdent(range);
  return nullptr;
}

CSSValue* CSSPropertyBackgroundUtils::ConsumeBackgroundBox(
    CSSParserTokenRange& range) {
  return CSSPropertyParserHelpers::ConsumeIdent<
      CSSValueBorderBox, CSSValuePaddingBox, CSSValueContentBox>(range);
}

CSSValue* CSSPropertyBackgroundUtils::ConsumeBackgroundComposite(
    CSSParserTokenRange& range) {
  return CSSPropertyParserHelpers::ConsumeIdentRange(range, CSSValueClear,
                                                     CSSValuePlusLighter);
}

CSSValue* CSSPropertyBackgroundUtils::ConsumeMaskSourceType(
    CSSParserTokenRange& range) {
  DCHECK(RuntimeEnabledFeatures::CSSMaskSourceTypeEnabled());
  return CSSPropertyParserHelpers::ConsumeIdent<CSSValueAuto, CSSValueAlpha,
                                                CSSValueLuminance>(range);
}

bool CSSPropertyBackgroundUtils::ConsumeBackgroundPosition(
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    CSSPropertyParserHelpers::UnitlessQuirk unitless,
    CSSValue*& result_x,
    CSSValue*& result_y) {
  do {
    CSSValue* position_x = nullptr;
    CSSValue* position_y = nullptr;
    if (!CSSPropertyParserHelpers::ConsumePosition(
            range, context, unitless,
            WebFeature::kThreeValuedPositionBackground, position_x, position_y))
      return false;
    AddBackgroundValue(result_x, position_x);
    AddBackgroundValue(result_y, position_y);
  } while (CSSPropertyParserHelpers::ConsumeCommaIncludingWhitespace(range));
  return true;
}

CSSValue* CSSPropertyBackgroundUtils::ConsumePrefixedBackgroundBox(
    CSSParserTokenRange& range,
    const CSSParserContext* context,
    bool allow_text_value) {
  // The values 'border', 'padding' and 'content' are deprecated and do not
  // apply to the version of the property that has the -webkit- prefix removed.
  if (CSSValue* value = CSSPropertyParserHelpers::ConsumeIdentRange(
          range, CSSValueBorder, CSSValuePaddingBox))
    return value;
  if (allow_text_value && range.Peek().Id() == CSSValueText)
    return CSSPropertyParserHelpers::ConsumeIdent(range);
  return nullptr;
}

CSSValue* CSSPropertyBackgroundUtils::ConsumeBackgroundSize(
    CSSParserTokenRange& range,
    CSSParserMode css_parser_mode,
    bool use_legacy_parsing) {
  if (CSSPropertyParserHelpers::IdentMatches<CSSValueContain, CSSValueCover>(
          range.Peek().Id()))
    return CSSPropertyParserHelpers::ConsumeIdent(range);

  CSSValue* horizontal =
      CSSPropertyParserHelpers::ConsumeIdent<CSSValueAuto>(range);
  if (!horizontal) {
    horizontal = CSSPropertyParserHelpers::ConsumeLengthOrPercent(
        range, css_parser_mode, kValueRangeAll,
        CSSPropertyParserHelpers::UnitlessQuirk::kForbid);
  }

  CSSValue* vertical = nullptr;
  if (!range.AtEnd()) {
    if (range.Peek().Id() == CSSValueAuto)  // `auto' is the default
    {
      range.ConsumeIncludingWhitespace();
    } else {
      vertical = CSSPropertyParserHelpers::ConsumeLengthOrPercent(
          range, css_parser_mode, kValueRangeAll,
          CSSPropertyParserHelpers::UnitlessQuirk::kForbid);
    }
  } else if (use_legacy_parsing) {
    // Legacy syntax: "-webkit-background-size: 10px" is equivalent to
    // "background-size: 10px 10px".
    vertical = horizontal;
  }
  if (!vertical)
    return horizontal;
  return CSSValuePair::Create(horizontal, vertical,
                              CSSValuePair::kKeepIdenticalValues);
}

bool CSSPropertyBackgroundUtils::ConsumeRepeatStyleComponent(
    CSSParserTokenRange& range,
    CSSValue*& value1,
    CSSValue*& value2,
    bool& implicit) {
  if (CSSPropertyParserHelpers::ConsumeIdent<CSSValueRepeatX>(range)) {
    value1 = CSSIdentifierValue::Create(CSSValueRepeat);
    value2 = CSSIdentifierValue::Create(CSSValueNoRepeat);
    implicit = true;
    return true;
  }
  if (CSSPropertyParserHelpers::ConsumeIdent<CSSValueRepeatY>(range)) {
    value1 = CSSIdentifierValue::Create(CSSValueNoRepeat);
    value2 = CSSIdentifierValue::Create(CSSValueRepeat);
    implicit = true;
    return true;
  }
  value1 = CSSPropertyParserHelpers::ConsumeIdent<
      CSSValueRepeat, CSSValueNoRepeat, CSSValueRound, CSSValueSpace>(range);
  if (!value1)
    return false;

  value2 = CSSPropertyParserHelpers::ConsumeIdent<
      CSSValueRepeat, CSSValueNoRepeat, CSSValueRound, CSSValueSpace>(range);
  if (!value2) {
    value2 = value1;
    implicit = true;
  }
  return true;
}

bool CSSPropertyBackgroundUtils::ConsumeRepeatStyle(CSSParserTokenRange& range,
                                                    CSSValue*& result_x,
                                                    CSSValue*& result_y,
                                                    bool& implicit) {
  do {
    CSSValue* repeat_x = nullptr;
    CSSValue* repeat_y = nullptr;
    if (!ConsumeRepeatStyleComponent(range, repeat_x, repeat_y, implicit))
      return false;
    AddBackgroundValue(result_x, repeat_x);
    AddBackgroundValue(result_y, repeat_y);
  } while (CSSPropertyParserHelpers::ConsumeCommaIncludingWhitespace(range));
  return true;
}

bool CSSPropertyBackgroundUtils::ConsumeSizeProperty(
    CSSParserTokenRange& range,
    CSSParserMode css_parser_mode,
    bool is_prev_pos_parsed,
    CSSValue*& value) {
  DCHECK(!value);
  if (!CSSPropertyParserHelpers::ConsumeSlashIncludingWhitespace(range))
    return true;  // so caller can continue

  value = ConsumeBackgroundSize(range, css_parser_mode,
                                false /* use_legacy_parsing */);
  // If is_prev_pos_parsed is false, Position must have been parsed in the
  // current layer.
  return value && is_prev_pos_parsed;
}

void CSSPropertyBackgroundUtils::BackgroundValuePostProcessing(
    CSSValue* value,
    CSSValue* value_y,
    CSSValue** longhands,
    bool* parsed_longhand,
    size_t longhands_sz,
    size_t longhand_idx) {
  DCHECK_LT(longhand_idx, longhands_sz);

  parsed_longhand[longhand_idx] = true;
  AddBackgroundValue(longhands[longhand_idx], value);
  if (value_y) {
    DCHECK_LT(longhand_idx + 1, longhands_sz);
    parsed_longhand[longhand_idx + 1] = true;
    AddBackgroundValue(longhands[longhand_idx + 1], value_y);
  }
}

}  // namespace blink
