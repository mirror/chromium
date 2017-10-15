// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_ANIMATION_TARGET_PROPERTIES_H_
#define CC_ANIMATION_TARGET_PROPERTIES_H_

#include <bitset>

namespace cc {

static constexpr size_t kMaxTargetProprtyIndex = 32u;

using TargetProperties = std::bitset<kMaxTargetProprtyIndex>;

}  // namespace cc

#endif  // CC_ANIMATION_TARGET_PROPERTIES_H_
