// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ScrollCustomization_h
#define ScrollCustomization_h

#include "platform/PlatformExport.h"

namespace blink {
namespace scroll_customization {
using ScrollDirection = unsigned char;

PLATFORM_EXPORT extern const ScrollDirection kScrollDirectionNone;
PLATFORM_EXPORT extern const ScrollDirection kScrollDirectionPanLeft;
PLATFORM_EXPORT extern const ScrollDirection kScrollDirectionPanRight;
PLATFORM_EXPORT extern const ScrollDirection kScrollDirectionPanX;
PLATFORM_EXPORT extern const ScrollDirection kScrollDirectionPanUp;
PLATFORM_EXPORT extern const ScrollDirection kScrollDirectionPanDown;
PLATFORM_EXPORT extern const ScrollDirection kScrollDirectionPanY;
PLATFORM_EXPORT extern const ScrollDirection kScrollDirectionAuto;

PLATFORM_EXPORT ScrollDirection GetScrollDirectionFromDeltas(double delta_x,
                                                             double delta_y);
}  // namespace scroll_customization
}  // namespace blink

#endif  // ScrollCustomization_h
