// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/vr_input_manager.h"

#include <memory>

#include "base/memory/ptr_util.h"
#include "base/time/time.h"
#include "chrome/browser/vr/content_input_delegate.h"
#include "chrome/browser/vr/elements/ui_element.h"
#include "chrome/browser/vr/test/animation_utils.h"
#include "chrome/browser/vr/test/mock_content_input_delegate.h"
#include "chrome/browser/vr/test/ui_scene_manager_test.h"
#include "chrome/browser/vr/ui_scene.h"
#include "content/public/browser/render_widget_host.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "third_party/WebKit/public/platform/WebGestureEvent.h"
#include "ui/gfx/geometry/vector3d_f.h"

namespace vr {

class MockRenderWidgetHost : public content::RenderWidgetHost {
 public:
  MockRenderWidgetHost() { last_gesture_ = blink::WebGestureEvent::kUndefined; }
  blink::WebInputEvent::Type GetLastGesture() { return last_gesture_; }

  bool Send(IPC::Message* msg) override { return false; }
  void UpdateTextDirection(blink::WebTextDirection direction) override {}
  void NotifyTextDirection() override {}
  void Focus() override {}
  void Blur() override {}
  void SetActive(bool active) override {}
  void ForwardMouseEvent(const blink::WebMouseEvent& mouse_event) override {}
  void ForwardWheelEvent(
      const blink::WebMouseWheelEvent& wheel_event) override {}
  void ForwardKeyboardEvent(
      const content::NativeWebKeyboardEvent& key_event) override {}
  void ForwardKeyboardEventWithLatencyInfo(
      const content::NativeWebKeyboardEvent& key_event,
      const ui::LatencyInfo& latency_info) override{};
  void ForwardGestureEvent(
      const blink::WebGestureEvent& gesture_event) override {
    last_gesture_ = gesture_event.GetType();
  }
  content::RenderProcessHost* GetProcess() const override { return nullptr; }
  int GetRoutingID() const override { return 0; }
  content::RenderWidgetHostView* GetView() const override { return nullptr; }
  bool IsLoading() const override { return false; }
  void RestartHangMonitorTimeoutIfNecessary() override {}
  void SetIgnoreInputEvents(bool ignore_input_events) override {}
  void WasResized() override {}
  void AddKeyPressEventCallback(
      const content::RenderWidgetHost::KeyPressEventCallback& callback)
      override {}
  void RemoveKeyPressEventCallback(
      const content::RenderWidgetHost::KeyPressEventCallback& callback)
      override {}
  void AddMouseEventCallback(
      const content::RenderWidgetHost::MouseEventCallback& callback) override {}
  void RemoveMouseEventCallback(
      const content::RenderWidgetHost::MouseEventCallback& callback) override {}
  void AddInputEventObserver(
      RenderWidgetHost::InputEventObserver* observer) override {}
  void RemoveInputEventObserver(
      RenderWidgetHost::InputEventObserver* observer) override {}
  void GetScreenInfo(content::ScreenInfo* result) override {}

 private:
  blink::WebInputEvent::Type last_gesture_;
};

class VrInputManagerTest : public testing::Test {
 public:
  void SetUp() override {
    input_manager_ = base::MakeUnique<VrInputManager>(nullptr);
    input_manager_->SetRenderWidgetHostForTest(&render_widget_host_);
  }

 protected:
  std::unique_ptr<VrInputManager> input_manager_;
  MockRenderWidgetHost render_widget_host_;
};

TEST_F(VrInputManagerTest, ScrollWithoutScrollBegin) {
  std::unique_ptr<blink::WebInputEvent> scroll_update(
      new blink::WebGestureEvent());
  scroll_update->SetType(blink::WebInputEvent::kGestureScrollUpdate);
  input_manager_->ProcessUpdatedGesture(std::move(scroll_update));
  EXPECT_EQ(render_widget_host_.GetLastGesture(),
            blink::WebGestureEvent::kUndefined);

  std::unique_ptr<blink::WebInputEvent> scroll_end(
      new blink::WebGestureEvent());
  scroll_end->SetType(blink::WebInputEvent::kGestureScrollEnd);
  input_manager_->ProcessUpdatedGesture(std::move(scroll_end));
  EXPECT_EQ(render_widget_host_.GetLastGesture(),
            blink::WebGestureEvent::kUndefined);

  std::unique_ptr<blink::WebInputEvent> fling(new blink::WebGestureEvent());
  fling->SetType(blink::WebInputEvent::kGestureFlingStart);
  input_manager_->ProcessUpdatedGesture(std::move(fling));
  EXPECT_EQ(render_widget_host_.GetLastGesture(),
            blink::WebGestureEvent::kUndefined);
}

TEST_F(VrInputManagerTest, ScrollWithScrollBegin) {
  std::unique_ptr<blink::WebInputEvent> scroll_begin(
      new blink::WebGestureEvent());
  scroll_begin->SetType(blink::WebInputEvent::kGestureScrollBegin);
  input_manager_->ProcessUpdatedGesture(std::move(scroll_begin));
  EXPECT_EQ(render_widget_host_.GetLastGesture(),
            blink::WebGestureEvent::kGestureScrollBegin);

  std::unique_ptr<blink::WebInputEvent> scroll_end(
      new blink::WebGestureEvent());
  scroll_end->SetType(blink::WebInputEvent::kGestureScrollEnd);
  input_manager_->ProcessUpdatedGesture(std::move(scroll_end));
  EXPECT_EQ(render_widget_host_.GetLastGesture(),
            blink::WebGestureEvent::kGestureScrollEnd);
}

}  // namespace vr
