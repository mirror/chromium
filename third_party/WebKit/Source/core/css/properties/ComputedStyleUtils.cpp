// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/properties/ComputedStyleUtils.h"

#include "core/css/CSSColorValue.h"
#include "core/css/CSSValue.h"
#include "core/css/StyleColor.h"
#include "core/style/ComputedStyle.h"

// TODO(rjwright): move these
#include "core/css/parser/CSSParserContext.h"
#include "core/css/parser/CSSParserLocalContext.h"
#include "core/css/parser/CSSPropertyParserHelpers.h"

namespace blink {

const CSSValue* ComputedStyleUtils::CurrentColorOrValidColor(
    const ComputedStyle& style,
    const StyleColor& color) {
  // This function does NOT look at visited information, so that computed style
  // doesn't expose that.
  return cssvalue::CSSColorValue::Create(color.Resolve(style.GetColor()).Rgb());
}

// TODO(rjwright): Move this
const CSSValue* ComputedStyleUtils::ParseBorderColorSide(
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    const CSSParserLocalContext& local_context) {
  CSSPropertyID shorthand = local_context.CurrentShorthand();
  bool allow_quirky_colors =
      IsQuirksModeBehavior(context.Mode()) &&
      (shorthand == CSSPropertyInvalid || shorthand == CSSPropertyBorderColor);
  return CSSPropertyParserHelpers::ConsumeColor(range, context.Mode(),
                                                allow_quirky_colors);
}

}  // namespace blink
