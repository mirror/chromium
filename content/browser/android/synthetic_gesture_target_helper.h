// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ANDROID_SYNTHETIC_GESTURE_TARGET_HELPER_H_
#define CONTENT_BROWSER_ANDROID_SYNTHETIC_GESTURE_TARGET_HELPER_H_

namespace content {

class RenderWidgetHostImpl;
class SyntheticGestureTargetAndroid;

// Helper method that creates |SyntheticGestureTargetAndroid| instance without
// breaking dependency rules. The Java counterpart is created together, and
// owned by |WebContentsImpl|.
SyntheticGestureTargetAndroid* CreateSyntheticGestureTargetAndroid(
    RenderWidgetHostImpl* host);

}  // namespace content

#endif  // CONTENT_BROWSER_ANDROID_SYNTHETIC_GESTURE_TARGET_HELPER_H_
