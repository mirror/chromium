// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PlatformAXTree_h
#define PlatformAXTree_h

#include "third_party/WebKit/public/platform/PlatformAXNode.h"

namespace blink {

class PlatformAXTree {
 public:
  ~PlatformAXTree() = default;

  virtual bool RequestTreeSnapshot() = 0;
  virtual PlatformAXNode* GetAXNode(int32_t) = 0;
};

}  // namespace blink

#endif  // PlatformAXTree_h
