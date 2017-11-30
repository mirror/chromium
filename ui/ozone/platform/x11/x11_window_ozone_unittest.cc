// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/events/devices/x11/touch_factory_x11.h"
#include "ui/events/test/events_test_utils_x11.h"
#include "ui/ozone/public/ozone_platform.h"
#include "ui/platform_window/platform_window_delegate.h"
#include "ui/platform_window/x11/x11_window_ozone.h"

namespace ui {

using ::testing::Eq;
using ::testing::_;

namespace {

const int kPointerDeviceId = 1;

class MockPlatformWindowDelegate : public PlatformWindowDelegate {
 public:
  MockPlatformWindowDelegate() = default;
  ~MockPlatformWindowDelegate() override = default;

  MOCK_METHOD1(DispatchEvent, void(ui::Event* event));

  MOCK_METHOD2(OnAcceleratedWidgetAvailable,
               void(gfx::AcceleratedWidget widget, float device_pixel_ratio));
  MOCK_METHOD0(OnCloseRequest, void());
  MOCK_METHOD0(OnClosed, void());
  MOCK_METHOD1(OnWindowStateChanged, void(PlatformWindowState new_state));
  MOCK_METHOD0(OnLostCapture, void());
  MOCK_METHOD1(OnBoundsChanged, void(const gfx::Rect& new_bounds));
  MOCK_METHOD1(OnDamageRect, void(const gfx::Rect& damaged_region));
  MOCK_METHOD0(OnAcceleratedWidgetDestroyed, void());
  MOCK_METHOD1(OnActivationChanged, void(bool active));
  MOCK_METHOD1(GetParentWindowAcceleratedWidget,
               void(gfx::AcceleratedWidget* widget));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockPlatformWindowDelegate);
};

ACTION_P(StoreWidget, ptr) {
  *ptr = arg0;
}

ACTION_P(CloneEvent, ptr) {
  *ptr = Event::Clone(*arg0);
}

}  // namespace

class X11WindowOzoneTest : public testing::Test {
 public:
  X11WindowOzoneTest()
      : task_env_(new base::test::ScopedTaskEnvironment(
            base::test::ScopedTaskEnvironment::MainThreadType::UI)) {}

  ~X11WindowOzoneTest() override = default;

  void SetUp() override {
    ui::OzonePlatform::InitParams params;
    OzonePlatform::InitializeForUI(params);

    ozone_platform_x11_ = OzonePlatform::GetInstance();
    std::vector<int> pointer_devices;
    pointer_devices.push_back(kPointerDeviceId);
    ui::TouchFactory::GetInstance()->SetPointerDeviceForTest(pointer_devices);
  }

  void TearDown() override { OzonePlatform::Shutdown(); }

 protected:
  std::unique_ptr<PlatformWindow> CreatePlatformWindow(
      MockPlatformWindowDelegate* delegate,
      const gfx::Rect& bounds,
      gfx::AcceleratedWidget* widget) {
    EXPECT_CALL(*delegate, OnAcceleratedWidgetAvailable(_, _))
        .WillOnce(StoreWidget(widget));
    return ozone_platform_x11_->CreatePlatformWindow(delegate, bounds);
  }

  void DispatchXEvent(XEvent* event, gfx::AcceleratedWidget widget) {
    DCHECK_EQ(GenericEvent, event->type);
    XIDeviceEvent* device_event =
        static_cast<XIDeviceEvent*>(event->xcookie.data);
    device_event->event = widget;
    auto* event_source = X11EventSourceLibevent::GetInstance();
    event_source->ProcessXEvent(event);
  }

 private:
  std::unique_ptr<base::test::ScopedTaskEnvironment> task_env_;
  OzonePlatform* ozone_platform_x11_;
};

// This test ensures that events are handled by a right target. Most common
// case is when XEvent is sent for a certain target and processed by it.
// Another case is when a window sets an explicit capture and must intercept and
// process an event even though it has been sent to another window.
TEST_F(X11WindowOzoneTest, SendPlatformEventToRightTarget) {
  MockPlatformWindowDelegate delegate;
  gfx::AcceleratedWidget widget;
  gfx::Rect bounds(30, 80, 800, 600);
  EXPECT_CALL(delegate, OnClosed()).Times(1);

  auto window = CreatePlatformWindow(&delegate, bounds, &widget);

  ui::ScopedXI2Event xi_event;
  xi_event.InitGenericButtonEvent(kPointerDeviceId, ui::ET_MOUSE_PRESSED,
                                  gfx::Point(218, 290), ui::EF_NONE);

  // First check events can be received by a target window.
  std::unique_ptr<Event> event;
  EXPECT_CALL(delegate, DispatchEvent(_)).WillOnce(CloneEvent(&event));

  DispatchXEvent(xi_event, widget);
  EXPECT_EQ(ui::ET_MOUSE_PRESSED, event->type());

  base::RunLoop().RunUntilIdle();

  event.reset();

  MockPlatformWindowDelegate delegate_2;
  gfx::AcceleratedWidget widget_2;
  gfx::Rect bounds_2(525, 155, 296, 407);
  EXPECT_CALL(delegate_2, OnClosed()).Times(1);

  auto window_2 = CreatePlatformWindow(&delegate_2, bounds_2, &widget_2);

  // Check event goes to right target without capture being set.
  std::unique_ptr<Event> event_2;
  EXPECT_CALL(delegate, DispatchEvent(_)).Times(0);
  EXPECT_CALL(delegate_2, DispatchEvent(_)).WillOnce(CloneEvent(&event_2));

  DispatchXEvent(xi_event, widget_2);
  EXPECT_FALSE(event);
  EXPECT_EQ(ui::ET_MOUSE_PRESSED, event_2->type());

  base::RunLoop().RunUntilIdle();

  event.reset();
  event_2.reset();

  // Set capture to the second window, but send an event to another window
  // target. The event must have its location converted and received by the
  // captured window instead.
  window_2->SetCapture();
  EXPECT_CALL(delegate, DispatchEvent(_)).Times(0);
  EXPECT_CALL(delegate_2, DispatchEvent(_)).WillOnce(CloneEvent(&event_2));

  DispatchXEvent(xi_event, widget);
  EXPECT_FALSE(event);
  EXPECT_EQ(ui::ET_MOUSE_PRESSED, event_2->type());
  EXPECT_EQ(gfx::Point(-277, 215), event_2->AsLocatedEvent()->location());

  base::RunLoop().RunUntilIdle();
}

}  // namespace ui
