// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PlatformAXNode_h
#define PlatformAXNode_h

#include <string>

namespace blink {

enum PlatformAXStringAttribute {
  AX_STRING_ATTRIBUTE_NONE,
  AX_ATTR_ACCESS_KEY,
  AX_ATTR_ARIA_INVALID_VALUE,
  AX_ATTR_AUTO_COMPLETE,
  AX_ATTR_CHROME_CHANNEL,
  AX_ATTR_CLASS_NAME,
  AX_ATTR_CONTAINER_LIVE_RELEVANT,
  AX_ATTR_CONTAINER_LIVE_STATUS,
  AX_ATTR_DESCRIPTION,
  AX_ATTR_DISPLAY,
  AX_ATTR_FONT_FAMILY,
  AX_ATTR_HTML_TAG,
  AX_ATTR_IMAGE_DATA_URL,
  AX_ATTR_INNER_HTML,
  AX_ATTR_KEY_SHORTCUTS,
  AX_ATTR_LANGUAGE,
  AX_ATTR_NAME,
  AX_ATTR_LIVE_RELEVANT,
  AX_ATTR_LIVE_STATUS,
  AX_ATTR_PLACEHOLDER,
  AX_ATTR_ROLE,
  AX_ATTR_ROLE_DESCRIPTION,
  AX_ATTR_URL,
  AX_ATTR_VALUE,
  AX_STRING_ATTRIBUTE_LAST = AX_ATTR_VALUE,
};

class PlatformAXNode {
 public:
  ~PlatformAXNode() = default;

  const std::string& GetStringAttribute(PlatformAXStringAttribute);
};

}  // namespace blink

#endif  // PlatformAXNode_h
