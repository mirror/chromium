// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ComputedAXTree_h
#define ComputedAXTree_h

#include <string>
#include "public/platform/AOMEnums.h"

namespace blink {

class WebFrame;

class ComputedAXTree {
 public:
  virtual ~ComputedAXTree() = default;
  virtual bool ComputeAccessibilityTree(WebFrame*) = 0;
  virtual const std::string GetNameForAXNode(int32_t axID) = 0;
  virtual const std::string GetRoleForAXNode(int32_t axID) = 0;

  // Get the specified attribute for |axID|, returning true if the node is
  // present in the tree, and has the attribute |int_attr|. Stores this
  // attribute value in |out_param|.
  virtual bool GetIntAttributeForId(int32_t axID,
                                    AOMIntAttribute,
                                    int32_t* out_param) = 0;
};

}  // namespace blink

#endif  // ComputedAXTree_h
