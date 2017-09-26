// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CSSPropertyAPIApplyAtRule_h
#define CSSPropertyAPIApplyAtRule_h

#include "build/build_config.h"
#include "core/css/properties/CSSPropertyAPI.h"
#include "platform/RuntimeEnabledFeatures.h"

namespace blink {

class CSSPropertyAPIApplyAtRule : public CSSPropertyAPI {
 public:
#if !defined(OS_WIN) || !defined(COMPILER_MSVC)
  constexpr CSSPropertyAPIApplyAtRule(CSSPropertyID id) : CSSPropertyAPI(id) {}
#else
  CSSPropertyAPIApplyAtRule(CSSPropertyID id) : CSSPropertyAPI(id) {}
#endif

  bool IsEnabled() const override {
    return RuntimeEnabledFeatures::CSSApplyAtRulesEnabled();
  }
};

}  // namespace blink

#endif  // CSSPropertyAPIApplyAtRule_h
