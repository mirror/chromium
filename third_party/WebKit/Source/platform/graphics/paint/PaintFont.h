// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PaintFont_h
#define PaintFont_h

#include "build/build_config.h"
#include "cc/paint/paint_font.h"

#if defined(OS_MACOSX)
#include "cc/paint/paint_font_loader_mac.h"
#endif

namespace blink {
using cc::PaintFont;

#if defined(OS_MACOSX)
using cc::PaintFontLoaderMac;
#endif
}

#endif  // PaintFont_h
