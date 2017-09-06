// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_VR_INPUT_MANAGER_H_
#define CHROME_BROWSER_VR_VR_INPUT_MANAGER_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/render_widget_host.h"
#include "third_party/WebKit/public/platform/WebGestureEvent.h"
#include "third_party/WebKit/public/platform/WebInputEvent.h"
#include "third_party/WebKit/public/platform/WebMouseEvent.h"

namespace content {
class WebContents;
}

namespace vr {

// Note: This class is not thread safe and must only be used from main thread.
class VrInputManager {
 public:
  explicit VrInputManager(content::WebContents* web_contents);
  ~VrInputManager();

  void ProcessUpdatedGesture(std::unique_ptr<blink::WebInputEvent> event);

  // For test
  void SetRenderWidgetHostForTest(content::RenderWidgetHost* rwh);

 private:
  void UpdateState(blink::WebInputEvent::Type type);
  bool GestureIsValid(blink::WebInputEvent::Type type);
  void SendGesture(content::RenderWidgetHost* rwh,
                   const blink::WebGestureEvent& gesture);
  void ForwardGestureEvent(content::RenderWidgetHost* rwh,
                           const blink::WebGestureEvent& gesture);
  void ForwardMouseEvent(content::RenderWidgetHost* rwh,
                         const blink::WebMouseEvent& mouse_event);

  content::WebContents* web_contents_;
  bool is_in_scroll_ = false;

  // For test
  content::RenderWidgetHost* render_widget_host_ = nullptr;
  DISALLOW_COPY_AND_ASSIGN(VrInputManager);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_VR_INPUT_MANAGER_H_
