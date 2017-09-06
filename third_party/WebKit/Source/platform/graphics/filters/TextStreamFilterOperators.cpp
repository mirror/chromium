// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/graphics/filters/TextStreamFilterOperators.h"

#include "platform/geometry/FloatPoint3D.h"
#include "platform/text/TextStream.h"

namespace blink {

TextStream& operator<<(TextStream& ts, const FloatPoint3D& p) {
  ts << "x=" << p.X() << " y=" << p.Y() << " z=" << p.Z();
  return ts;
}

}  // namespace blink
