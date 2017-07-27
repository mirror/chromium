// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CSSPropertyAPI_h
#define CSSPropertyAPI_h

#include "core/CSSPropertyNames.h"
#include "platform/heap/HeapAllocator.h"

namespace blink {

class CSSParserTokenRange;
class CSSParserContext;
class CSSParserLocalContext;
class CSSProperty;
class CSSValue;

class CSSPropertyAPI {
 public:
  static const CSSPropertyAPI& Get(CSSPropertyID);

  constexpr CSSPropertyAPI() {}

  virtual const CSSValue* parseSingleValue(CSSPropertyID,
                                           CSSParserTokenRange&,
                                           const CSSParserContext&,
                                           const CSSParserLocalContext&) const;
  virtual bool parseShorthand(CSSPropertyID,
                              bool important,
                              CSSParserTokenRange&,
                              const CSSParserContext&,
                              bool use_legacy_parsing,
                              HeapVector<CSSProperty, 256>&) const;
};

}  // namespace blink

#endif  // CSSPropertyAPI_h
