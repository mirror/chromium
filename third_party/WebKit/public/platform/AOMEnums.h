// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef AOMEnums_h
#define AOMEnums_h

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

}  // namespace blink

#endif  // AOMEnums_h
