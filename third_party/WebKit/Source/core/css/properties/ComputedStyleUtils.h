// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ComputedStyleUtils_h
#define ComputedStyleUtils_h

#include "platform/wtf/Allocator.h"

namespace blink {

class ComputedStyle;
class CSSValue;
class StyleColor;

class CSSParserContext;
class CSSParserLocalContext;
class CSSParserTokenRange;

class ComputedStyleUtils {
  STATIC_ONLY(ComputedStyleUtils);

 public:
  static const CSSValue* CurrentColorOrValidColor(const ComputedStyle&,
                                                  const StyleColor&);
  // TODO(rjwright): Move this
  static const CSSValue* ParseBorderColorSide(CSSParserTokenRange&,
                                              const CSSParserContext&,
                                              const CSSParserLocalContext&);
};

}  // namespace blink

#endif  // ComputedStyleUtils_h
