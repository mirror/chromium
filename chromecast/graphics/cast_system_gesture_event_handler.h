// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_GRAPHICS_CAST_SYSTEM_GESTURE_EVENT_HANDLER_H_
#define CHROMECAST_GRAPHICS_CAST_SYSTEM_GESTURE_EVENT_HANDLER_H_

#include "base/macros.h"
#include "ui/events/event_handler.h"

namespace aura {
class Window;
}  // namespace aura

namespace gfx {
class Point;
class Rect;
}  // namespace gfx

namespace chromecast {

enum class CastSideSwipeOrigin { TOP, BOTTOM, LEFT, RIGHT, NONE };

// Interface for handlers triggered on reception of side-swipe gestures on the
// root window.
class CastSideSwipeGestureHandlerInterface {
 public:
  CastSideSwipeGestureHandlerInterface() = default;
  virtual ~CastSideSwipeGestureHandlerInterface() = default;

  // Triggered on the beginning of a swipe.
  virtual void OnSideSwipeBegin(CastSideSwipeOrigin swipe_origin,
                                ui::GestureEvent* gesture_event) = 0;

  // Triggered on the completion (finger up) of a swipe.
  virtual void OnSideSwipeEnd(CastSideSwipeOrigin swipe_origin,
                              ui::GestureEvent* gesture_event) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(CastSideSwipeGestureHandlerInterface);
};

// An event handler for detecting system-wide gestures performed on the screen.
// Recognizes swipe gestures that originate from the top, left, bottom, and
// right of the root window.
class CastSystemGestureEventHandler : public ui::EventHandler {
 public:
  explicit CastSystemGestureEventHandler(aura::Window* root_window);

  ~CastSystemGestureEventHandler() override;

  // Register a new handler for a side swipe event.
  void AddSideSwipeGestureHandler(
      CastSideSwipeGestureHandlerInterface* handler);

  // Remove the registration of a side swipe event handler.
  void RemoveSideSwipeGestureHandler(
      CastSideSwipeGestureHandlerInterface* handler);

  CastSideSwipeOrigin GetDragPosition(const gfx::Point& point,
                                      const gfx::Rect& screen_bounds) const;

  void OnGestureEvent(ui::GestureEvent* event) override;

 private:
  aura::Window* root_window_;
  CastSideSwipeOrigin current_swipe_;

  using SwipeGestureHandlersList =
      std::vector<CastSideSwipeGestureHandlerInterface*>;
  SwipeGestureHandlersList swipe_gesture_handlers_;

  DISALLOW_COPY_AND_ASSIGN(CastSystemGestureEventHandler);
};

}  // namespace chromecast

#endif  // CHROMECAST_GRAPHICS_CAST_SYSTEM_GESTURE_EVENT_HANDLER_H_