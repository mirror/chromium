// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/wayland/wayland_touch.h"

#include <sys/mman.h>
#include <wayland-client.h>

#include "base/files/scoped_file.h"
#include "ui/base/ui_features.h"
#include "ui/events/event.h"
#include "ui/ozone/platform/wayland/wayland_connection.h"
#include "ui/ozone/platform/wayland/wayland_window.h"

namespace ui {

namespace {

TouchPoint::TouchPoint() = default;

TouchPoint::TouchPoint(gfx::Point location,
                       std::unique_ptr<TouchEvent> touch_event,
                       wl_surface* current_surface)
    : surface(current_surface),
      event(std::move(touch_event)),
      last_known_location(location) {}

TouchPoint::~TouchPoint() = default;

}  // namespace

WaylandTouch::WaylandTouch(wl_touch* touch,
                           const EventDispatchCallback& callback)
    : obj_(touch), callback_(callback) {
  // TODO(linkmauve): Add support for wl_touch version 6.
  static const wl_touch_listener listener = {
      &WaylandTouch::Down,  &WaylandTouch::Up,     &WaylandTouch::Motion,
      &WaylandTouch::Frame, &WaylandTouch::Cancel,
  };

  wl_touch_add_listener(obj_.get(), &listener, this);
}

WaylandTouch::~WaylandTouch() {
  DCHECK(current_points_.empty());
}

void WaylandTouch::Down(void* data,
                        wl_touch* obj,
                        uint32_t serial,
                        uint32_t time,
                        struct wl_surface* surface,
                        int32_t id,
                        wl_fixed_t x,
                        wl_fixed_t y) {
  if (!surface)
    return;
  WaylandTouch* touch = static_cast<WaylandTouch*>(data);
  touch->SetSerial(serial);
  WaylandWindow::FromSurface(surface)->set_touch_focus(true);

  // Make sure this touch point wasn't present before.
  DCHECK(touch->current_points_.find(id) == touch->current_points_.end());

  EventType type = ET_TOUCH_PRESSED;
  gfx::Point location(wl_fixed_to_double(x), wl_fixed_to_double(y));
  base::TimeTicks time_stamp =
      base::TimeTicks() + base::TimeDelta::FromMilliseconds(time);
  PointerDetails pointer_details(EventPointerType::POINTER_TYPE_TOUCH, id);
  std::unique_ptr<TouchEvent> event(
      new TouchEvent(type, location, time_stamp, pointer_details));
  touch->callback_.Run(event.get());

  std::unique_ptr<TouchPoint> point(
      new TouchPoint(location, std::move(event), surface));
  touch->current_points_[id] = std::move(point);
}

static void MaybeUnsetFocus(const WaylandTouch::TouchPoints& points,
                            int32_t id) {
  wl_surface* surface = points.find(id)->second->surface;

  for (const auto& point : points) {
    // Return early on the first other point having this surface.
    if (surface == point.second->surface && id != point.first)
      return;
  }
  WaylandWindow::FromSurface(surface)->set_touch_focus(false);
}

void WaylandTouch::Up(void* data,
                      wl_touch* obj,
                      uint32_t serial,
                      uint32_t time,
                      int32_t id) {
  WaylandTouch* touch = static_cast<WaylandTouch*>(data);
  const auto iterator = touch->current_points_.find(id);

  // Make sure this touch point was present before.
  DCHECK(iterator != touch->current_points_.end());

  EventType type = ET_TOUCH_RELEASED;
  base::TimeTicks time_stamp =
      base::TimeTicks() + base::TimeDelta::FromMilliseconds(time);
  PointerDetails pointer_details(EventPointerType::POINTER_TYPE_TOUCH, id);
  TouchEvent event(type, touch->current_points_[id]->last_known_location,
                   time_stamp, pointer_details);
  touch->callback_.Run(&event);

  MaybeUnsetFocus(touch->current_points_, id);
  touch->current_points_.erase(iterator);
}

void WaylandTouch::Motion(void* data,
                          wl_touch* obj,
                          uint32_t time,
                          int32_t id,
                          wl_fixed_t x,
                          wl_fixed_t y) {
  WaylandTouch* touch = static_cast<WaylandTouch*>(data);

  // Make sure this touch point wasn't present before.
  DCHECK(touch->current_points_.find(id) != touch->current_points_.end());

  EventType type = ET_TOUCH_MOVED;
  gfx::Point location(wl_fixed_to_double(x), wl_fixed_to_double(y));
  base::TimeTicks time_stamp =
      base::TimeTicks() + base::TimeDelta::FromMilliseconds(time);
  PointerDetails pointer_details(EventPointerType::POINTER_TYPE_TOUCH, id);
  std::unique_ptr<TouchEvent> event(
      new TouchEvent(type, location, time_stamp, pointer_details));
  touch->callback_.Run(event.get());
  touch->current_points_[id]->event = std::move(event);
  touch->current_points_[id]->last_known_location = location;
}

void WaylandTouch::Frame(void* data, wl_touch* obj) {
  WaylandTouch* touch = static_cast<WaylandTouch*>(data);
  for (auto& point : touch->current_points_) {
    // Replaces the TouchEvent created in a previous event in the frame with
    // nullptr.
    std::unique_ptr<TouchEvent> event = nullptr;
    event.swap(point.second->event);

    // Not all ids have to had been changed in a single frame.
    if (event)
      touch->callback_.Run(event.get());
  }
}

void WaylandTouch::Cancel(void* data, wl_touch* obj) {
  WaylandTouch* touch = static_cast<WaylandTouch*>(data);
  for (auto& point : touch->current_points_) {
    int32_t id = point.first;

    EventType type = ET_TOUCH_CANCELLED;
    base::TimeTicks time_stamp = base::TimeTicks::Now();
    PointerDetails pointer_details(EventPointerType::POINTER_TYPE_TOUCH, id);
    TouchEvent event(type, gfx::Point(), time_stamp, pointer_details);
    touch->callback_.Run(&event);

    WaylandWindow::FromSurface(point.second->surface)->set_touch_focus(false);
  }
  touch->current_points_.clear();
}

void WaylandTouch::SetSerial(uint32_t serial) {
  DCHECK(connection_);
  connection_->set_serial(serial);
}

}  // namespace ui
