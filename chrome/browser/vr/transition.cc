// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/transition.h"

namespace vr {

namespace {
static constexpr int kDefaultTransitionDurationMS = 120;
}  // namespace

Transition::Transition()
    : duration(
          base::TimeDelta::FromMilliseconds(kDefaultTransitionDurationMS)) {}

Transition::~Transition() {}

}  // namespace vr
