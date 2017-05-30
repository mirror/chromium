// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TouchEventRegion_h
#define TouchEventRegion_h

#include "cc/base/region.h"
#include "cc/input/touch_action.h"

namespace cc {

struct TouchEventRegion {
  TouchAction touch_action;
  Region region;
};

}  // namespace cc

#endif
