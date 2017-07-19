// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_ANDROID_MOTION_EVENT_ACTION_H_
#define CONTENT_PUBLIC_BROWSER_ANDROID_MOTION_EVENT_ACTION_H_

namespace content {

// A Java counterpart will be generated for this enum.
// GENERATED_JAVA_ENUM_PACKAGE: org.chromium.content.browser
enum MotionEventAction {
  Invalid = -1,
  Start = 0,
  Move = 1,
  Cancel = 2,
  End = 3,
  Scroll = 4,
  HoverEnter = 5,
  HoverExit = 6,
  HoverMove = 7,
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_ANDROID_MOTION_EVENT_ACTION_H_
