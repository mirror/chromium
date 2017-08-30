// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_PUBLIC_CPP_TOUCHSCREEN_STATUS_SOURCE_H_
#define ASH_PUBLIC_CPP_TOUCHSCREEN_STATUS_SOURCE_H_

namespace ash {

enum class TouchscreenStatusSource {
  // The user profile preference touchscreen status source.
  USE_PREF,
  // The global touchscreen status source.
  GLOBAL,
};

}  // namespace ash

#endif  // ASH_PUBLIC_CPP_TOUCHSCREEN_STATUS_SOURCE_H_
