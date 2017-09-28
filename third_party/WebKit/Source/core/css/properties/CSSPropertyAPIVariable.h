// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CSSPropertyAPIVariable_h
#define CSSPropertyAPIVariable_h

#include "build/build_config.h"
#include "core/css/properties/CSSPropertyAPI.h"
#include "platform/runtime_enabled_features.h"

namespace blink {

class CSSPropertyAPIVariable : public CSSPropertyAPI {
 public:
#if !defined(OS_WIN) || !defined(COMPILER_MSVC)
  constexpr CSSPropertyAPIVariable(CSSPropertyID id) : CSSPropertyAPI(id) {}
#else
  CSSPropertyAPIVariable(CSSPropertyID id) : CSSPropertyAPI(id) {}
#endif

  bool IsInherited() const override { return true; }
  bool IsAffectedByAll() const override { return false; }
};

}  // namespace blink

#endif  // CSSPropertyAPIVariable_h
