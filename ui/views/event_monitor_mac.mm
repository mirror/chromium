// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ui/views/event_monitor_mac.h"

#import <Cocoa/Cocoa.h>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "ui/display/screen.h"
#include "ui/events/event.h"
#include "ui/events/event_handler.h"
#include "ui/events/event_utils.h"

// A ref-counted object to enclose a weak pointer to the event monitor.
// AppKit posts to all event monitors active when an event is received, so it's
// possible for one monitor to remove another, and the removed monitor to still
// see the event.
@interface EventMonitorMacHandle : NSObject {
 @public
  views::EventMonitorMac* weakOwner;
}
@end
@implementation EventMonitorMacHandle
@end

namespace views {

// static
std::unique_ptr<EventMonitor> EventMonitor::CreateApplicationMonitor(
    ui::EventHandler* event_handler) {
  return base::WrapUnique(new EventMonitorMac(event_handler, nullptr));
}

// static
std::unique_ptr<EventMonitor> EventMonitor::CreateWindowMonitor(
    ui::EventHandler* event_handler,
    gfx::NativeWindow target_window) {
  return base::WrapUnique(new EventMonitorMac(event_handler, target_window));
}

// static
gfx::Point EventMonitor::GetLastMouseLocation() {
  return display::Screen::GetScreen()->GetCursorScreenPoint();
}

EventMonitorMac::EventMonitorMac(ui::EventHandler* event_handler,
                                 gfx::NativeWindow target_window) {
  DCHECK(event_handler);
  handle_.reset([[EventMonitorMacHandle alloc] init]);
  handle_.get()->weakOwner = this;
  EventMonitorMacHandle* handle = handle_;  // Ensure the object gets captured.

  auto block = ^NSEvent*(NSEvent* event) {
    if (!handle->weakOwner)
      return event;

    if (!target_window || [event window] == target_window) {
      std::unique_ptr<ui::Event> ui_event = ui::EventFromNative(event);
      if (ui_event)
        event_handler->OnEvent(ui_event.get());
    }
    return event;
  };

  monitor_ = [NSEvent addLocalMonitorForEventsMatchingMask:NSAnyEventMask
                                                   handler:block];
}

EventMonitorMac::~EventMonitorMac() {
  handle_.get()->weakOwner = nullptr;
  [NSEvent removeMonitor:monitor_];
}

}  // namespace views
