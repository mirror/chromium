// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/vr_input_manager.h"

#include <memory>

#include "base/task_runner_util.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "ui/events/keycodes/dom/dom_key.h"

using blink::WebGestureEvent;
using blink::WebMouseEvent;
using blink::WebInputEvent;

namespace vr {

namespace {
WebGestureEvent MakeGestureEvent(WebInputEvent::Type type,
                                 double time,
                                 float x,
                                 float y) {
  WebGestureEvent result(type, WebInputEvent::kNoModifiers, time);
  result.x = x;
  result.y = y;
  result.source_device = blink::kWebGestureDeviceTouchpad;
  return result;
}
}  // namespace

VrInputManager::VrInputManager(content::WebContents* web_contents)
    : web_contents_(web_contents) {}

VrInputManager::~VrInputManager() = default;

void VrInputManager::ProcessUpdatedGesture(
    std::unique_ptr<blink::WebInputEvent> event) {
  if (!GestureIsValid(event->GetType()))
    return;

  UpdateState(event->GetType());

  content::RenderWidgetHost* rwh = nullptr;
  if (render_widget_host_ != nullptr) {
    rwh = render_widget_host_;
  } else {
    if (!web_contents_->GetRenderWidgetHostView())
      return;
    rwh = web_contents_->GetRenderWidgetHostView()->GetRenderWidgetHost();
  }
  if (rwh == nullptr)
    return;

  if (WebInputEvent::IsMouseEventType(event->GetType()))
    ForwardMouseEvent(rwh, static_cast<const blink::WebMouseEvent&>(*event));
  else if (WebInputEvent::IsGestureEventType(event->GetType()))
    SendGesture(rwh, static_cast<const blink::WebGestureEvent&>(*event));
}

void VrInputManager::UpdateState(WebInputEvent::Type type) {
  if (type == WebInputEvent::kGestureScrollBegin)
    is_in_scroll_ = true;

  if (type == WebInputEvent::kGestureScrollEnd ||
      type == WebInputEvent::kGestureFlingStart) {
    is_in_scroll_ = false;
  }
}

bool VrInputManager::GestureIsValid(WebInputEvent::Type type) {
  if (!is_in_scroll_ && (type == WebInputEvent::kGestureScrollUpdate ||
                         type == WebInputEvent::kGestureScrollEnd ||
                         type == WebInputEvent::kGestureFlingStart)) {
    return false;
  }
  return true;
}

void VrInputManager::SendGesture(content::RenderWidgetHost* rwh,
                                 const WebGestureEvent& gesture) {
  if (gesture.GetType() == WebGestureEvent::kGestureTapDown) {
    ForwardGestureEvent(rwh, gesture);

    // Generate and forward Tap
    WebGestureEvent tap_event =
        MakeGestureEvent(WebInputEvent::kGestureTap, gesture.TimeStampSeconds(),
                         gesture.x, gesture.y);
    tap_event.data.tap.tap_count = 1;
    ForwardGestureEvent(rwh, tap_event);
  } else {
    ForwardGestureEvent(rwh, gesture);
  }
}

void VrInputManager::SetRenderWidgetHostForTest(
    content::RenderWidgetHost* rwh) {
  render_widget_host_ = rwh;
}

void VrInputManager::ForwardGestureEvent(
    content::RenderWidgetHost* rwh,
    const blink::WebGestureEvent& gesture) {
  rwh->ForwardGestureEvent(gesture);
}

void VrInputManager::ForwardMouseEvent(
    content::RenderWidgetHost* rwh,
    const blink::WebMouseEvent& mouse_event) {
  rwh->ForwardMouseEvent(mouse_event);
}

}  // namespace vr
