// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_GPU_ANDROID_MOCK_ANDROID_VIDEO_SURFACE_CHOOSER_H_
#define MEDIA_GPU_ANDROID_MOCK_ANDROID_VIDEO_SURFACE_CHOOSER_H_

#include "media/gpu/android/android_video_surface_chooser.h"

#include "testing/gmock/include/gmock/gmock.h"

namespace media {

class MockAndroidVideoSurfaceChooser
    : public ::testing::NiceMock<AndroidVideoSurfaceChooser> {
 public:
  // We will copy state into |*state| when anybody calls UpdateState.
  MockAndroidVideoSurfaceChooser(AndroidVideoSurfaceChooser::State* state);
  ~MockAndroidVideoSurfaceChooser();

  MOCK_METHOD2(SetClientCallbacks,
               void(UseOverlayCB use_overlay_cb,
                    UseSurfaceTextureCB use_surface_texture_cb));

  // MockUpdateState is called by UpdateState.
  MOCK_METHOD0(MockUpdateState, void());
  void UpdateState(base::Optional<AndroidOverlayFactoryCB> new_factory,
                   const State& new_state) override;

 private:
  // State into which we'll copy the state we get from UpdateState.
  AndroidVideoSurfaceChooser::State* state_;
};

}  // namespace media

#endif  // MEDIA_GPU_ANDROID_MOCK_ANDROID_VIDEO_SURFACE_CHOOSER_H_
