// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_WAYLAND_WAYLAND_TOUCH_H_
#define UI_OZONE_PLATFORM_WAYLAND_WAYLAND_TOUCH_H_

#include <memory>
#include <unordered_map>

#include "ui/events/ozone/evdev/event_dispatch_callback.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/ozone/platform/wayland/wayland_object.h"

namespace ui {

class WaylandConnection;
class TouchEvent;

namespace {

struct TouchPoint {
  TouchPoint();
  TouchPoint(gfx::Point location,
             std::unique_ptr<TouchEvent> touch_event,
             wl_surface* current_surface);
  ~TouchPoint();

  wl_surface* surface;
  std::unique_ptr<TouchEvent> event;
  gfx::Point last_known_location;
};

}  // namespace

class WaylandTouch {
 public:
  using TouchPoints = std::unordered_map<int32_t, std::unique_ptr<TouchPoint>>;

  WaylandTouch(wl_touch* touch, const EventDispatchCallback& callback);
  virtual ~WaylandTouch();

  void set_connection(WaylandConnection* connection) {
    connection_ = connection;
  }

 private:
  // wl_touch_listener
  static void Down(void* data,
                   wl_touch* obj,
                   uint32_t serial,
                   uint32_t time,
                   struct wl_surface* surface,
                   int32_t id,
                   wl_fixed_t x,
                   wl_fixed_t y);
  static void Up(void* data,
                 wl_touch* obj,
                 uint32_t serial,
                 uint32_t time,
                 int32_t id);
  static void Motion(void* data,
                     wl_touch* obj,
                     uint32_t time,
                     int32_t id,
                     wl_fixed_t x,
                     wl_fixed_t y);
  static void Frame(void* data, wl_touch* obj);
  static void Cancel(void* data, wl_touch* obj);

  void SetSerial(uint32_t serial);

  WaylandConnection* connection_ = nullptr;
  wl::Object<wl_touch> obj_;
  EventDispatchCallback callback_;
  TouchPoints current_points_;
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_WAYLAND_WAYLAND_TOUCH_H_
