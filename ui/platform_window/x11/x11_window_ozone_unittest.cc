// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/events/devices/x11/touch_factory_x11.h"
#include "ui/events/event.h"
#include "ui/events/platform/x11/x11_event_source_libevent.h"
#include "ui/events/test/events_test_utils_x11.h"
#include "ui/platform_window/platform_window_delegate.h"
#include "ui/platform_window/x11/x11_window_manager_ozone.h"
#include "ui/platform_window/x11/x11_window_ozone.h"

namespace ui {

namespace {

using ::testing::Eq;
using ::testing::_;

constexpr int kPointerDeviceId = 1;

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

ACTION_P(StoreWidget, widget_ptr) {
  *widget_ptr = arg0;
}

ACTION_P(CloneEvent, event_ptr) {
  *event_ptr = Event::Clone(*arg0);
}

}  // namespace

class X11WindowOzoneTest : public testing::Test {
 public:
  X11WindowOzoneTest()
      : task_env_(std::make_unique<base::test::ScopedTaskEnvironment>(
            base::test::ScopedTaskEnvironment::MainThreadType::UI)) {}

  ~X11WindowOzoneTest() override = default;

  void SetUp() override {
    XDisplay* display = gfx::GetXDisplay();
    event_source_ = std::make_unique<X11EventSourceLibevent>(display);
    window_manager_ = std::make_unique<X11WindowManagerOzone>();

    ui::TouchFactory::GetInstance()->SetPointerDeviceForTest(
        {kPointerDeviceId});
  }

 protected:
  std::unique_ptr<PlatformWindow> CreatePlatformWindow(
      MockPlatformWindowDelegate* delegate,
      const gfx::Rect& bounds,
      gfx::AcceleratedWidget* widget) {
    EXPECT_CALL(*delegate, OnAcceleratedWidgetAvailable(_, _))
        .WillOnce(StoreWidget(widget));
    auto window = std::make_unique<X11WindowOzone>(window_manager_.get(),
                                                   delegate, bounds);
    window->Create();
    return std::move(window);
  }

  void DispatchXEvent(XEvent* event, gfx::AcceleratedWidget widget) {
    DCHECK_EQ(GenericEvent, event->type);
    XIDeviceEvent* device_event =
        static_cast<XIDeviceEvent*>(event->xcookie.data);
    device_event->event = widget;
    auto* event_source = X11EventSourceLibevent::GetInstance();
    DCHECK(event_source);
    event_source->ProcessXEvent(event);
  }

 private:
  std::unique_ptr<base::test::ScopedTaskEnvironment> task_env_;
  std::unique_ptr<X11WindowManagerOzone> window_manager_;
  std::unique_ptr<X11EventSourceLibevent> event_source_;

  DISALLOW_COPY_AND_ASSIGN(X11WindowOzoneTest);
};

// This test ensures that events are handled by a right target(widget).
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
}

// This test case ensures that events are consumed by a window with explicit
// capture, even though the event is sent to other window.
TEST_F(X11WindowOzoneTest, SendPlatformEventToCapturedWindow) {
  MockPlatformWindowDelegate delegate;
  gfx::AcceleratedWidget widget;
  gfx::Rect bounds(30, 80, 800, 600);
  EXPECT_CALL(delegate, OnClosed()).Times(1);
  auto window = CreatePlatformWindow(&delegate, bounds, &widget);

  MockPlatformWindowDelegate delegate_2;
  gfx::AcceleratedWidget widget_2;
  gfx::Rect bounds_2(525, 155, 296, 407);
  EXPECT_CALL(delegate_2, OnClosed()).Times(1);
  auto window_2 = CreatePlatformWindow(&delegate_2, bounds_2, &widget_2);

  ui::ScopedXI2Event xi_event;
  xi_event.InitGenericButtonEvent(kPointerDeviceId, ui::ET_MOUSE_PRESSED,
                                  gfx::Point(218, 290), ui::EF_NONE);

  // Set capture to the second window, but send an event to another window
  // target. The event must have its location converted and received by the
  // captured window instead.
  window_2->SetCapture();
  std::unique_ptr<ui::Event> event;
  EXPECT_CALL(delegate, DispatchEvent(_)).Times(0);
  EXPECT_CALL(delegate_2, DispatchEvent(_)).WillOnce(CloneEvent(&event));

  DispatchXEvent(xi_event, widget);
  EXPECT_EQ(ui::ET_MOUSE_PRESSED, event->type());
  EXPECT_EQ(gfx::Point(-277, 215), event->AsLocatedEvent()->location());

  base::RunLoop().RunUntilIdle();
}

}  // namespace ui
