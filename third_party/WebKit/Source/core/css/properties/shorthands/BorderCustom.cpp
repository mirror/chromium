// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/properties/shorthands/Border.h"

#include "core/css/CSSInitialValue.h"
#include "core/css/parser/CSSParserContext.h"
#include "core/css/parser/CSSPropertyParserHelpers.h"
#include "core/style/ComputedStyle.h"

namespace blink {
namespace CSSShorthand {

bool Border::ParseShorthand(
    bool important,
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    const CSSParserLocalContext&,
    HeapVector<CSSPropertyValue, 256>& properties) const {
  CSSValue* width = nullptr;
  const CSSValue* style = nullptr;
  CSSValue* color = nullptr;

  while (!width || !style || !color) {
    if (!width) {
      width = CSSPropertyParserHelpers::ConsumeLineWidth(
          range, context.Mode(),
          CSSPropertyParserHelpers::UnitlessQuirk::kForbid);
      if (width)
        continue;
    }
    if (!style) {
      bool needs_legacy_parsing = false;
      style = CSSPropertyParserHelpers::ParseLonghand(
          CSSPropertyBorderLeftStyle, CSSPropertyBorder, context, range);
      DCHECK(!needs_legacy_parsing);
      if (style)
        continue;
    }
    if (!color) {
      color = CSSPropertyParserHelpers::ConsumeColor(range, context.Mode());
      if (color)
        continue;
    }
    break;
  }

  if (!width && !style && !color)
    return false;

  if (!width)
    width = CSSInitialValue::Create();
  if (!style)
    style = CSSInitialValue::Create();
  if (!color)
    color = CSSInitialValue::Create();

  CSSPropertyParserHelpers::AddExpandedPropertyForValue(
      CSSPropertyBorderWidth, *width, important, properties);
  CSSPropertyParserHelpers::AddExpandedPropertyForValue(
      CSSPropertyBorderStyle, *style, important, properties);
  CSSPropertyParserHelpers::AddExpandedPropertyForValue(
      CSSPropertyBorderColor, *color, important, properties);
  CSSPropertyParserHelpers::AddExpandedPropertyForValue(
      CSSPropertyBorderImage, *CSSInitialValue::Create(), important,
      properties);

  return range.AtEnd();
}

const CSSValue* Border::CSSValueFromComputedStyleInternal(
    const ComputedStyle& style,
    const SVGComputedStyle&,
    const LayoutObject*,
    Node* styled_node,
    bool allow_visited_style) const {
  const CSSValue* value = Get(GetCSSPropertyBorderTop(), style, layout_object,
                              styled_node, allow_visited_style);
  static const CSSProperty* kProperties[3] = {&GetCSSPropertyBorderRight(),
                                              &GetCSSPropertyBorderBottom(),
                                              &GetCSSPropertyBorderLeft()};
  for (size_t i = 0; i < WTF_ARRAY_LENGTH(kProperties); ++i) {
    if (!DataEquivalent(value, Get(*kProperties[i], style, layout_object,
                                   styled_node, allow_visited_style)))
      return nullptr;
  }
  return value;
}

}  // namespace CSSShorthand
}  // namespace blink
