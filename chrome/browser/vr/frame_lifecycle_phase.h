// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_FRAME_LIFECYCLE_PHASE_H_
#define CHROME_BROWSER_VR_FRAME_LIFECYCLE_PHASE_H_

namespace vr {

// When the frame begins and time has updated, we enter the kDirty phase. As the
// frame lifecycle progresses, we advance through the subsequent phases.
enum FrameLifecyclePhase {
  kDirty = 0,
  kAnimationClean,
  kBindingsClean,
  kTexturesAndSizesClean,
  kLayoutClean,
  kClean,
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_FRAME_LIFECYCLE_PHASE_H_
