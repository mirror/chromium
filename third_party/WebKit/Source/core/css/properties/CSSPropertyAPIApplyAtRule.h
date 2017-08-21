// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CSSPropertyAPIApplyAtRule_h
#define CSSPropertyAPIApplyAtRule_h

#include "core/css/properties/CSSPropertyAPI.h"

namespace blink {

class CSSPropertyAPIApplyAtRule : public CSSPropertyAPI {
 public:
  bool IsProperty() const override { return false; }
};

}  // namespace blink

#endif  // CSSPropertyAPIApplyAtRule_h
