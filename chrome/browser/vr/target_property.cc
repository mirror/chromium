// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/target_property.h"

#include "cc/animation/target_properties.h"

namespace vr {

static_assert(TargetProperty::LAST_TARGET_PROPERTY < cc::kMaxTargetProprtyIndex,
              "The number of cc target properties has exceeded the capacity of"
              " TargetProperties");

}  // namespace vr
