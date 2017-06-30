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

class WebContents;

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

// Interface used to generate and inject synthetic touch events.
// An instance can be obtained through the method |Create|.
class CONTENT_EXPORT MotionEventSynthesizer {
 public:
  // A helper that manages internal object management with the help of
  // WebContents. Callsite should provide it through constructor.
  class Helper {
   public:
    virtual WebContents* GetWebContents() const = 0;
  };

  // Creates and returns a MotionEventSynthesizer instance.
  static std::unique_ptr<MotionEventSynthesizer> Create(Helper* helper,
                                                        ui::ViewAndroid* view);

  // Sets the coordinate of a point of an event to inject.
  virtual void SetPointer(int index, int x, int y, int id) = 0;

  // Sets the amount of scroll to be applied to an event.
  virtual void SetScrollDeltas(int x, int y, int dx, int dy) = 0;

  // Injects an event with a given action at the point with coordinates info
  // provided ahead.
  virtual void Inject(MotionEventAction action,
                      int pointer_count,
                      int64_t time_ms) = 0;

  virtual ~MotionEventSynthesizer() {}
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_ANDROID_MOTION_EVENT_SYNTHESIZER_H_
