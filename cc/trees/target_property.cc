// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/trees/target_property.h"

namespace cc {

static_assert(TargetProperty::LAST_TARGET_PROPERTY < kMaxTargetPropertyIndex,
              "The number of cc target properties has exceeded the capacity of"
              " TargetProperties");

static_assert(kMaxTargetPropertyIndex % sizeof(uint32_t) == 0,
              "The maximum number of target properties should be a multiple of "
              "sizeof(uint32_t)");

}  // namespace cc
