// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_VR_SHELL_ANDROID_UI_GESTURE_TARGET_H_
#define CHROME_BROWSER_ANDROID_VR_SHELL_ANDROID_UI_GESTURE_TARGET_H_

#include <memory>

#include "base/macros.h"
#include "content/public/browser/android/motion_event_synthesizer.h"

namespace blink {
class WebInputEvent;
}

namespace content {
class WebContents;
}

namespace ui {
class ViewAndroid;
}

namespace vr_shell {

class AndroidUiGestureTarget {
 public:
  AndroidUiGestureTarget(content::WebContents* web_contents,
                         ui::ViewAndroid* view,
                         float dpr_ratio);
  ~AndroidUiGestureTarget();

  void DispatchWebInputEvent(std::unique_ptr<blink::WebInputEvent> event);

 private:
  void Inject(content::MotionEventAction action, int64_t time_ms);
  void SetPointer(int x, int y);

  class EventSynthesizerHelper
      : public content::MotionEventSynthesizer::Helper {
   public:
    explicit EventSynthesizerHelper(content::WebContents* web_contents)
        : web_contents_(web_contents) {}
    content::WebContents* GetWebContents() const override;

   private:
    content::WebContents* web_contents_;
  };

  std::unique_ptr<EventSynthesizerHelper> event_helper_;
  std::unique_ptr<content::MotionEventSynthesizer> event_synthesizer_;
  int scroll_x_ = 0;
  int scroll_y_ = 0;
  float scroll_ratio_;

  DISALLOW_COPY_AND_ASSIGN(AndroidUiGestureTarget);
};

}  // namespace vr_shell

#endif  // CHROME_BROWSER_ANDROID_VR_SHELL_ANDROID_UI_GESTURE_TARGET_H_
