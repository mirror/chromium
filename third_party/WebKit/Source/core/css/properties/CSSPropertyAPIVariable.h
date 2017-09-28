// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This header is hand-written, whereas most CSSPropertyAPI subclass
// headers are generate by
// core/css/properties/templates/CSSPropertyAPISubclass.h.tmpl. The
// typical CSSPropertyAPI header is generated from an object in
// core/css/CSSProperties.json5, but adding an entry there for
// CSSPropertyAPIVariable causes problems elsewhere.
// CSSPropertyAPIVariable is not really a CSS property, but it is
// convient to treat it as one some of the time. The same is true of
// CSSPropertyApplyAtRule, which also has a hand-written
// CSSPropertyAPI header.
#ifndef CSSPropertyAPIVariable_h
#define CSSPropertyAPIVariable_h

#include "core/css/properties/CSSPropertyAPI.h"

namespace blink {

class CSSPropertyAPIVariable : public CSSPropertyAPI {
 public:
  bool IsInherited() const override { return true; }
  bool IsAffectedByAll() const override { return false; }
};

}  // namespace blink

#endif  // CSSPropertyAPIVariable_h
