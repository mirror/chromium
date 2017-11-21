// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/properties/longhands/BorderBottomColor.h"

#include "core/CSSPropertyNames.h"
#include "core/css/CSSColorValue.h"
#include "core/css/properties/ComputedStyleUtils.h"
#include "core/style/ComputedStyle.h"

namespace blink {

class CSSParserContext;
class CSSParserLocalContext;
class CSSParserTokenRange;

namespace CSSLonghand {

const CSSValue* BorderBottomColor::ParseSingleValue(
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    const CSSParserLocalContext& local_context) const {
  return ComputedStyleUtils::ParseBorderColorSide(range, context,
                                                  local_context);
}

const CSSValue* BorderBottomColor::CSSValueFromComputedStyle(
    const ComputedStyle& style,
    const LayoutObject* layout_object,
    Node* styled_node,
    bool allow_visited_style) const {
  return allow_visited_style
             ? cssvalue::CSSColorValue::Create(
                   style.VisitedDependentColor(PropertyID()).Rgb())
             : ComputedStyleUtils::CurrentColorOrValidColor(
                   style, style.BorderBottomColor());
}

}  // namespace CSSLonghand
}  // namespace blink
