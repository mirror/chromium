// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/properties/longhands/CaretColor.h"

#include "core/css/CSSColorValue.h"
#include "core/css/parser/CSSParserContext.h"
#include "core/css/parser/CSSPropertyParserHelpers.h"
#include "core/style/ComputedStyle.h"

namespace blink {
namespace CSSLonghand {

const CSSValue* CaretColor::ParseSingleValue(
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    const CSSParserLocalContext&) const {
  if (range.Peek().Id() == CSSValueAuto)
    return CSSPropertyParserHelpers::ConsumeIdent(range);
  return CSSPropertyParserHelpers::ConsumeColor(range, context.Mode());
}

const CSSValue* CaretColor::CSSValueFromComputedStyle(
    const ComputedStyle& style,
    const LayoutObject* layout_object,
    Node* styled_node,
    bool allow_visited_style) const {
  Color color;
  if (allow_visited_style)
    color = style.VisitedDependentColor(PropertyID());
  else if (style.CaretColor().IsAutoColor())
    color = StyleColor::CurrentColor().Resolve(style.GetColor());
  else
    color = style.CaretColor().ToStyleColor().Resolve(style.GetColor());
  return cssvalue::CSSColorValue::Create(color.Rgb());
}

}  // namespace CSSLonghand
}  // namespace blink
