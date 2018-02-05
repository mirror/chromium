// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "CanvasContextCreationAttributes.h"

namespace blink {

CanvasContextCreationAttributes::CanvasContextCreationAttributes() {}
CanvasContextCreationAttributes::CanvasContextCreationAttributes(
    blink::CanvasContextCreationAttributes const&) = default;
CanvasContextCreationAttributes::~CanvasContextCreationAttributes() {}

void CanvasContextCreationAttributes::Trace(blink::Visitor* visitor) {
  visitor->Trace(compatiblexr_device_);
}

}  // namespace blink
