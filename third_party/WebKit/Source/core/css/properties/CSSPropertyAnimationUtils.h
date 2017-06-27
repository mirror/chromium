// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CSSPropertyAnimationUtils_h
#define CSSPropertyAnimationUtils_h

#include "platform/heap/HeapAllocator.h"
#include "platform/wtf/Allocator.h"

namespace blink {

class CSSParserContext;
class CSSParserTokenRange;
class StylePropertyShorthand;

class CSSPropertyAnimationUtils {
  STATIC_ONLY(CSSPropertyAnimationUtils);

 public:
  static bool ConsumeShorthand(const StylePropertyShorthand&,
                               bool is_transition,
                               bool use_legacy_parsing,
                               bool important,
                               CSSParserTokenRange&,
                               const CSSParserContext&,
                               HeapVector<CSSProperty, 256>&);
};

}  // namespace blink

#endif  // CSSPropertyAnimationUtils_h
