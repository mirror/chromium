// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/properties/longhands/BorderTopColor.h"

#include "core/css/CSSColorValue.h"
#include "core/css/properties/ComputedStyleUtils.h"
#include "core/style/ComputedStyle.h"

namespace blink {

class CSSParserContext;
class CSSParserLocalContext;
class CSSParserTokenRange;

namespace CSSLonghand {

const CSSValue* BorderTopColor::ParseSingleValue(
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    const CSSParserLocalContext& local_context) const {
  return ComputedStyleUtils::ParseBorderColorSide(range, context,
                                                  local_context);
}

const blink::Color BorderTopColor::ColorIncludingFallback(
    bool visited_link,
    const ComputedStyle& style) const {
  StyleColor result =
      visited_link ? style.VisitedLinkBorderTopColor() : style.BorderTopColor();
  EBorderStyle border_style = style.BorderTopStyle();
  if (!result.IsCurrentColor())
    return result.GetColor();
  // FIXME: Treating styled borders with initial color differently causes
  // problems, see crbug.com/316559, crbug.com/276231
  if (!visited_link && (border_style == EBorderStyle::kInset ||
                        border_style == EBorderStyle::kOutset ||
                        border_style == EBorderStyle::kRidge ||
                        border_style == EBorderStyle::kGroove))
    return blink::Color(238, 238, 238);
  return visited_link ? style.VisitedLinkColor() : style.GetColor();
}

const CSSValue* BorderTopColor::CSSValueFromComputedStyle(
    const ComputedStyle& style,
    const LayoutObject* layout_object,
    Node* styled_node,
    bool allow_visited_style) const {
  return allow_visited_style
             ? cssvalue::CSSColorValue::Create(
                   style.VisitedDependentColor(*this).Rgb())
             : ComputedStyleUtils::ComputedStyleUtils::CurrentColorOrValidColor(
                   style, style.BorderTopColor());
}

}  // namespace CSSLonghand
}  // namespace blink
