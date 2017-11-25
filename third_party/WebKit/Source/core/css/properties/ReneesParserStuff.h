// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ReneesParserStuff_h
#define ReneesParserStuff_h

#include "platform/wtf/Allocator.h"

namespace blink {

class CSSParserContext;
class CSSParserLocalContext;
class CSSParserTokenRange;
class CSSValue;

class ReneesParserStuff {
  STATIC_ONLY(ReneesParserStuff);

 public:
  // TODO(rjwright): Move this
  static const CSSValue* ParseBorderColorSide(CSSParserTokenRange&,
                                              const CSSParserContext&,
                                              const CSSParserLocalContext&);
};

}  // namespace blink

#endif  // ReneesParserStuff_h
