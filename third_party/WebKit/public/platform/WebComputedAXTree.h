// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebComputedAXTree_h
#define WebComputedAXTree_h

#include "third_party/WebKit/public/platform/WebString.h"

namespace blink {

enum AOMIntAttribute {
  AOM_INT_ATTRIBUTE_NONE,
  AOM_ATTR_COLUMN_COUNT,
  AOM_ATTR_COLUMN_INDEX,
  AOM_ATTR_COLUMN_SPAN,
  AOM_ATTR_HIERARCHICAL_LEVEL,
  AOM_ATTR_POS_IN_SET,
  AOM_ATTR_ROW_COUNT,
  AOM_ATTR_ROW_INDEX,
  AOM_ATTR_ROW_SPAN,
  AOM_ATTR_SET_SIZE,
};

class WebComputedAXTree {
 public:
  virtual ~WebComputedAXTree() {}

  virtual bool ComputeAccessibilityTree() = 0;
  virtual WebString GetNameForAXNode(int32_t axID) = 0;
  virtual WebString GetRoleForAXNode(int32_t axID) = 0;

  // Get the specified attribute for |axID|, returning true if the node is
  // present in the tree, and has the attribute |int_attr|. Stores this
  // attribute value in |out_param|.
  virtual bool GetIntAttributeForId(int32_t axID,
                                    AOMIntAttribute,
                                    int32_t* out_param) = 0;
};

}  // namespace blink

#endif  // WebComputedAXTree_h
