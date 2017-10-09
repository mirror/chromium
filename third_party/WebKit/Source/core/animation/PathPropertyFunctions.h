// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PathPropertyFunctions_h
#define PathPropertyFunctions_h

#include "core/CSSPropertyNames.h"
#include "core/style/StylePath.h"

namespace blink {

class ComputedStyle;

class PathPropertyFunctions {
 public:
  static StylePath* GetPath(CSSPropertyID, const ComputedStyle&);

  static void SetPath(CSSPropertyID, ComputedStyle&, RefPtr<blink::StylePath>);
};

}  // namespace blink

#endif  // PathPropertyFunctions_h
