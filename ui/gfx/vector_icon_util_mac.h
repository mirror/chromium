// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_VECTOR_ICON_UTIL_MAC_H_
#define UI_GFX_VECTOR_ICON_UTIL_MAC_H_

#import <Cocoa/Cocoa.h>

#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/gfx_export.h"
#include "ui/gfx/vector_icon_types.h"

namespace gfx {
// Creates an autoreleased NSImage with a custom image rep that draws |icon|.
// Uses the icon's natural size.
GFX_EXPORT NSImage* NSImageWithIcon(const VectorIcon& icon, SkColor color);
// Creates an autoreleased NSImage with a custom image rep that draws |icon|,
// mirroring for RTL if |flip_x|  is true.
GFX_EXPORT NSImage* NSImageWithIcon(const VectorIcon& icon,
                                    SkColor color,
                                    NSSize size,
                                    bool flip_x);
}  // namespace gfx

#endif  // UI_GFX_VECTOR_ICON_UTIL_MAC_H_
