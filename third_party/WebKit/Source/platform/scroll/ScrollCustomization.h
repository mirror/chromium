// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ScrollCustomization_h
#define ScrollCustomization_h

#include "cc/input/scroll_customization.h"
#include "third_party/WebKit/Source/platform/scroll/ScrollStateData.h"

namespace blink {

using ScrollCustomization = cc::ScrollCustomization;

inline ScrollCustomization GetScrollCustomizationFromScrollStateData(
    const ScrollStateData& data) {
  return cc::GetScrollCustomizationFromScrollStateData(data);
}

}  // namespace blink

#endif  // ScrollCustomization_h
