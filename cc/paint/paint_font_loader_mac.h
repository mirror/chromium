// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_PAINT_PAINT_FONT_LOADER_MAC_H_
#define CC_PAINT_PAINT_FONT_LOADER_MAC_H_

#include "third_party/skia/include/core/SkRefCnt.h"

#if __OBJC__
@class NSFont;
#else
class NSFont;
#endif

class SkTypeface;

namespace cc {

class PaintFontLoaderMac {
 public:
  virtual sk_sp<SkTypeface> LoadFont(NSFont* font, float text_size) const = 0;
};

}  // namespace cc

#endif  // CC_PAINT_PAINT_FONT_LOADER_MAC_H_
