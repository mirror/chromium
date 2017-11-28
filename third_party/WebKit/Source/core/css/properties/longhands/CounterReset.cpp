// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/properties/longhands/CounterReset.h"

#include "core/css/properties/CSSParsingUtils.h"

namespace blink {
namespace CSSLonghand {

const CSSValue* CounterReset::ParseSingleValue(
    CSSParserTokenRange& range,
    const CSSParserContext&,
    const CSSParserLocalContext&) const {
  return CSSParsingUtils::ConsumeCounter(range,
                                         CSSParsingUtils::kResetDefaultValue);
}

}  // namespace CSSLonghand
}  // namespace blink
