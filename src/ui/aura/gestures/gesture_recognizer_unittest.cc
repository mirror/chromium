// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/gtest/include/gtest/gtest.h"
#include "ui/aura/event.h"
#include "ui/aura/root_window.h"
#include "ui/aura/test/aura_test_base.h"
#include "ui/aura/test/test_window_delegate.h"
#include "ui/aura/test/test_windows.h"
#include "ui/base/hit_test.h"
#include "ui/gfx/point.h"
#include "ui/gfx/rect.h"

namespace aura {
namespace test {

namespace {

// A delegate that keeps track of gesture events.
class GestureEventConsumeDelegate : public TestWindowDelegate {
 public:
  GestureEventConsumeDelegate()
      : tap_(false),
        tap_down_(false),
        double_tap_(false),
        scroll_begin_(false),
        scroll_update_(false),
        scroll_end_(false),
        pinch_begin_(false),
        pinch_update_(false),
        pinch_end_(false),
        scroll_x_(0),
        scroll_y_(0) {
  }

  virtual ~GestureEventConsumeDelegate() {}

  void Reset() {
    tap_ = false;
    tap_down_ = false;
    double_tap_ = false;
    scroll_begin_ = false;
    scroll_update_ = false;
    scroll_end_ = false;
    pinch_begin_ = false;
    pinch_update_ = false;
    pinch_end_ = false;

    scroll_x_ = 0;
    scroll_y_ = 0;
  }

  bool tap() const { return tap_; }
  bool tap_down() const { return tap_down_; }
  bool double_tap() const { return double_tap_; }
  bool scroll_begin() const { return scroll_begin_; }
  bool scroll_update() const { return scroll_update_; }
  bool scroll_end() const { return scroll_end_; }
  bool pinch_begin() const { return pinch_begin_; }
  bool pinch_update() const { return pinch_update_; }
  bool pinch_end() const { return pinch_end_; }

  float scroll_x() const { return scroll_x_; }
  float scroll_y() const { return scroll_y_; }

  virtual ui::GestureStatus OnGestureEvent(GestureEvent* gesture) OVERRIDE {
    switch (gesture->type()) {
      case ui::ET_GESTURE_TAP:
        tap_ = true;
        break;
      case ui::ET_GESTURE_TAP_DOWN:
        tap_down_ = true;
        break;
      case ui::ET_GESTURE_DOUBLE_TAP:
        double_tap_ = true;
        break;
      case ui::ET_GESTURE_SCROLL_BEGIN:
        scroll_begin_ = true;
        break;
      case ui::ET_GESTURE_SCROLL_UPDATE:
        scroll_update_ = true;
        scroll_x_ += gesture->delta_x();
        scroll_y_ += gesture->delta_y();
        break;
      case ui::ET_GESTURE_SCROLL_END:
        scroll_end_ = true;
        break;
      case ui::ET_GESTURE_PINCH_BEGIN:
        pinch_begin_ = true;
        break;
      case ui::ET_GESTURE_PINCH_UPDATE:
        pinch_update_ = true;
        break;
      case ui::ET_GESTURE_PINCH_END:
        pinch_end_ = true;
        break;
      default:
        NOTREACHED();
    }
    return ui::GESTURE_STATUS_CONSUMED;
  }

 private:
  bool tap_;
  bool tap_down_;
  bool double_tap_;
  bool scroll_begin_;
  bool scroll_update_;
  bool scroll_end_;
  bool pinch_begin_;
  bool pinch_update_;
  bool pinch_end_;

  float scroll_x_;
  float scroll_y_;

  DISALLOW_COPY_AND_ASSIGN(GestureEventConsumeDelegate);
};

class QueueTouchEventDelegate : public GestureEventConsumeDelegate {
 public:
  QueueTouchEventDelegate() : window_(NULL) {}
  virtual ~QueueTouchEventDelegate() {}

  virtual ui::TouchStatus OnTouchEvent(TouchEvent* event) OVERRIDE {
    return event->type() == ui::ET_TOUCH_RELEASED ?
        ui::TOUCH_STATUS_QUEUED_END : ui::TOUCH_STATUS_QUEUED;
  }

  void ReceivedAck() {
    RootWindow::GetInstance()->AdvanceQueuedTouchEvent(window_, false);
  }

  void set_window(Window* w) { window_ = w; }

 private:
  Window* window_;

  DISALLOW_COPY_AND_ASSIGN(QueueTouchEventDelegate);
};

// A delegate that ignores gesture events but keeps track of [synthetic] mouse
// events.
class GestureEventSynthDelegate : public TestWindowDelegate {
 public:
  GestureEventSynthDelegate()
      : mouse_enter_(false),
        mouse_exit_(false),
        mouse_press_(false),
        mouse_release_(false),
        mouse_move_(false) {
  }

  void Reset() {
    mouse_enter_ = false;
    mouse_exit_ = false;
    mouse_press_ = false;
    mouse_release_ = false;
    mouse_move_ = false;
  }

  bool mouse_enter() const { return mouse_enter_; }
  bool mouse_exit() const { return mouse_exit_; }
  bool mouse_press() const { return mouse_press_; }
  bool mouse_move() const { return mouse_move_; }
  bool mouse_release() const { return mouse_release_; }

  virtual bool OnMouseEvent(MouseEvent* event) OVERRIDE {
    switch (event->type()) {
      case ui::ET_MOUSE_PRESSED:
        mouse_press_ = true;
        break;
      case ui::ET_MOUSE_RELEASED:
        mouse_release_ = true;
        break;
      case ui::ET_MOUSE_MOVED:
        mouse_move_ = true;
        break;
      case ui::ET_MOUSE_ENTERED:
        mouse_enter_ = true;
        break;
      case ui::ET_MOUSE_EXITED:
        mouse_exit_ = true;
        break;
      default:
        NOTREACHED();
    }
    return true;
  }

 private:
  bool mouse_enter_;
  bool mouse_exit_;
  bool mouse_press_;
  bool mouse_release_;
  bool mouse_move_;

  DISALLOW_COPY_AND_ASSIGN(GestureEventSynthDelegate);
};

void SendScrollEvents(int x_start,
                      int y_start,
                      base::TimeDelta time_start,
                      int dx,
                      int dy,
                      int time_step,
                      int num_steps,
                      GestureEventConsumeDelegate* delegate) {
  int x = x_start;
  int y = y_start;
  base::TimeDelta time = time_start;

  for (int i = 0; i < num_steps; i++) {
    delegate->Reset();
    TouchEvent move(ui::ET_TOUCH_MOVED, gfx::Point(x, y), 0);
    Event::TestApi test_move(&move);
    test_move.set_time_stamp(time);
    RootWindow::GetInstance()->DispatchTouchEvent(&move);
    x += dx;
    y += dy;
    time = time + base::TimeDelta::FromMilliseconds(time_step);
  }
}

void SendScrollEvent(int x, int y, GestureEventConsumeDelegate* delegate) {
  delegate->Reset();
  TouchEvent move(ui::ET_TOUCH_MOVED, gfx::Point(x, y), 0);
  RootWindow::GetInstance()->DispatchTouchEvent(&move);
}

const int kBufferedPoints = 10;

}  // namespace

typedef AuraTestBase GestureRecognizerTest;

// Check that appropriate touch events generate tap gesture events.
TEST_F(GestureRecognizerTest, GestureEventTap) {
  scoped_ptr<GestureEventConsumeDelegate> delegate(
      new GestureEventConsumeDelegate());
  const int kWindowWidth = 123;
  const int kWindowHeight = 45;
  gfx::Rect bounds(100, 200, kWindowWidth, kWindowHeight);
  scoped_ptr<aura::Window> window(CreateTestWindowWithDelegate(
      delegate.get(), -1234, bounds, NULL));

  delegate->Reset();
  TouchEvent press(ui::ET_TOUCH_PRESSED, gfx::Point(101, 201), 0);
  RootWindow::GetInstance()->DispatchTouchEvent(&press);
  EXPECT_FALSE(delegate->tap());
  EXPECT_TRUE(delegate->tap_down());
  EXPECT_FALSE(delegate->double_tap());
  EXPECT_FALSE(delegate->scroll_begin());
  EXPECT_FALSE(delegate->scroll_update());
  EXPECT_FALSE(delegate->scroll_end());

  // Make sure there is enough delay before the touch is released so that it is
  // recognized as a tap.
  delegate->Reset();
  TouchEvent release(ui::ET_TOUCH_RELEASED, gfx::Point(101, 201), 0);
  Event::TestApi test_release(&release);
  test_release.set_time_stamp(press.time_stamp() +
                              base::TimeDelta::FromMilliseconds(50));
  RootWindow::GetInstance()->DispatchTouchEvent(&release);
  EXPECT_TRUE(delegate->tap());
  EXPECT_FALSE(delegate->tap_down());
  EXPECT_FALSE(delegate->double_tap());
  EXPECT_FALSE(delegate->scroll_begin());
  EXPECT_FALSE(delegate->scroll_update());
  EXPECT_FALSE(delegate->scroll_end());
}

// Check that appropriate touch events generate scroll gesture events.
TEST_F(GestureRecognizerTest, GestureEventScroll) {
  scoped_ptr<GestureEventConsumeDelegate> delegate(
      new GestureEventConsumeDelegate());
  const int kWindowWidth = 123;
  const int kWindowHeight = 45;
  gfx::Rect bounds(100, 200, kWindowWidth, kWindowHeight);
  scoped_ptr<aura::Window> window(CreateTestWindowWithDelegate(
      delegate.get(), -1234, bounds, NULL));

  delegate->Reset();
  TouchEvent press(ui::ET_TOUCH_PRESSED, gfx::Point(101, 201), 0);
  RootWindow::GetInstance()->DispatchTouchEvent(&press);
  EXPECT_FALSE(delegate->tap());
  EXPECT_TRUE(delegate->tap_down());
  EXPECT_FALSE(delegate->double_tap());
  EXPECT_FALSE(delegate->scroll_begin());
  EXPECT_FALSE(delegate->scroll_update());
  EXPECT_FALSE(delegate->scroll_end());

  // Move the touch-point enough so that it is considered as a scroll. This
  // should generate both SCROLL_BEGIN and SCROLL_UPDATE gestures.
  // The first movement is diagonal, to ensure that we have a free scroll,
  // and not a rail scroll.
  SendScrollEvent(130, 230, delegate.get());
  EXPECT_FALSE(delegate->tap());
  EXPECT_FALSE(delegate->tap_down());
  EXPECT_FALSE(delegate->double_tap());
  EXPECT_TRUE(delegate->scroll_begin());
  EXPECT_TRUE(delegate->scroll_update());
  EXPECT_FALSE(delegate->scroll_end());
  EXPECT_EQ(29, delegate->scroll_x());
  EXPECT_EQ(29, delegate->scroll_y());

  // Move some more to generate a few more scroll updates.
  SendScrollEvent(110, 211, delegate.get());
  EXPECT_FALSE(delegate->tap());
  EXPECT_FALSE(delegate->tap_down());
  EXPECT_FALSE(delegate->double_tap());
  EXPECT_FALSE(delegate->scroll_begin());
  EXPECT_TRUE(delegate->scroll_update());
  EXPECT_FALSE(delegate->scroll_end());
  EXPECT_EQ(-20, delegate->scroll_x());
  EXPECT_EQ(-19, delegate->scroll_y());

  SendScrollEvent(140, 215, delegate.get());
  EXPECT_FALSE(delegate->tap());
  EXPECT_FALSE(delegate->tap_down());
  EXPECT_FALSE(delegate->double_tap());
  EXPECT_FALSE(delegate->scroll_begin());
  EXPECT_TRUE(delegate->scroll_update());
  EXPECT_FALSE(delegate->scroll_end());
  EXPECT_EQ(30, delegate->scroll_x());
  EXPECT_EQ(4, delegate->scroll_y());

  // Release the touch. This should end the scroll.
  delegate->Reset();
  TouchEvent release(ui::ET_TOUCH_RELEASED, gfx::Point(101, 201), 0);
  Event::TestApi test_release(&release);
  test_release.set_time_stamp(press.time_stamp() +
                              base::TimeDelta::FromMilliseconds(50));
  RootWindow::GetInstance()->DispatchTouchEvent(&release);
  EXPECT_FALSE(delegate->tap());
  EXPECT_FALSE(delegate->tap_down());
  EXPECT_FALSE(delegate->double_tap());
  EXPECT_FALSE(delegate->scroll_begin());
  EXPECT_FALSE(delegate->scroll_update());
  EXPECT_TRUE(delegate->scroll_end());
}

// Check that horizontal scroll gestures cause scrolls on horizontal rails.
// Also tests that horizontal rails can be broken.
TEST_F(GestureRecognizerTest, GestureEventHorizontalRailScroll) {
  scoped_ptr<GestureEventConsumeDelegate> delegate(
      new GestureEventConsumeDelegate());
  gfx::Rect bounds(0, 0, 1000, 1000);
  scoped_ptr<aura::Window> window(CreateTestWindowWithDelegate(
      delegate.get(), -1234, bounds, NULL));

  TouchEvent press(ui::ET_TOUCH_PRESSED, gfx::Point(0, 0), 0);
  RootWindow::GetInstance()->DispatchTouchEvent(&press);

  // Move the touch-point horizontally enough that it is considered a
  // horizontal scroll.
  SendScrollEvent(20, 1, delegate.get());
  EXPECT_EQ(0, delegate->scroll_y());
  EXPECT_EQ(20, delegate->scroll_x());

  SendScrollEvent(25, 6, delegate.get());
  EXPECT_TRUE(delegate->scroll_update());
  EXPECT_EQ(5, delegate->scroll_x());
  // y shouldn't change, as we're on a horizontal rail.
  EXPECT_EQ(0, delegate->scroll_y());

  // Send enough information that a velocity can be calculated for the gesture,
  // and we can break the rail
  SendScrollEvents(1, 1, press.time_stamp(),
                   1, 100, 1, kBufferedPoints, delegate.get());

  SendScrollEvent(0, 0, delegate.get());
  SendScrollEvent(5, 5, delegate.get());

  // The rail should be broken
  EXPECT_TRUE(delegate->scroll_update());
  EXPECT_EQ(5, delegate->scroll_x());
  EXPECT_EQ(5, delegate->scroll_y());
}

// Check that vertical scroll gestures cause scrolls on vertical rails.
// Also tests that vertical rails can be broken.
TEST_F(GestureRecognizerTest, GestureEventVerticalRailScroll) {
  scoped_ptr<GestureEventConsumeDelegate> delegate(
      new GestureEventConsumeDelegate());
  gfx::Rect bounds(0, 0, 1000, 1000);
  scoped_ptr<aura::Window> window(CreateTestWindowWithDelegate(
      delegate.get(), -1234, bounds, NULL));

  TouchEvent press(ui::ET_TOUCH_PRESSED, gfx::Point(0, 0), 0);
  RootWindow::GetInstance()->DispatchTouchEvent(&press);

  // Move the touch-point vertically enough that it is considered a
  // vertical scroll.
  SendScrollEvent(1, 20, delegate.get());
  EXPECT_EQ(0, delegate->scroll_x());
  EXPECT_EQ(20, delegate->scroll_y());

  SendScrollEvent(6, 25, delegate.get());
  EXPECT_TRUE(delegate->scroll_update());
  EXPECT_EQ(5, delegate->scroll_y());
  // x shouldn't change, as we're on a vertical rail.
  EXPECT_EQ(0, delegate->scroll_x());

  // Send enough information that a velocity can be calculated for the gesture,
  // and we can break the rail
  SendScrollEvents(1, 1, press.time_stamp(),
                   100, 1, 1, kBufferedPoints, delegate.get());

  SendScrollEvent(0, 0, delegate.get());
  SendScrollEvent(5, 5, delegate.get());

  // The rail should be broken
  EXPECT_TRUE(delegate->scroll_update());
  EXPECT_EQ(5, delegate->scroll_x());
  EXPECT_EQ(5, delegate->scroll_y());
}

TEST_F(GestureRecognizerTest, GestureTapFollowedByScroll) {
  // First, tap. Then, do a scroll using the same touch-id.
  scoped_ptr<GestureEventConsumeDelegate> delegate(
      new GestureEventConsumeDelegate());
  const int kWindowWidth = 123;
  const int kWindowHeight = 45;
  gfx::Rect bounds(100, 200, kWindowWidth, kWindowHeight);
  scoped_ptr<aura::Window> window(CreateTestWindowWithDelegate(
      delegate.get(), -1234, bounds, NULL));

  delegate->Reset();
  TouchEvent press(ui::ET_TOUCH_PRESSED, gfx::Point(101, 201), 0);
  RootWindow::GetInstance()->DispatchTouchEvent(&press);
  EXPECT_FALSE(delegate->tap());
  EXPECT_TRUE(delegate->tap_down());
  EXPECT_FALSE(delegate->double_tap());
  EXPECT_FALSE(delegate->scroll_begin());
  EXPECT_FALSE(delegate->scroll_update());
  EXPECT_FALSE(delegate->scroll_end());

  // Make sure there is enough delay before the touch is released so that it is
  // recognized as a tap.
  delegate->Reset();
  TouchEvent release(ui::ET_TOUCH_RELEASED, gfx::Point(101, 201), 0);
  Event::TestApi test_release(&release);
  test_release.set_time_stamp(press.time_stamp() +
                              base::TimeDelta::FromMilliseconds(50));
  RootWindow::GetInstance()->DispatchTouchEvent(&release);
  EXPECT_TRUE(delegate->tap());
  EXPECT_FALSE(delegate->tap_down());
  EXPECT_FALSE(delegate->double_tap());
  EXPECT_FALSE(delegate->scroll_begin());
  EXPECT_FALSE(delegate->scroll_update());
  EXPECT_FALSE(delegate->scroll_end());

  // Now, do a scroll gesture. Delay it sufficiently so that it doesn't trigger
  // a double-tap.
  delegate->Reset();
  TouchEvent press1(ui::ET_TOUCH_PRESSED, gfx::Point(101, 201), 0);
  Event::TestApi test_release1(&press1);
  test_release1.set_time_stamp(release.time_stamp() +
                               base::TimeDelta::FromMilliseconds(1000));
  RootWindow::GetInstance()->DispatchTouchEvent(&press1);
  EXPECT_FALSE(delegate->tap());
  EXPECT_TRUE(delegate->tap_down());
  EXPECT_FALSE(delegate->double_tap());
  EXPECT_FALSE(delegate->scroll_begin());
  EXPECT_FALSE(delegate->scroll_update());
  EXPECT_FALSE(delegate->scroll_end());

  // Move the touch-point enough so that it is considered as a scroll. This
  // should generate both SCROLL_BEGIN and SCROLL_UPDATE gestures.
  // The first movement is diagonal, to ensure that we have a free scroll,
  // and not a rail scroll.
  delegate->Reset();
  TouchEvent move(ui::ET_TOUCH_MOVED, gfx::Point(130, 230), 0);
  RootWindow::GetInstance()->DispatchTouchEvent(&move);
  EXPECT_FALSE(delegate->tap());
  EXPECT_FALSE(delegate->tap_down());
  EXPECT_FALSE(delegate->double_tap());
  EXPECT_TRUE(delegate->scroll_begin());
  EXPECT_TRUE(delegate->scroll_update());
  EXPECT_FALSE(delegate->scroll_end());
  EXPECT_EQ(29, delegate->scroll_x());
  EXPECT_EQ(29, delegate->scroll_y());

  // Move some more to generate a few more scroll updates.
  delegate->Reset();
  TouchEvent move1(ui::ET_TOUCH_MOVED, gfx::Point(110, 211), 0);
  RootWindow::GetInstance()->DispatchTouchEvent(&move1);
  EXPECT_FALSE(delegate->tap());
  EXPECT_FALSE(delegate->tap_down());
  EXPECT_FALSE(delegate->double_tap());
  EXPECT_FALSE(delegate->scroll_begin());
  EXPECT_TRUE(delegate->scroll_update());
  EXPECT_FALSE(delegate->scroll_end());
  EXPECT_EQ(-20, delegate->scroll_x());
  EXPECT_EQ(-19, delegate->scroll_y());

  delegate->Reset();
  TouchEvent move2(ui::ET_TOUCH_MOVED, gfx::Point(140, 215), 0);
  RootWindow::GetInstance()->DispatchTouchEvent(&move2);
  EXPECT_FALSE(delegate->tap());
  EXPECT_FALSE(delegate->tap_down());
  EXPECT_FALSE(delegate->double_tap());
  EXPECT_FALSE(delegate->scroll_begin());
  EXPECT_TRUE(delegate->scroll_update());
  EXPECT_FALSE(delegate->scroll_end());
  EXPECT_EQ(30, delegate->scroll_x());
  EXPECT_EQ(4, delegate->scroll_y());

  // Release the touch. This should end the scroll.
  delegate->Reset();
  TouchEvent release1(ui::ET_TOUCH_RELEASED, gfx::Point(101, 201), 0);
  RootWindow::GetInstance()->DispatchTouchEvent(&release1);
  EXPECT_FALSE(delegate->tap());
  EXPECT_FALSE(delegate->tap_down());
  EXPECT_FALSE(delegate->double_tap());
  EXPECT_FALSE(delegate->scroll_begin());
  EXPECT_FALSE(delegate->scroll_update());
  EXPECT_TRUE(delegate->scroll_end());
}

// Check that unprocessed gesture events generate appropriate synthetic mouse
// events.
TEST_F(GestureRecognizerTest, GestureTapSyntheticMouse) {
  scoped_ptr<GestureEventSynthDelegate> delegate(
      new GestureEventSynthDelegate());
  scoped_ptr<Window> window(CreateTestWindowWithDelegate(delegate.get(), -1234,
        gfx::Rect(0, 0, 123, 45), NULL));

  delegate->Reset();
  GestureEvent tap(ui::ET_GESTURE_TAP, 20, 20, 0, base::Time::Now(), 0, 0);
  RootWindow::GetInstance()->DispatchGestureEvent(&tap);
  EXPECT_TRUE(delegate->mouse_enter());
  EXPECT_TRUE(delegate->mouse_press());
  EXPECT_TRUE(delegate->mouse_release());
  EXPECT_TRUE(delegate->mouse_exit());
}

TEST_F(GestureRecognizerTest, AsynchronousGestureRecognition) {
  scoped_ptr<QueueTouchEventDelegate> queued_delegate(
      new QueueTouchEventDelegate());
  const int kWindowWidth = 123;
  const int kWindowHeight = 45;
  gfx::Rect bounds(100, 200, kWindowWidth, kWindowHeight);
  scoped_ptr<aura::Window> queue(CreateTestWindowWithDelegate(
      queued_delegate.get(), -1234, bounds, NULL));

  queued_delegate->set_window(queue.get());

  // Touch down on the window. This should not generate any gesture event.
  queued_delegate->Reset();
  TouchEvent press(ui::ET_TOUCH_PRESSED, gfx::Point(101, 201), 0);
  RootWindow::GetInstance()->DispatchTouchEvent(&press);
  EXPECT_FALSE(queued_delegate->tap());
  EXPECT_FALSE(queued_delegate->tap_down());
  EXPECT_FALSE(queued_delegate->double_tap());
  EXPECT_FALSE(queued_delegate->scroll_begin());
  EXPECT_FALSE(queued_delegate->scroll_update());
  EXPECT_FALSE(queued_delegate->scroll_end());

  // Introduce some delay before the touch is released so that it is recognized
  // as a tap. However, this still should not create any gesture events.
  queued_delegate->Reset();
  TouchEvent release(ui::ET_TOUCH_RELEASED, gfx::Point(101, 201), 0);
  Event::TestApi test_release(&release);
  test_release.set_time_stamp(press.time_stamp() +
                              base::TimeDelta::FromMilliseconds(50));
  RootWindow::GetInstance()->DispatchTouchEvent(&release);
  EXPECT_FALSE(queued_delegate->tap());
  EXPECT_FALSE(queued_delegate->tap_down());
  EXPECT_FALSE(queued_delegate->double_tap());
  EXPECT_FALSE(queued_delegate->scroll_begin());
  EXPECT_FALSE(queued_delegate->scroll_update());
  EXPECT_FALSE(queued_delegate->scroll_end());

  // Create another window, and place a touch-down on it. This should create a
  // tap-down gesture.
  scoped_ptr<GestureEventConsumeDelegate> delegate(
      new GestureEventConsumeDelegate());
  scoped_ptr<aura::Window> window(CreateTestWindowWithDelegate(
      delegate.get(), -2345, gfx::Rect(0, 0, 50, 50), NULL));
  delegate->Reset();
  TouchEvent press2(ui::ET_TOUCH_PRESSED, gfx::Point(10, 20), 0);
  RootWindow::GetInstance()->DispatchTouchEvent(&press2);
  EXPECT_FALSE(delegate->tap());
  EXPECT_TRUE(delegate->tap_down());
  EXPECT_FALSE(delegate->double_tap());
  EXPECT_FALSE(delegate->scroll_begin());
  EXPECT_FALSE(delegate->scroll_update());
  EXPECT_FALSE(delegate->scroll_end());

  TouchEvent release2(ui::ET_TOUCH_RELEASED, gfx::Point(10, 20), 0);
  RootWindow::GetInstance()->DispatchTouchEvent(&release2);

  // Process the first queued event.
  queued_delegate->Reset();
  queued_delegate->ReceivedAck();
  EXPECT_FALSE(queued_delegate->tap());
  EXPECT_TRUE(queued_delegate->tap_down());
  EXPECT_FALSE(queued_delegate->double_tap());
  EXPECT_FALSE(queued_delegate->scroll_begin());
  EXPECT_FALSE(queued_delegate->scroll_update());
  EXPECT_FALSE(queued_delegate->scroll_end());

  // Now, process the second queued event.
  queued_delegate->Reset();
  queued_delegate->ReceivedAck();
  EXPECT_TRUE(queued_delegate->tap());
  EXPECT_FALSE(queued_delegate->tap_down());
  EXPECT_FALSE(queued_delegate->double_tap());
  EXPECT_FALSE(queued_delegate->scroll_begin());
  EXPECT_FALSE(queued_delegate->scroll_update());
  EXPECT_FALSE(queued_delegate->scroll_end());

  // Start all over. Press on the first window, then press again on the second
  // window. The second press should still go to the first window.
  queued_delegate->Reset();
  TouchEvent press3(ui::ET_TOUCH_PRESSED, gfx::Point(101, 201), 0);
  RootWindow::GetInstance()->DispatchTouchEvent(&press3);
  EXPECT_FALSE(queued_delegate->tap());
  EXPECT_FALSE(queued_delegate->tap_down());
  EXPECT_FALSE(queued_delegate->double_tap());
  EXPECT_FALSE(queued_delegate->scroll_begin());
  EXPECT_FALSE(queued_delegate->scroll_update());
  EXPECT_FALSE(queued_delegate->scroll_end());

  queued_delegate->Reset();
  delegate->Reset();
  TouchEvent press4(ui::ET_TOUCH_PRESSED, gfx::Point(10, 20), 1);
  RootWindow::GetInstance()->DispatchTouchEvent(&press4);
  EXPECT_FALSE(delegate->tap());
  EXPECT_FALSE(delegate->tap_down());
  EXPECT_FALSE(delegate->double_tap());
  EXPECT_FALSE(delegate->scroll_begin());
  EXPECT_FALSE(delegate->scroll_update());
  EXPECT_FALSE(delegate->scroll_end());
  EXPECT_FALSE(queued_delegate->tap());
  EXPECT_FALSE(queued_delegate->tap_down());
  EXPECT_FALSE(queued_delegate->double_tap());
  EXPECT_FALSE(queued_delegate->scroll_begin());
  EXPECT_FALSE(queued_delegate->scroll_update());
  EXPECT_FALSE(queued_delegate->scroll_end());

  queued_delegate->Reset();
  queued_delegate->ReceivedAck();
  EXPECT_FALSE(queued_delegate->tap());
  EXPECT_TRUE(queued_delegate->tap_down());
  EXPECT_FALSE(queued_delegate->double_tap());
  EXPECT_FALSE(queued_delegate->scroll_begin());
  EXPECT_FALSE(queued_delegate->scroll_update());
  EXPECT_FALSE(queued_delegate->scroll_end());

  queued_delegate->Reset();
  queued_delegate->ReceivedAck();
  EXPECT_FALSE(queued_delegate->tap());
  EXPECT_TRUE(queued_delegate->tap_down());
  EXPECT_FALSE(queued_delegate->double_tap());
  EXPECT_TRUE(queued_delegate->scroll_begin());
  EXPECT_FALSE(queued_delegate->scroll_update());
  EXPECT_FALSE(queued_delegate->scroll_end());
  EXPECT_TRUE(queued_delegate->pinch_begin());
  EXPECT_FALSE(queued_delegate->pinch_update());
  EXPECT_FALSE(queued_delegate->pinch_end());
}

// Check that appropriate touch events generate pinch gesture events.
TEST_F(GestureRecognizerTest, GestureEventPinchFromScroll) {
  scoped_ptr<GestureEventConsumeDelegate> delegate(
      new GestureEventConsumeDelegate());
  const int kWindowWidth = 300;
  const int kWindowHeight = 400;
  gfx::Rect bounds(5, 5, kWindowWidth, kWindowHeight);
  scoped_ptr<aura::Window> window(CreateTestWindowWithDelegate(
      delegate.get(), -1234, bounds, NULL));

  aura::RootWindow* root = RootWindow::GetInstance();

  delegate->Reset();
  TouchEvent press(ui::ET_TOUCH_PRESSED, gfx::Point(101, 201), 0);
  root->DispatchTouchEvent(&press);
  EXPECT_FALSE(delegate->tap());
  EXPECT_TRUE(delegate->tap_down());
  EXPECT_FALSE(delegate->double_tap());
  EXPECT_FALSE(delegate->scroll_begin());
  EXPECT_FALSE(delegate->scroll_update());
  EXPECT_FALSE(delegate->scroll_end());

  // Move the touch-point enough so that it is considered as a scroll. This
  // should generate both SCROLL_BEGIN and SCROLL_UPDATE gestures.
  delegate->Reset();
  TouchEvent move(ui::ET_TOUCH_MOVED, gfx::Point(130, 201), 0);
  root->DispatchTouchEvent(&move);
  EXPECT_FALSE(delegate->tap());
  EXPECT_FALSE(delegate->tap_down());
  EXPECT_FALSE(delegate->double_tap());
  EXPECT_TRUE(delegate->scroll_begin());
  EXPECT_TRUE(delegate->scroll_update());
  EXPECT_FALSE(delegate->scroll_end());

  // Press the second finger. It should cause both a tap-down and pinch-begin.
  delegate->Reset();
  TouchEvent press2(ui::ET_TOUCH_PRESSED, gfx::Point(10, 10), 1);
  root->DispatchTouchEvent(&press2);
  EXPECT_FALSE(delegate->tap());
  EXPECT_TRUE(delegate->tap_down());
  EXPECT_FALSE(delegate->double_tap());
  EXPECT_FALSE(delegate->scroll_begin());
  EXPECT_FALSE(delegate->scroll_update());
  EXPECT_FALSE(delegate->scroll_end());
  EXPECT_TRUE(delegate->pinch_begin());

  // Move the first finger.
  delegate->Reset();
  TouchEvent move3(ui::ET_TOUCH_MOVED, gfx::Point(95, 201), 0);
  root->DispatchTouchEvent(&move3);
  EXPECT_FALSE(delegate->tap());
  EXPECT_FALSE(delegate->tap_down());
  EXPECT_FALSE(delegate->double_tap());
  EXPECT_FALSE(delegate->scroll_begin());
  EXPECT_FALSE(delegate->scroll_update());
  EXPECT_FALSE(delegate->scroll_end());
  EXPECT_FALSE(delegate->pinch_begin());
  EXPECT_TRUE(delegate->pinch_update());

  // Now move the second finger.
  delegate->Reset();
  TouchEvent move4(ui::ET_TOUCH_MOVED, gfx::Point(55, 15), 1);
  root->DispatchTouchEvent(&move4);
  EXPECT_FALSE(delegate->tap());
  EXPECT_FALSE(delegate->tap_down());
  EXPECT_FALSE(delegate->double_tap());
  EXPECT_FALSE(delegate->scroll_begin());
  EXPECT_FALSE(delegate->scroll_update());
  EXPECT_FALSE(delegate->scroll_end());
  EXPECT_FALSE(delegate->pinch_begin());
  EXPECT_TRUE(delegate->pinch_update());

  // Release the first finger. This should end pinch.
  delegate->Reset();
  TouchEvent release(ui::ET_TOUCH_RELEASED, gfx::Point(101, 201), 0);
  Event::TestApi test_release(&release);
  test_release.set_time_stamp(press.time_stamp() +
                              base::TimeDelta::FromMilliseconds(50));
  root->DispatchTouchEvent(&release);
  EXPECT_FALSE(delegate->tap());
  EXPECT_FALSE(delegate->tap_down());
  EXPECT_FALSE(delegate->double_tap());
  EXPECT_FALSE(delegate->scroll_begin());
  EXPECT_FALSE(delegate->scroll_update());
  EXPECT_FALSE(delegate->scroll_end());
  EXPECT_TRUE(delegate->pinch_end());

  // Move the second finger. This should still generate a scroll.
  delegate->Reset();
  TouchEvent move5(ui::ET_TOUCH_MOVED, gfx::Point(25, 10), 1);
  root->DispatchTouchEvent(&move5);
  EXPECT_FALSE(delegate->tap());
  EXPECT_FALSE(delegate->tap_down());
  EXPECT_FALSE(delegate->double_tap());
  EXPECT_FALSE(delegate->scroll_begin());
  EXPECT_TRUE(delegate->scroll_update());
  EXPECT_FALSE(delegate->scroll_end());
  EXPECT_FALSE(delegate->pinch_begin());
  EXPECT_FALSE(delegate->pinch_update());
}

TEST_F(GestureRecognizerTest, GestureEventPinchFromTap) {
  scoped_ptr<GestureEventConsumeDelegate> delegate(
      new GestureEventConsumeDelegate());
  const int kWindowWidth = 300;
  const int kWindowHeight = 400;
  gfx::Rect bounds(5, 5, kWindowWidth, kWindowHeight);
  scoped_ptr<aura::Window> window(CreateTestWindowWithDelegate(
      delegate.get(), -1234, bounds, NULL));

  aura::RootWindow* root = RootWindow::GetInstance();

  delegate->Reset();
  TouchEvent press(ui::ET_TOUCH_PRESSED, gfx::Point(101, 201), 0);
  root->DispatchTouchEvent(&press);
  EXPECT_FALSE(delegate->tap());
  EXPECT_TRUE(delegate->tap_down());
  EXPECT_FALSE(delegate->double_tap());
  EXPECT_FALSE(delegate->scroll_begin());
  EXPECT_FALSE(delegate->scroll_update());
  EXPECT_FALSE(delegate->scroll_end());

  // Press the second finger. It should cause a tap-down, scroll-begin and
  // pinch-begin.
  delegate->Reset();
  TouchEvent press2(ui::ET_TOUCH_PRESSED, gfx::Point(10, 10), 1);
  root->DispatchTouchEvent(&press2);
  EXPECT_FALSE(delegate->tap());
  EXPECT_TRUE(delegate->tap_down());
  EXPECT_FALSE(delegate->double_tap());
  EXPECT_TRUE(delegate->scroll_begin());
  EXPECT_FALSE(delegate->scroll_update());
  EXPECT_FALSE(delegate->scroll_end());
  EXPECT_TRUE(delegate->pinch_begin());

  // Move the first finger.
  delegate->Reset();
  TouchEvent move3(ui::ET_TOUCH_MOVED, gfx::Point(65, 201), 0);
  root->DispatchTouchEvent(&move3);
  EXPECT_FALSE(delegate->tap());
  EXPECT_FALSE(delegate->tap_down());
  EXPECT_FALSE(delegate->double_tap());
  EXPECT_FALSE(delegate->scroll_begin());
  EXPECT_FALSE(delegate->scroll_update());
  EXPECT_FALSE(delegate->scroll_end());
  EXPECT_FALSE(delegate->pinch_begin());
  EXPECT_TRUE(delegate->pinch_update());

  // Now move the second finger.
  delegate->Reset();
  TouchEvent move4(ui::ET_TOUCH_MOVED, gfx::Point(55, 15), 1);
  root->DispatchTouchEvent(&move4);
  EXPECT_FALSE(delegate->tap());
  EXPECT_FALSE(delegate->tap_down());
  EXPECT_FALSE(delegate->double_tap());
  EXPECT_FALSE(delegate->scroll_begin());
  EXPECT_FALSE(delegate->scroll_update());
  EXPECT_FALSE(delegate->scroll_end());
  EXPECT_FALSE(delegate->pinch_begin());
  EXPECT_TRUE(delegate->pinch_update());

  // Release the first finger. This should end pinch.
  delegate->Reset();
  TouchEvent release(ui::ET_TOUCH_RELEASED, gfx::Point(101, 201), 0);
  Event::TestApi test_release(&release);
  test_release.set_time_stamp(press.time_stamp() +
                              base::TimeDelta::FromMilliseconds(50));
  root->DispatchTouchEvent(&release);
  EXPECT_FALSE(delegate->tap());
  EXPECT_FALSE(delegate->tap_down());
  EXPECT_FALSE(delegate->double_tap());
  EXPECT_FALSE(delegate->scroll_begin());
  EXPECT_FALSE(delegate->scroll_update());
  EXPECT_FALSE(delegate->scroll_end());
  EXPECT_TRUE(delegate->pinch_end());

  // Move the second finger. This should still generate a scroll.
  delegate->Reset();
  TouchEvent move5(ui::ET_TOUCH_MOVED, gfx::Point(25, 10), 1);
  root->DispatchTouchEvent(&move5);
  EXPECT_FALSE(delegate->tap());
  EXPECT_FALSE(delegate->tap_down());
  EXPECT_FALSE(delegate->double_tap());
  EXPECT_FALSE(delegate->scroll_begin());
  EXPECT_TRUE(delegate->scroll_update());
  EXPECT_FALSE(delegate->scroll_end());
  EXPECT_FALSE(delegate->pinch_begin());
  EXPECT_FALSE(delegate->pinch_update());
}

}  // namespace test
}  // namespace aura
