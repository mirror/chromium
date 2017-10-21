// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ScrollCustomization_h
#define ScrollCustomization_h

#include "cc/input/scroll_customization.h"
#include "platform/PlatformExport.h"

namespace blink {

using ScrollCustomizationEnabledDirection =
    cc::ScrollCustomizationEnabledDirection;

PLATFORM_EXPORT ScrollCustomizationEnabledDirection
GetScrollCustomizationForDirection(double delta_x, double delta_y);

}  // namespace blink

#endif  // ScrollCustomization_h
