// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_PAINT_PAINT_FONT_LOADER_MAC_IN_PROCESS_ONLY_H_
#define CC_PAINT_PAINT_FONT_LOADER_MAC_IN_PROCESS_ONLY_H_

#include "build/build_config.h"
#include "cc/paint/paint_font_loader_mac.h"

namespace cc {

// TODO(vmpstr): We need to support font loading from the browser process as
// well. This involves sending mojo messages that are currently implemented in
// content. We need to figure out how to get access to that functionality from
// cc/paint.
class PaintFontLoaderMac_InProcessOnly : public PaintFontLoaderMac {
 public:
  sk_sp<SkTypeface> LoadFont(NSFont* font, float text_size) const override;
};

}  // namespace cc

#endif  // CC_PAINT_PAINT_FONT_LOADER_MAC_IN_PROCESS_ONLY_H_
