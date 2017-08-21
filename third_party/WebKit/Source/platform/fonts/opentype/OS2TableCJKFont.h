// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OS2TableCJKFont_h
#define OS2TableCJKFont_h

#include "platform/PlatformExport.h"

class SkTypeface;

namespace blink {

class PLATFORM_EXPORT OS2TableCJKFont {
 public:
  static bool IsCJKFontFromOS2Table(SkTypeface*);
};

}  // namespace blink

#endif
