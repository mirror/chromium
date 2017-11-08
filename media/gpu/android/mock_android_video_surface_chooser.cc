// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/android/mock_android_video_surface_chooser.h"

namespace media {

MockAndroidVideoSurfaceChooser::MockAndroidVideoSurfaceChooser(
    AndroidVideoSurfaceChooser::State* state)
    : state_(state) {}

MockAndroidVideoSurfaceChooser::~MockAndroidVideoSurfaceChooser() {}

void MockAndroidVideoSurfaceChooser::UpdateState(
    base::Optional<AndroidOverlayFactoryCB> new_factory,
    const State& new_state) {
  MockUpdateState();
  *state_ = new_state;
}

}  // namespace media
