// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_ANDROID_MOTION_EVENT_SYNTHESIZER_H_
#define CONTENT_PUBLIC_BROWSER_ANDROID_MOTION_EVENT_SYNTHESIZER_H_

#include "content/common/content_export.h"

namespace ui {
class ViewAndroid;
}

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

class CONTENT_EXPORT MotionEventSynthesizer {
 public:
  // Creates and returns a MotionEventSynthesizer instance.
  static std::unique_ptr<MotionEventSynthesizer> Create(ui::ViewAndroid* view);

  virtual void SetPointer(int index, int x, int y, int id) = 0;
  virtual void SetScrollDeltas(int x, int y, int dx, int dy) = 0;
  virtual void Inject(MotionEventAction action,
                      int pointer_count,
                      int64_t time_ms) = 0;
  virtual ~MotionEventSynthesizer() {}
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_ANDROID_MOTION_EVENT_SYNTHESIZER_H_
