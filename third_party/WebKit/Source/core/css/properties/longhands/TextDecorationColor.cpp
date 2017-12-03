// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/properties/longhands/TextDecorationColor.h"

#include "core/css/parser/CSSParserContext.h"
#include "core/css/parser/CSSPropertyParserHelpers.h"
#include "core/css/properties/ComputedStyleUtils.h"
#include "core/style/ComputedStyle.h"

namespace blink {
namespace CSSLonghand {

const CSSValue* TextDecorationColor::ParseSingleValue(
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    const CSSParserLocalContext&) const {
  return CSSPropertyParserHelpers::ConsumeColor(range, context.Mode());
}

const blink::Color TextDecorationColor::ColorIncludingFallback(
    bool visited_link,
    const ComputedStyle& style) const {
  StyleColor result = visited_link
                          ? style.TextDecorationColorIgnoringUnvisited()
                          : style.TextDecorationColor();

  if (result.IsCurrentColor()) {
    result = visited_link ? style.TextFillColorIgnoringUnvisited()
                          : style.TextFillColorIgnoringVisited();
    if (style.TextStrokeWidth()) {
      // Prefer stroke color if possible, but not if it's fully transparent.
      StyleColor text_stroke_style_color =
          visited_link ? style.TextStrokeColorIgnoringUnvisited()
                       : style.TextStrokeColorIgnoringVisited();
      if (!text_stroke_style_color.IsCurrentColor() &&
          text_stroke_style_color.GetColor().Alpha())
        result = text_stroke_style_color;
    }
  }

  if (!result.IsCurrentColor())
    return result.GetColor();
  return visited_link ? style.VisitedLinkColor() : style.ColorIgnoringVisited();
}

const CSSValue* TextDecorationColor::CSSValueFromComputedStyle(
    const ComputedStyle& style,
    const LayoutObject* layout_object,
    Node* styled_node,
    bool allow_visited_style) const {
  return ComputedStyleUtils::CurrentColorOrValidColor(
      style, style.TextDecorationColor());
}

}  // namespace CSSLonghand
}  // namespace blink
