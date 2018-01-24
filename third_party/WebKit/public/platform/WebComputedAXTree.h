// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebComputedAXTree_h
#define WebComputedAXTree_h

#include "third_party/WebKit/public/platform/WebString.h"

namespace blink {

enum AOMStringAttribute {
  AOM_STRING_ATTRIBUTE_NONE,
  AOM_ATTR_KEY_SHORTCUTS,
  AOM_ATTR_NAME,
  AOM_ATTR_PLACEHOLDER,
  AOM_ATTR_ROLE,
  AOM_ATTR_ROLE_DESCRIPTION,
  AOM_ATTR_VALUE_TEXT,
};

class WebComputedAXTree {
 public:
  virtual ~WebComputedAXTree() {}

  virtual bool ComputeAccessibilityTree() = 0;

  // This will return a null string if the id does not correspond to a node in
  // the current AXTree snapshot, or if the requested property does not apply.
  virtual blink::WebString GetStringAttributeForAXNode(
      int32_t,
      blink::AOMStringAttribute) = 0;
};

}  // namespace blink

#endif  // WebComputedAXTree_h
