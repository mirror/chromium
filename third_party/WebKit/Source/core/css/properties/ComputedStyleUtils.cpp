// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/properties/ComputedStyleUtils.h"

#include "core/css/CSSColorValue.h"
#include "core/css/CSSIdentifierValue.h"
#include "core/css/CSSValue.h"
#include "core/css/CSSValueList.h"
#include "core/css/StyleColor.h"
#include "core/css/ZoomAdjustedPixelValue.h"

namespace blink {

CSSValue* ComputedStyleUtils::ValueForPosition(const LengthPoint& position,
                                               const ComputedStyle& style) {
  DCHECK_EQ(position.X().IsAuto(), position.Y().IsAuto());
  if (position.X().IsAuto())
    return CSSIdentifierValue::Create(CSSValueAuto);

  CSSValueList* list = CSSValueList::CreateSpaceSeparated();
  list->Append(*ComputedStyleUtils::ZoomAdjustedPixelValueForLength(
      position.X(), style));
  list->Append(*ComputedStyleUtils::ZoomAdjustedPixelValueForLength(
      position.Y(), style));
  return list;
}

CSSValue* ComputedStyleUtils::ValueForOffset(const ComputedStyle& style,
                                             const LayoutObject* layout_object,
                                             Node* styled_node,
                                             bool allow_visited_style) {
  CSSValueList* list = CSSValueList::CreateSpaceSeparated();
  if (RuntimeEnabledFeatures::CSSOffsetPositionAnchorEnabled()) {
    CSSValue* position = ValueForPosition(style.OffsetPosition(), style);
    if (!position->IsIdentifierValue())
      list->Append(*position);
    else
      DCHECK(ToCSSIdentifierValue(position)->GetValueID() == CSSValueAuto);
  }

  static const CSSProperty* longhands[3] = {&GetCSSPropertyOffsetPath(),
                                            &GetCSSPropertyOffsetDistance(),
                                            &GetCSSPropertyOffsetRotate()};
  for (const CSSProperty* longhand : longhands) {
    const CSSValue* value = longhand->CSSValueFromComputedStyle(
        style, layout_object, styled_node, allow_visited_style);
    DCHECK(value);
    list->Append(*value);
  }

  if (RuntimeEnabledFeatures::CSSOffsetPositionAnchorEnabled()) {
    CSSValue* anchor = ValueForPosition(style.OffsetAnchor(), style);
    if (!anchor->IsIdentifierValue()) {
      // Add a slash before anchor.
      CSSValueList* result = CSSValueList::CreateSlashSeparated();
      result->Append(*list);
      result->Append(*anchor);
      return result;
    }
    DCHECK(ToCSSIdentifierValue(anchor)->GetValueID() == CSSValueAuto);
  }
  return list;
}

CSSValue* ComputedStyleUtils::CurrentColorOrValidColor(
    const ComputedStyle& style,
    const StyleColor& color) {
  // This function does NOT look at visited information, so that computed style
  // doesn't expose that.
  return cssvalue::CSSColorValue::Create(color.Resolve(style.GetColor()).Rgb());
}

const blink::Color ComputedStyleUtils::BorderSideColor(
    const ComputedStyle& style,
    const StyleColor& color,
    EBorderStyle border_style,
    bool visited_link) {
  if (!color.IsCurrentColor())
    return color.GetColor();
  // FIXME: Treating styled borders with initial color differently causes
  // problems, see crbug.com/316559, crbug.com/276231
  if (!visited_link && (border_style == EBorderStyle::kInset ||
                        border_style == EBorderStyle::kOutset ||
                        border_style == EBorderStyle::kRidge ||
                        border_style == EBorderStyle::kGroove))
    return blink::Color(238, 238, 238);
  return visited_link ? style.VisitedLinkColor() : style.GetColor();
}

// TODO(rjwright): make this const
CSSValue* ComputedStyleUtils::ZoomAdjustedPixelValueForLength(
    const Length& length,
    const ComputedStyle& style) {
  if (length.IsFixed())
    return ZoomAdjustedPixelValue(length.Value(), style);
  return CSSValue::Create(length, style.EffectiveZoom());
}

}  // namespace blink
