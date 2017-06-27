// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/properties/CSSShorthandPropertyAPITransition.h"

#include "core/StylePropertyShorthand.h"
#include "core/css/CSSIdentifierValue.h"
#include "core/css/parser/CSSParserContext.h"
#include "core/css/parser/CSSParserLocalContext.h"
#include "core/css/parser/CSSPropertyParserHelpers.h"
#include "core/css/properties/CSSPropertyAnimationUtils.h"

namespace blink {

bool CSSShorthandPropertyAPITransition::parseShorthand(
    bool important,
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    const CSSParserLocalContext&,
    HeapVector<CSSProperty, 256>& properties) {
  bool is_transition = true;
  return CSSPropertyAnimationUtils::ConsumeShorthand(
      transitionShorthandForParsing(), is_transition, false, important, range,
      context, properties);
}

}  // namespace blink
