// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "cc/paint/paint_font_loader_mac_in_process_only.h"
#import "third_party/skia/include/ports/SkTypeface_mac.h"

namespace cc {

sk_sp<SkTypeface> PaintFontLoaderMac_InProcessOnly::LoadFont(
    NSFont* font,
    float text_size) const {
  return sk_sp<SkTypeface>(
      SkCreateTypefaceFromCTFont(reinterpret_cast<CTFontRef>(font)));
}

}  // namespace cc
