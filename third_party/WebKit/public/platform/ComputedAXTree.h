// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ComputedAXTree_h
#define ComputedAXTree_h

#include <string>

namespace blink {

class WebFrame;

class ComputedAXTree {
 public:
  virtual ~ComputedAXTree() = default;
  virtual bool ComputeAccessibilityTree(WebFrame*) = 0;
  virtual const std::string GetNameFromId(int32_t) = 0;
  virtual const std::string GetRoleFromId(int32_t) = 0;
};

}  // namespace blink

#endif  // ComputedAXTree_h
