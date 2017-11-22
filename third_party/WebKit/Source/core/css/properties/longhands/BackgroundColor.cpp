// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/properties/longhands/BackgroundColor.h"

#include "core/css/CSSColorValue.h"
#include "core/css/parser/CSSParserContext.h"
#include "core/css/parser/CSSParserMode.h"
#include "core/css/parser/CSSPropertyParserHelpers.h"
#include "core/css/properties/ComputedStyleUtils.h"
#include "core/style/ComputedStyle.h"

namespace blink {
namespace CSSLonghand {

const CSSValue* BackgroundColor::ParseSingleValue(
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    const CSSParserLocalContext&) const {
  return CSSPropertyParserHelpers::ConsumeColor(
      range, context.Mode(), IsQuirksModeBehavior(context.Mode()));
}

const CSSValue* BackgroundColor::CSSValueFromComputedStyle(
    const ComputedStyle& style,
    const LayoutObject* layout_object,
    Node* styled_node,
    bool allow_visited_style) const {
  return allow_visited_style
             ? cssvalue::CSSColorValue::Create(
                   style.VisitedDependentColor(PropertyID()).Rgb())
             : ComputedStyleUtils::CurrentColorOrValidColor(
                   style, style.BackgroundColor());
}

}  // namespace CSSLonghand
}  // namespace blink
